#ifndef MULTITASKING_H
#define MULTITASKING_H

#include "stdlib.h"
#include "interrupts.h"

#define USER_PROGRAM_BASE 0x100000 // all exec binaries are started there

typedef struct {
	uint32 reserved_1;
	uint32 esp0;
	uint32 ss0;
	uint32 esp1;
	uint32 ss1;
	uint32 esp2;
	uint32 ss2;
	uint32 cr3;
	uint32 eip;
	uint32 eflags;
	uint32 eax;
	uint32 ecx;
	uint32 edx;
	uint32 ebx;
	uint32 esp;
	uint32 ebp;
	uint32 esi;
	uint32 edi;
	uint32 es;
	uint32 cs;
	uint32 ss;
	uint32 ds;
	uint32 fs;
	uint32 gs;
	uint32 ldtr;
	uint16 reserved_2;
	uint16 io_map_offset;
	uint8 io_map[8192 + 1];
} __attribute__((packed)) TSS;

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
void switch_task(Registers *regs, uint32 esp);
void tss_set_stack(uint32 ss, uint32 esp);

void run_image(phyaddr proc_image_start, char file_name[]);
Thread *create_thread(Process *process, void *entry_point, size_t stack_size, bool kernel, bool suspend);

#endif