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

void switch_task(Registers *regs, uint32 esp) {
	if (multitasking_enabled) {
		(*((char*)(0xB8000 + 79 * 2)))++; // just to indicate that switch task is working

		memcpy(&current_thread->state, regs, sizeof(Registers));
		
		do {
			current_thread = (Thread*)current_thread->list_item.next;
			current_process = current_thread->process;
		} while (current_thread->suspend || current_process->suspend);
		asm("movl %0, %%cr3"::"a"(current_process->address_space.page_dir));
		
		memcpy(regs, &current_thread->state, sizeof(Registers));
		
		if(current_process->address_space.page_dir != 0x1000) { //if it's not kernel process
			tss_set_stack(0x10, esp + 17 * 4);
		}

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

Thread *create_thread(Process *process, void *entry_point, size_t stack_size_in_pages, 
	bool kernel, bool suspend) {
		
	Thread *thread = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	thread->process = process;
	thread->suspend = suspend;
	thread->stack_size = stack_size_in_pages << PAGE_OFFSET_BITS;
	thread->stack_base = alloc_virt_pages(&process->address_space, 0x200000, -1, 
		stack_size_in_pages, PAGE_PRESENT | PAGE_WRITABLE | (kernel ? 0 : PAGE_USER));
	memset(&thread->state, 0, sizeof(Registers));
	uint32 data_selector = (kernel ? 16 : 35);
	uint32 code_selector = (kernel ? 8 : 27);
	thread->state.eflags = 0x202;
	thread->state.cs = code_selector;
	thread->state.eip = (uint32)entry_point;
	thread->state.ss = data_selector;
	thread->state.esp = (uint32)thread->stack_base + thread->stack_size;
	thread->state.ds = data_selector;
	thread->state.es = data_selector;
	thread->state.fs = data_selector;
	thread->state.gs = data_selector;
	list_append((List*)&thread_list, (ListItem*)thread);
	process->thread_count++;
	
	return thread;
}

phyaddr get_kernel_page_table_addr () {
	temp_map_page(kernel_page_dir);
	uint32 *kernel_page_dir_p = (uint32*)TEMP_PAGE;
	
	uint16 kernel_page_table_index = 1022;
	phyaddr result = (phyaddr)kernel_page_dir_p[kernel_page_table_index];
	return result;
}

phyaddr get_last_page_table_addr () {
	temp_map_page(kernel_page_dir);
	uint32 *kernel_page_dir_p = (uint32*)TEMP_PAGE;
	
	uint16 last_page_table_index = 1023;
	phyaddr result = (phyaddr)kernel_page_dir_p[last_page_table_index];
	return result;
}

phyaddr init_page_tables(phyaddr memory_location_start) {
	uint8 kernel_pte_settings = PAGE_PRESENT | PAGE_WRITABLE;
	uint8 user_mode_pte_settings = PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
	
	// new process page table addresses:
	phyaddr first_page_table_phyaddr = alloc_phys_pages(1);
	phyaddr page_table_with_tss_records = alloc_phys_pages(1);
	phyaddr kernel_page_table_phyaddr = get_kernel_page_table_addr(); // we just copy page table of kernel process
	phyaddr last_page_table_phyaddr = get_last_page_table_addr(); // we just copy page table of kernel process
	
	// new process page dir:
	phyaddr page_dir = alloc_phys_pages(1);
	temp_map_page(page_dir);
	uint32 *page_dir_p = (uint32*)TEMP_PAGE;

	page_dir_p[0] = first_page_table_phyaddr | user_mode_pte_settings;
	page_dir_p[511] = page_table_with_tss_records | kernel_pte_settings; // 511 - because we use last three pages of user address space to store TSS segment data
	page_dir_p[1022] = kernel_page_table_phyaddr; // 1022 - the same approach like within kernel process
	page_dir_p[1023] = last_page_table_phyaddr; // 1023 - the same approach like within kernel process
	
	// identity mapping of the 1st Mb
	uint32 first_mb_limit = 0x100000;
	temp_map_page(first_page_table_phyaddr);
	uint32 *first_page_table_p = (uint32*)TEMP_PAGE;
	uint32 entry_value = 0 | user_mode_pte_settings;
	uint16 entries_count = first_mb_limit / PAGE_SIZE;
	for (int i = 0; i < entries_count; i++) {
		first_page_table_p[i] = entry_value;
		entry_value += 0x1000;
	}

	// identity mapping of 2-4 Mb, pay attention on memory_location_start
	phyaddr fourth_mb_limit = 0x400000;
	int start_index = first_mb_limit / PAGE_SIZE;
	entries_count = start_index + ((fourth_mb_limit - first_mb_limit) / PAGE_SIZE);
	entry_value = memory_location_start | user_mode_pte_settings;
	for (int i = start_index; i < entries_count; i++) {
		first_page_table_p[i] = entry_value;
		entry_value += 0x1000;
	}
	
	// fill page table with tss
	phyaddr page_for_tss = alloc_phys_pages(1);
	temp_map_page(page_table_with_tss_records);
	uint32 *page_table_with_tss_records_p = (uint32*)TEMP_PAGE;
	page_table_with_tss_records_p[1021] = page_for_tss | kernel_pte_settings;

	return page_dir;
}

void run_image(phyaddr proc_image_start, char file_name[]) {		
	phyaddr page_dir = init_page_tables(proc_image_start);
	Process *new_process = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	init_address_space(&new_process->address_space, page_dir);
	new_process->suspend = false;
	new_process->thread_count = 0;
	strncpy(new_process->name, file_name, sizeof(new_process->name));

	list_append((List*)&process_list, (ListItem*)new_process);
	create_thread(new_process, (void*)USER_PROGRAM_BASE, 1, false, false);
}
