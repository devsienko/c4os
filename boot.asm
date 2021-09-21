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
reboot_msg db "Press any key...",13,10,0
boot_file_name db "boot.bin",0

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

reboot:
	mov si, reboot_msg
	call write_str
	xor ah, ah
	int 0x16
	jmp 0xFFFF:0

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

; critical error
error:
	pop si
	call write_str

; load sector from DX:AX to buffer from ES:DI
load_sector:
	push ax bx cx dx si
	div [sector_per_track] ; divide DX:AX by sector per track numbers (quotient will be written into AX (it's number of tracks), remainder of division into DX)
	mov cl, dl ; remainder of division is sector index, we put it into CL
	inc cl ; because first sector index is 1
	div [head_count] ; we divide quotient (from AX) by disk head numbers (in this case we divide by byte instead of word)
	mov dh, ah ; reminder of division (it’s in AH) is head number, we put it into DH
	mov ch, al ; quotient (it’s in AL) is track number (aka cylinder number) - we put it into CH
	mov dl, [disk_id] ; put disk id into DL
	mov bx, di ; put the buffer offset into BX. The segment offset of BX is already in ES
	mov al, 1 ; number of sectors for the reading - 1.
	mov si, 3 ; number of writing attempts
 @@: ; reading loop
	mov ah, 2 ; routine number
	int 0x13 ; try to read
	jnc @f ; in success case go out from function
	xor ah, ah ; reset routine number (0x00)
	int 0x13 ; reset disk system
	dec si ; decrement attempts count
	jnz @b ; if we have not spent attempts then go to begin of the loop
 .error: ; reading attempts are exceeded
	call error
	db "DISK ERROR",13,10,0
 @@:
	pop si dx cx bx ax
	ret

; find a file by name DS:SI in directory DX:AX
find_file:
	push cx dx di
 .find:
	cmp ax, -1 ; is it the end of the list?
	jne @f
	cmp dx, -1
	jne @f
 .not_found: ;if it’s the end of the list and we didn’t find the file
	call error
	db "NOT FOUND",13,10,0
 @@:
	mov di, f_info
	call load_sector ; load a sector with file info
	push di ; get file name length (DI = f_info = f_name)
	mov cx, 0xFFFF ; we don’t know string length, consider max (0xFFFF) length
	xor al, al ; 0 is the end of string sign
	repne scasb
	neg cx ; get length of the string, some binary math magic
	dec cx
	pop di
	push si ; compare file names
	repe cmpsb
	pop si
	je .found ; if the file name are equal then we return
	mov ax, word[f_next] ; load next file info sector
	mov dx, word[f_next + 2]
	jmp .find
 .found:
	pop di dx cx 
	ret

; load current file to memory to BX:0 address. 
; we load number of loaded sectors to AX
load_file_data:
	push bx cx dx si di
	mov ax, word[f_data] ; load to DX:AX index of the first file sectors list
	mov dx, word[f_data + 2]
 .load_list:
	cmp ax, -1 ; is it the end of the list?
	jne @f
	cmp dx, -1
	jne @f
 .file_end: ; file loading is done
	pop di si dx cx ; restore all registers except BX
	mov ax, bx ; remember BX
	pop bx ; restore previous BX value
	sub ax, bx ; find difference it will be the file size in 16 byte blocks
	shr ax, 9 - 4 ; translate blocks to sectors
	ret
 @@:
	mov di, f_info ; we will load the list to temp buffer
	call load_sector
	mov si, di ; SI := DI
	mov cx, 512 / 8 - 1 ; number of sectors in the list
 .load_sector:
	lodsw ; load next sector
	mov dx, [si]
	add si, 6 ; 6 because lodsw adds 2 to si + 6 = 8 byte, size of the sector number in sectors list
	cmp ax, -1 ; is it the end of the list?
	jne @f
	cmp dx, -1
	je .file_end ; if it’s the end then we return
 @@:
	push es
	mov es, bx ; load next sector
	xor di, di
	call load_sector
	add bx, 0x200 / 16 ; load the next sector further 512 bytes
	pop es
	loop .load_sector ; this command decrement CX and if CX is greater than 0 then go to  .load_sector
	lodsw ; load next list index to DX:AX
	mov dx, [si]
	jmp .load_list

load_stage2:
	; load stage 2 of the bootloader
	mov si, boot_file_name
	mov ax, word[fs_first_file]
	mov dx, word[fs_first_file + 2]
	call find_file
	mov bx, 0x7E00 / 16
	call load_file_data
	; go to stage 2 of the bootloader
	jmp stage2
start:
 
    cli ; Clear all Interrupts

	mov si, boot_msg
	call write_str

	call get_disk_parameters
	call load_stage2

	;hlt							; halt the system
	
times 510 - ($-$$) db 0			; We have to be 512 bytes. Clear the rest of the bytes with 0
 
dw 0xAA55						; Boot Signiture

; additional bootloader data
load_msg_preffix db "Loading '",0
load_msg_suffix db "'...",0
ok_msg db "OK",13,10,0
; split string from DS:SI by '/'
split_file_name:
	push si
 @@:
	lodsb
	cmp al, "/"
	je @f
	jmp @b
 @@:
	mov byte[si - 1], 0
	mov ax, si
	pop si
	ret
; load file with name DS:SI to the buffer DI:0. file size (in sectors) we return in  AX
load_file:
	push si
	mov si, load_msg_preffix
	call write_str
	pop si
	call write_str
	push si
	mov si, load_msg_suffix
	call write_str
	pop si
	push si bp
	mov dx, word[fs_first_file + 2] ; start to search with root dir
	mov ax, word[fs_first_file]
 @@:
	push ax
	call split_file_name
	mov bp, ax
	pop ax
	call find_file
	test byte[f_flags], 1 ; is it dir flag?
	jz @f
	mov si, bp
	mov dx, word[f_data + 2]
	mov ax, word[f_data]
	jmp @b	
 @@:
	call load_file_data
	mov si, ok_msg
	call write_str
	pop bp si
	ret
stage2:
	call reboot