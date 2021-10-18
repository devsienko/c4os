#include "tty.h"
#include "dma.h"
#include "stdlib.h"
#include "memory_manager.h"
#include "interrupts.h"
#include "floppy.h"
#include "timer.h"
#include "multitasking.h"
#include "listfs.h"

void kernel_main(uint8 boot_disk_id, void *memory_map, uint64 first_file_sector_number) {
	init_memory_manager(memory_map);
	init_interrupts();
	init_multitasking();
	init_tty();
	init_floppy();
	set_text_attr(63);
	clear_screen();

	printf("Welcome to C4 OS!\n\n");

	printf("Boot disk id ------- %d\n", boot_disk_id);
	printf("Memory map --------- 0x%x\n\n", memory_map);

	display_memory_map(memory_map);

	printf("kernel_page_dir = 0x%x\n", kernel_page_dir);
	printf("memory_size = %d MB\n", memory_size / 1024 / 1024);
	printf("get_page_info(kernel_page_dir, 0xB8000) = 0x%x\n\n", get_page_info(kernel_page_dir, (void*)0xB8000));

	int i;
	for (i = 0; i < kernel_address_space.block_count; i++) {
		printf("type = %d, base = 0x%x, length = 0x%x\n", kernel_address_space.blocks[i].type, kernel_address_space.blocks[i].base,
			kernel_address_space.blocks[i].length);
	}
	printf("\n");
	
	while (true) {
		char buffer[256];
		out_string("Command>");
		in_string(buffer, sizeof(buffer));
		if(!strcmp("read", buffer))
			cmd_read_sect();
		else if(!strcmp("ticks", buffer))
			cmd_get_ticks();
		else if(!strcmp("ps", buffer))
			show_process_list();
		else if(!strcmp("ls", buffer))
			show_files((uint32)first_file_sector_number);
		else if(is_there_exe((uint32)first_file_sector_number, buffer))
			run_new_process((uint32)first_file_sector_number, buffer);
		else if(!strcmp("test", buffer)) {
			char *digits = "syscall test\n";
			asm("movl %0, %%ebx" : : "a" (digits) :); // syscall function parameter
			asm("movl $0, %eax"); // syscall function number
			asm("int $0x30");//0x20 - irq_base, 16 - handler index, 0x20 + 16 = 0x30
		}
		else 
			printf("You typed: %s\n", buffer);
	}
}

void show_files (uint32 file_sector_number) {
	listfs_file_header *file_header = get_file_info_by(file_sector_number);
	if(file_header == LISTFS_EMPTY)
		return;
	
	printf("name: %s\n", file_header->name);

	if(CHECK_BIT(file_header->flags, 1))
		printf(" type: directory");
	else
		printf(" type: file\n");

	if(file_header->parent == LISTFS_COMMON_INDICATOR)
		printf(" directory: root\n");
	else
		printf(" directory: not root\n");

	printf(" size: %d bytes\n", (uint32)file_header->size);
	
	if(file_header->next != LISTFS_COMMON_INDICATOR) {
		printf("\n");
		show_files((uint32)file_header->next);
	}
}

uint32 get_current_esp () {
	uint32 esp;
	asm("movl %%esp, %0":"=a"(esp));
	return esp;
}

void init_floppy () { 
	//! set drive 0 as current drive
	flpydsk_set_working_drive(0);

	//! install floppy disk to IR 38, uses IRQ 6
	flpydsk_install();

	//! set DMA buffer to 64k
	flpydsk_set_dma(TEMP_DMA_BUFFER_ADDR);
}

bool is_last_memory_map_entry(struct memory_map_entry *entry);

void cmd_get_ticks() { 
	printf("\nticks: %d\n", get_tick_count());
}

void cmd_read_sect() {
	uint32 sectornum = 0;
	char sectornumbuf [4];
	uint8* sector;

	printf ("\nPlease type in the sector number [0 is default] > ");
	in_string(sectornumbuf, sizeof(sectornumbuf));
	sectornum = atoi(sectornumbuf);

	printf("\nSector %d contents:\n\n", sectornum);

	// read sector from disk
	sector = (uint8*)flpydsk_read_sector(sectornum);

	// display sector
	if (sector != 0) {
		int i = 0;
		for (int c = 0; c < 4; c++) {
			for (int j = 0; j < 128; j++)
				printf("0x%x ", sector[i + j]);
			i += 128;
			printf("\nPress any key to continue...\n");

			char buffer[1];
			in_string(buffer, sizeof(buffer));
		}
	}
	else
		printf("\n*** Error reading sector from disk");

	printf("Done.\n");
}

void load_sector() {
	const int sector_per_track = 18;
}

void display_memory_map(void *memory_map) {
	char* memory_types[] = {
		{"Available"},			//memory_region.type==0
		{"Reserved"},			//memory_region.type==1
		{"ACPI Reclaim"},		//memory_region.type==2
		{"ACPI NVS Memory"}		//memory_region.type==3
	};
	
	struct memory_map_entry *entry = memory_map;
	int map_entry_size = 24; //in bytes
	int region_number = 1;
	while(true) {
		if(is_last_memory_map_entry(entry))
			break;
		
		printf("region %d: start - %u; length (bytes) - %u; type - %d (%s)\n", 
			region_number, 
			(unsigned long)entry->base,
			(unsigned long)entry->length,
			entry->type,
			memory_types[entry->type-1]);
		entry++;
		region_number++;
	}

	printf("\n");
}

bool is_last_memory_map_entry(struct memory_map_entry *entry) {
	bool result = entry->length == 0
		&& entry->length == 0
		&& entry->length == 0
		&& entry->length == 0;
	return result;
}

void show_process_list() {
	ListItem* process_item = process_list.first;
	for(int i = 0; i < process_list.count; i++) {
		char* process_name = (*((Process*)process_item)).name;
		printf("  process - %s\n", process_name);
		process_item = process_item->next;
	}
}

uint32 get_file_data_pointer(uint32 sector_list_sector_number) {
	//we support only 1-sector size files right now, so it's a little bit crazy function
	uint64 *sector_list = flpydsk_read_sector(sector_list_sector_number);
	uint32 result = LISTFS_COMMON_INDICATOR;
	for(int i = 0; ; i++) {
		if(sector_list[i] == LISTFS_COMMON_INDICATOR)
			break;
		result = (uint32)sector_list[i];
	}
	return result;
}

uint32 find_file(uint32 file_sector_number, char* file_name) {
	listfs_file_header *file_header = get_file_info_by(file_sector_number);
	
	if(!strcmp(file_name, file_header->name)) 
		return get_file_data_pointer((uint32)file_header->data);
	if(file_header->next == LISTFS_COMMON_INDICATOR)
		return LISTFS_COMMON_INDICATOR;
	
	return find_file((uint32)file_header->next, file_name);
}

int is_there_exe (uint32 file_sector_number, char* file_name) { 
	if (find_file(file_sector_number, file_name) == LISTFS_COMMON_INDICATOR)
		return 0;
	return 1;
}

phyaddr alloc_dma_buffer() {
	phyaddr buffer = 0x20000;
	flpydsk_set_dma(buffer);
	return buffer;
}

void run_new_process(uint32 file_sector_number, char* file_name) {
	printf("%s starting...\n", file_name);
	uint32 file_data_sector_number = find_file(file_sector_number, file_name);
	phyaddr dma_buffer = alloc_dma_buffer();
	(uint8*)flpydsk_read_sector(file_data_sector_number);
	
	phyaddr proc_image_start = alloc_phys_pages(1);
	temp_map_page(proc_image_start);
	uint16 floppy_sector_size = 512;
	//I don't map dma_buffer because of identity mapping the first megabyte
	memcpy((void*)TEMP_PAGE, (void*)dma_buffer, floppy_sector_size);

	run_image(proc_image_start, file_name);
	printf("%s have started.\n", file_name);
}