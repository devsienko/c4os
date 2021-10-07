#include "stdlib.h"
#include "memory_manager.h"
#include "interrupts.h"
#include "multitasking.h"

bool multitasking_enabled = false;

TSS *tss = (void*)(USER_MEMORY_END - PAGE_SIZE * 3 + 1); // we store TSS within last three page of user address space

void init_kernel_address_space(AddressSpace *address_space, phyaddr page_dir);

void init_multitasking() {
	list_init(&process_list);
	list_init(&thread_list);
	kernel_process = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	init_kernel_address_space(&kernel_process->address_space, kernel_page_dir);
	init_address_space(&user_address_space, kernel_page_dir);
	
	alloc_virt_pages(&user_address_space, tss, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	
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

	asm("ltr %w0"::"a"(40));
	multitasking_enabled = true;
}

void tss_set_stack(uint32 ss, uint32 esp) {
    tss->ss0 = ss;
    tss->esp0 = esp;
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

void init_address_space(AddressSpace *address_space, phyaddr page_dir) {
	address_space->page_dir = page_dir;
	address_space->start = USER_MEMORY_START;
	address_space->end = USER_MEMORY_END;
	address_space->block_table_size = PAGE_SIZE / sizeof(VirtMemoryBlock);
	address_space->blocks = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
	address_space->block_count = 0;
}