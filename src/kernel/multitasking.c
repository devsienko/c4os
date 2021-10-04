#include "stdlib.h"
#include "memory_manager.h"
#include "interrupts.h"
#include "multitasking.h"

bool multitasking_enabled = false;

void init_kernel_address_space(AddressSpace *address_space, phyaddr page_dir);

void init_multitasking() {
	list_init(&process_list);
	list_init(&thread_list);
	kernel_process = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	init_kernel_address_space(&kernel_process->address_space, kernel_page_dir);
	kernel_process->suspend = false;
	kernel_process->thread_count = 1;
	strncpy(kernel_process->name, "kernel", sizeof(kernel_process->name));
	list_append((List*)&process_list, (ListItem*)kernel_process);
	kernel_thread = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	kernel_thread->process = kernel_process;
	kernel_thread->suspend = false;
	kernel_thread->stack_size = PAGE_SIZE;

	list_append((List*)&thread_list, (ListItem*)kernel_thread);
	current_process = kernel_process;
	current_thread = kernel_thread;

	multitasking_enabled = true;
}

void switch_task(Registers *regs) {
	if (multitasking_enabled) {
		(*((char*)(0xB8000 + 79 * 2)))++; // just to indicate that switch task is working

		memcpy(&current_thread->state, regs, sizeof(Registers));
		
		do {
			current_thread = (Thread*)current_thread->list_item.next;
			current_process = current_thread->process;
		} while (current_thread->suspend || current_process->suspend);
		asm("movl %0, %%cr3"::"a"(current_process->address_space.page_dir));
		
		memcpy(regs, &current_thread->state, sizeof(Registers));

		return (uint32)&current_thread->state;
	}
}

void init_kernel_address_space(AddressSpace *address_space, phyaddr page_dir) {
	address_space->page_dir = page_dir;
	address_space->start = KERNEL_MEMORY_START;
	address_space->end = KERNEL_MEMORY_END;
	address_space->block_table_size = PAGE_SIZE / sizeof(VirtMemoryBlock);
	address_space->blocks = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
	address_space->block_count = 0;
}