org 0x7c00 ; We are loaded by BIOS at 0x7C00

jmp start

boot_msg db "Hi there! My OS boot loader have started to work!",13,10,0

write_str:
    push ax si
    mov ah, 0x0E
 @@:
    lodsb
    test al, al
    jz @f
    int 0x10
    jmp @b
 @@:
    pop si ax
    ret

start:

    cli ; Clear all Interrupts

    mov si, boot_msg
    call write_str

    hlt ; halt the system

times 510 - ($-$$) db 0 ; We have to be 512 bytes. Clear the rest of the bytes with 0

dw 0xAA55 ; Boot Signiture