use32
org     0x100000 ; see USER_PROGRAM_BASE

mov al, 75; capital K code
@@:
mov byte[0xB8000], al
inc al
jmp @b