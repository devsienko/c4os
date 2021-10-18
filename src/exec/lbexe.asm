use32
org     0x100000 ; see USER_PROGRAM_BASE

mov eax, 0
mov ebx, boot_file_name
int 0x30

mov al, 75; capital K code
@@:
mov byte[0xB8000 + (24 * 80) * 2], al
inc al
jmp @b

boot_file_name db "syscall test",10, 0