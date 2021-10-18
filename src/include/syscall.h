#ifndef SYSCALL_H
#define SYSCALL_H

#include "interrupts.h"

void syscall_handler(Registers *regs);

#endif