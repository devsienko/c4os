#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "stdlib.h"

typedef struct {
	uint32 gs, fs, es, ds;
	uint32 ebp, edi, esi, edx, ecx, ebx, eax;
	uint32 eip, cs;
	uint32 eflags;
	uint32 esp, ss;
} Registers;

uint8 irq_base;
uint8 irq_count;

void init_interrupts();
void set_int_handler(uint8 index, void *handler, uint8 type);

#endif