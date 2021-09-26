format ELF

public _start
extrn _kernel_main

section ".text" executable

_start:
	movzx edx, dl
	push edx
	push esi
	push ebx
	call _kernel_main
 @@:
	jmp @b

;section ".data" writable