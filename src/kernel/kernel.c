#include "tty.h"
#include "stdlib.h"
#include "memory_manager.h"
#include "interrupts.h"
#include "floppy.h"
#include "timer.h"
#include "multitasking.h"

void kernel_main(uint8 boot_disk_id, void *memory_map) {
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
		else if(!strcmp("umode", buffer))
			switch_to_user_mode();
		else 
			printf("You typed: %s\n", buffer);
	}
}

uint32 get_current_esp () {
	uint32 esp;
	asm("movl %%esp, %0":"=a"(esp));
	return esp;
}

void switch_to_user_mode()
{
	tss_set_stack(0x10, get_current_esp());
   	// Set up a stack structure for switching to user mode.
	asm volatile("  \
		cli; \
		mov $0x23, %ax; \
		mov %ax, %ds; \
		mov %ax, %es; \
		mov %ax, %fs; \
		mov %ax, %gs; \
					\
		mov %esp, %eax; \
		pushl $0x23; \
		pushl %eax; \
		pushf; \
		pop %eax; \
		or $0x200, %eax; \
		push %eax; \
		pushl $0x1B; \
		push $1f; \
		iret; \
	1: \
		");
	printf("Hello, user world!");
	while(true) {}
}

void init_floppy () { 
	//! set drive 0 as current drive
	flpydsk_set_working_drive(0);

	//! install floppy disk to IR 38, uses IRQ 6
	flpydsk_install();

	//! set DMA buffer to 64k
	flpydsk_set_dma(0x12000);
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