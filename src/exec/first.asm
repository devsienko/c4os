use32
org    0x20000 ; DMA buffer

; set tss start:
mov eax, esp
mov [0x7FFFD004], eax ; 7FFFD000 = USER_MEMORY_END - 3 pages
mov eax, 0x10
mov [0x7FFFD008], eax

cli
mov ax, 0x23
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov eax, esp;
push 0x23
push 0x200000 ; esp of user mode code

pushf
pop eax ; Get EFLAGS back into EAX. The only way to read EFLAGS is to pushf then pop.
or eax, 0x200 ; Set the IF flag.
push eax ; Push the new EFLAGS value back onto the stack.

push 0x1B
push @f
iret

@@:
mov byte[0xB8000 + (24 * 80) * 2 + 46], al
inc al
jmp @b