#include "tty.h"
#include "stdlib.h"
#include "interrupts.h"

typedef struct {
	uint64 base;
	uint64 size;
} BootModuleInfo;

void kernel_main(uint8 boot_disk_id, void *memory_map, BootModuleInfo *boot_module_list) {
	init_interrupts();
	init_tty();
	set_text_attr(63);
	clear_screen();

	printf("Welcome to C4 OS!\n\n");

	printf("Boot disk id ------- %d\n", boot_disk_id);
	printf("Memory map --------- 0x%x\n", memory_map);
	printf("Boot module list --- 0x%x (undefined yet)\n\n", boot_module_list);

	printf("String is %s, char is %c, number is %d, hex number is 0x%x\n\n", __DATE__, 'A', 1234, 0x1234);

	display_memory_map(memory_map);

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

		printf("region %d: start - 0x%x; ", 
			region_number, 
			entry->base);
		printf("length (bytes) - 0x%x; ", 
			entry->length);
		printf("type - %d (%s)\n", 
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