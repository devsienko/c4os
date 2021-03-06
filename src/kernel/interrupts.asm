format ELF

public _irq_handlers
extrn _irq_handler

section ".text" executable

macro IRQ_handler index {
IRQ # index # _handler:
	push eax
	mov eax, index - 1
	jmp common_irq_handler	
}

rept 17 i {
	IRQ_handler i
}

; common interrupt handler
common_irq_handler:
	push ebx ecx edx esi edi ebp
	push ds es fs gs
	mov ecx, 16
	mov ds, cx
	mov es, cx
	mov fs, cx
	mov gs, cx
	mov edx, esp
	push edx
	push eax
	call _irq_handler
	add esp, 2 * 4 ; we clean the  (because before we pushed edx (4 bytes) and eax (4 bytes)). More detalis 22:55 of https://www.youtube.com/watch?v=ySOm8CdpnXI
	pop gs fs es ds
	pop ebp edi esi edx ecx ebx
	mov al, 0x20
	out 0x20, al
	out 0xA0, al
	pop eax
	iretd

section ".data" writable

; interrupt handlers table
_irq_handlers:
	rept 17 i {
		dd IRQ # i # _handler
	} 