format ELF

public _start
extrn _kernel_main

section ".text" executable

_start:
	movzx edx, dl
	push ebx
	push esi
	push edx
	call _kernel_main
	add esp, 3 * 4
 @@:
	jmp @b

;section ".data" writable