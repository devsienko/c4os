#include "stdlib.h"

#ifndef TTY_H
#define TTY_H

void init_tty();
void out_char(char chr);
void out_string(char *str);
void clear_screen();
void set_text_attr(char attr);
void move_cursor(unsigned int pos);
void printf(char *fmt, ...);
char in_char(bool wait);
void in_string(char *buffer, size_t buffer_size);
void keyboard_interrupt();

#endif