org 0x7c00 ; We are loaded by BIOS at 0x7C00

jmp start

; ListFS header
align 4
fs_magic dd ?
fs_version dd ?
fs_flags dd ?
fs_base dq ?
fs_size dq ?
fs_map_base dq ?
fs_map_size dq ?
fs_first_file dq ?
fs_uid dq ?
fs_block_size dd ?
; ListFs file header
virtual at 0x800
f_info:
    f_name rb 256
    f_next dq ?
    f_prev dq ?
    f_parent dq ?
    f_flags dq ?
    f_data dq ?
    f_size dq ?
    f_ctime dq ?
    f_mtime dq ?
    f_atime dq ?
end virtual

; bootloader data
label sector_per_track word at $$ ; number of sectors per track
label head_count byte at $$ + 2 ; number disk heads
label disk_id byte at $$ + 3

boot_msg db "Hi there! My OS boot loader have started to work!",13,10,0
not_implemented_msg db "Not implemented",13,10,0

; write a string DS:SI to the monitor
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

load_sector.error:
	mov si, not_implemented_msg
	call write_str
	ret

get_disk_parameters:
	mov [disk_id], dl ; get bootable disk id
	mov ah, 0x08
	xor di, di ; ES is already zero
	push es ; we don’t need a pointer to BIOS parameters block but we need zero ES
	int 0x13
	pop es
	jc load_sector.error ; this function is not implemented for now
	inc dh ; first heads number is 0, to get the length we need increment this one
	mov [head_count], dh
	and cx, 111111b ; select low 6 bits of CX
	mov [sector_per_track], cx
	ret

start:

    cli ; Clear all Interrupts

    mov si, boot_msg
    call write_str

    hlt ; halt the system

times 510 - ($-$$) db 0 ; We have to be 512 bytes. Clear the rest of the bytes with 0

dw 0xAA55 ; Boot Signiture