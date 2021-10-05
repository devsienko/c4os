#include <stdarg.h>
#include "stdlib.h"
#include "memory_manager.h"
#include "tty.h"
#include "interrupts.h"
#include "scancodes.h"

typedef struct {
	uint8 chr;
	uint8 attr;
} TtyChar;

unsigned int tty_width;
unsigned int tty_height;
unsigned int cursor;
uint8 text_attr;
TtyChar *tty_buffer;
uint16 tty_io_port;

#define KEY_BUFFER_SIZE 16
char key_buffer[KEY_BUFFER_SIZE];
unsigned int key_buffer_head = 0;
unsigned int key_buffer_tail = 0; 

void keyboard_int_handler();

void init_tty() {
	tty_buffer = alloc_virt_pages(&kernel_address_space, NULL, 0xB8000, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
	tty_width = *((uint16*)0x44A);
	tty_height = 25;
	tty_io_port = *((uint16*)0x463);
	// cursor = (*((uint8*)0x451)) * tty_width + (*((uint8*)0x450));
    cursor = 0;
	text_attr = 7;
}

void out_char(char chr) {
	switch (chr) {
		case '\n':
			move_cursor((cursor / tty_width + 1) * tty_width);
			break;
		default:
			tty_buffer[cursor].chr = chr;
			tty_buffer[cursor].attr = text_attr;
			move_cursor(cursor + 1);
	}
}

void out_string(char *str) {
	while (*str) {
		out_char(*str);
		str++;
	}
}

void clear_screen() {
	memset_word(tty_buffer, (text_attr << 8) + ' ', tty_width * tty_height);
	move_cursor(0);
}

void set_text_attr(char attr) {
	text_attr = attr;
}

void move_cursor(unsigned int pos) {
	cursor = pos;
	if (cursor >= tty_width * tty_height) {
		cursor = (tty_height - 1) * tty_width;
		memcpy(tty_buffer, tty_buffer + tty_width, tty_width * tty_height * sizeof(TtyChar));
		memset_word(tty_buffer + tty_width * (tty_height - 1), (text_attr << 8) + ' ', tty_width);
	}
	outportb(tty_io_port, 0x0E);
	outportb(tty_io_port + 1, cursor >> 8);
	outportb(tty_io_port, 0x0F);
	outportb(tty_io_port + 1, cursor & 0xFF);
}

const char digits[] = "0123456789ABCDEF";
char num_buffer[65];

char *int_to_str(size_t value, unsigned char base) {
	size_t i = sizeof(num_buffer) - 1;
	num_buffer[i--] = '\0';
	do {
		num_buffer[i--] = digits[value % base];
		value = value / base;
	} while (value);
	return &num_buffer[i + 1];
}

char *uint32_to_str(uint32 value, unsigned char base) {
	size_t i = sizeof(num_buffer) - 1;
	num_buffer[i--] = '\0';
	do {
		num_buffer[i--] = digits[value % base];
		value = value / base;
	} while (value);
	return &num_buffer[i + 1];
}

void printf(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	while (*fmt) {
		if (*fmt == '%') {
			fmt++;
			size_t arg;
			uint32 uint32_arg;
			switch (*fmt) {
				case '%':
					out_char('%');
					break;
				case 'c':
					arg = va_arg(args, size_t);
					out_char(arg);
					break;
				case 's':
					arg = va_arg(args, size_t);
					out_string((char*)arg);
					break;
				case 'b':
					arg = va_arg(args, size_t);
					out_string(int_to_str(arg, 2));
					break;
				case 'o':
					arg = va_arg(args, size_t);
					out_string(int_to_str(arg, 8));
					break;
				case 'd':
					arg = va_arg(args, size_t);
					out_string(int_to_str(arg, 10));
					break;
				case 'u': //for unsigned long
					uint32_arg = va_arg(args, uint32);
					out_string(uint32_to_str(uint32_arg, 10));
					break;
				case 'x':
					arg = va_arg(args, size_t);
					out_string(int_to_str(arg, 16));
					break;
			}
		} else {
			out_char(*fmt);
		}
		fmt++;
	}
	va_end(args);
}

uint8 in_scancode() {
	uint8 result;
	if (key_buffer_head != key_buffer_tail) {
		if (key_buffer_head >= KEY_BUFFER_SIZE) {
			key_buffer_head = 0;
		}
		result = key_buffer[key_buffer_head];
		key_buffer_head++;
	} else {
		result = 0;
	}
	return result;
} 

char in_char(bool wait) {
	static bool shift = false;
	uint8 chr;
	do {
		chr = in_scancode();
		switch (chr) {
			case 0x2A:
			case 0x36:
				shift = true;
				break;
			case 0x2A + 0x80:
			case 0x36 + 0x80:
				shift = false;
				break;
		}
		if (chr & 0x80) {
			chr = 0;
		}
		if (shift) {
			chr = scancodes_shifted[chr];
		} else {
			chr = scancodes[chr];
		}
	} while (wait && (!chr));
	return chr;
} 

void in_string(char *buffer, size_t buffer_size) {
	char chr;
	size_t position = 0;
	do {
		chr = in_char(true);
		switch (chr) {
			case 0:
				break;
			case 8:
				if (position > 0) {
					position--;
					out_char(8);
				}
				break;
			case '\n':
				out_char('\n');
				break;
			default:
				if (position < buffer_size - 1) {
					buffer[position] = chr;
					position++;
					out_char(chr);
				}
		}
	} while (chr != '\n');
	buffer[position] = 0;
}

void keyboard_interrupt() {
	uint8 key_code;
	inportb(0x60, key_code);
	if (key_buffer_tail >= KEY_BUFFER_SIZE) {
		key_buffer_tail = 0;
	}
	key_buffer_tail++;
	key_buffer[key_buffer_tail - 1] = key_code;
	uint8 status;
	inportb(0x61, status);
	status |= 1;
	outportb(0x61, status);
}