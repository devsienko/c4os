#ifndef MULTITASKING_H
#define MULTITASKING_H

#include "stdlib.h"
#include "interrupts.h"

typedef struct {
	ListItem list_item;
	AddressSpace address_space;
	bool suspend;
	size_t thread_count;
	char name[256];
} Process;

typedef struct {
	ListItem list_item;
	Process *process;
	bool suspend;
	void *stack_base;
	size_t stack_size;
	void *stack_pointer;
	Registers state;
} Thread;

List process_list;
List thread_list;

Process *current_process;
Thread *current_thread;

Process *kernel_process;
Thread *kernel_thread;

void init_multitasking();
void switch_task(Registers *regs);

Thread *create_thread(Process *process, void *entry_point, size_t stack_size, bool kernel, bool suspend);

#endif