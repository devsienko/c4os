#include "stdlib.h"
#include "memory_manager.h"
#include "interrupts.h"
#include "multitasking.h"
#include "tty.h"
#include "timer.h"
#include "floppy.h"

void (*irq_handlers[])();
void irq_handler(uint32 index, Registers *regs);

typedef struct {
	uint16 address_0_15;
	uint16 selector;
	uint8 reserved;
	uint8 type;
	uint16 address_16_31;
} __attribute__((packed)) IntDesc;

typedef struct {
	uint16 limit;
	void *base;
} __attribute__((packed)) IDTR;

IntDesc *idt;

void timer_int_handler();

void init_interrupts() {
	idt = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
	memset(idt, 0, 256 * sizeof(IntDesc));
	IDTR idtr = {256 * sizeof(IntDesc), idt};
	asm("lidt (,%0,)"::"a"(&idtr));
	irq_base = 0x20;
	irq_count = 16;
	outportb(0x20, 0x11);
	outportb(0x21, irq_base);
	outportb(0x21, 4);
	outportb(0x21, 1);
	outportb(0xA0, 0x11);
	outportb(0xA1, irq_base + 8);
	outportb(0xA1, 2);
	outportb(0xA1, 1);
	int i;
	for (i = 0; i < 16; i++) {
		set_int_handler(irq_base + i, irq_handlers[i], 0x8E);
	}
	asm("sti");
}

void set_int_handler(uint8 index, void *handler, uint8 type) {
	asm("pushf \n cli");
	idt[index].selector = 8;
	idt[index].address_0_15 = (size_t)handler & 0xFFFF;
	idt[index].address_16_31 = (size_t)handler >> 16;
	idt[index].type = type;
	idt[index].reserved = 0;
	asm("popf"); 
}

void irq_handler(uint32 index, Registers *regs) {
	switch (index) {
		case 0:
			inc_pit_ticks();
			switch_task(regs);
			break;
		case 1:
			keyboard_interrupt();
			break;
		case 6: 
			i86_flpy_irq();
			break;
	}
}