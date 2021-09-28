#include "tty.h"
#include "stdlib.h"
#include "memory_manager.h"
#include "interrupts.h"

void kernel_main(uint8 boot_disk_id, void *memory_map) {
	init_memory_manager(memory_map);
	init_interrupts();
	init_tty();
	set_text_attr(63);
	clear_screen();

	printf("Welcome to C4 OS!\n\n");

	printf("Boot disk id ------- %d\n", boot_disk_id);
	printf("Memory map --------- 0x%x\n\n", memory_map);
	
	display_memory_map(memory_map);

	printf("kernel_page_dir = 0x%x\n", kernel_page_dir);
	printf("memory_size = %d MB\n", memory_size / 1024 / 1024);
	printf("get_page_info(kernel_page_dir, 0xB8000) = 0x%x\n\n", get_page_info(kernel_page_dir, (void*)0xB8000));

	while (true) {
		char buffer[256];
		out_string("Command>");
		in_string(buffer, sizeof(buffer));
		printf("You typed: %s\n", buffer);
	}
}

bool is_last_memory_map_entry(struct memory_map_entry *entry);

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