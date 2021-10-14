format ELF

public _start
extrn _kernel_main

section ".text" executable

_start:
	movzx edx, dl
	push ebx
	push eax
	push esi
	push edx
	call _kernel_main
	add esp, 4 * 4
 @@:
	jmp @b

;section ".data" writable