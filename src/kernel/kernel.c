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
	printf("Boot module list --- 0x%x\n\n", boot_module_list);

	printf("String is %s, char is %c, number is %d, hex number is 0x%x\n\n", __DATE__, 'A', 1234, 0x1234);
}