#ifndef TTY_H
#define TTY_H

void init_tty();
void out_char(char chr);
void out_string(char *str);
void clear_screen();
void set_text_attr(char attr);
void move_cursor(unsigned int pos);
void printf(char *fmt, ...);

#endif