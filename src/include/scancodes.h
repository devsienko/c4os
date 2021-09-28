#ifndef SCANCODES_H
#define SCANCODES_H

char scancodes[] = {
		0,
		0, // ESC
		'1','2','3','4','5','6','7','8','9','0', '-', '=',
		8, // BACKSPACE
		'\t', // TAB
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
		'\n', // ENTER
		0, // CTRL
		'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
		0, // LEFT SHIFT
		'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
		0, // RIGHT SHIFT
		'*', // NUMPAD
		0, // ALT
		' ', // SPACE
		0, // CAPSLOCK
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1 - F10
		0, // NUMLOCK
		0, // SCROLLLOCK
		0, // HOME
		0,
		0, // PAGE UP
		'-', // NUMPAD
		0, 0,
		0,
		'+', // NUMPAD
		0, // END
		0,
		0, // PAGE DOWN
		0, // INS
		0, // DEL
		0, // SYS RQ
		0,
		0, 0, // F11 - F12
		0,
		0, 0, 0, // F13 - F15
		0, 0, 0, 0, 0, 0, 0, 0, 0, // F16 - F24
		0, 0, 0, 0, 0, 0, 0, 0
	};
	
char scancodes_shifted[] = {
		0,
		0, // ESC
		'!','@','#','$','%','^','&','*','(',')', '_', '+',
		8, // BACKSPACE
		'\t', // TAB
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
		'\n', // ENTER
		0, // CTRL
		'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
		0, // LEFT SHIFT
		'|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
		0, // RIGHT SHIFT
		'*', // NUMPAD
		0, // ALT
		' ', // SPACE
		0, // CAPSLOCK
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1 - F10
		0, // NUMLOCK
		0, // SCROLLLOCK
		0, // HOME
		0,
		0, // PAGE UP
		'-', // NUMPAD
		0, 0,
		0,
		'+', // NUMPAD
		0, // END
		0,
		0, // PAGE DOWN
		0, // INS
		0, // DEL
		0, // SYS RQ
		0,
		0, 0, // F11 - F12
		0,
		0, 0, 0, // F13 - F15
		0, 0, 0, 0, 0, 0, 0, 0, 0, // F16 - F24
		0, 0, 0, 0, 0, 0, 0, 0
	};

#endif 