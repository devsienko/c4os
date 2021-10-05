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
kernel_name db "kernel.bin",0

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
	mov di, 0x8000 / 16 ; we will load the list to temp buffer
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

	mov si, boot_msg
	call write_str

	call get_disk_parameters
	call load_stage2

	;hlt							; halt the system
	
times 510 - ($-$$) db 0			; We have to be 512 bytes. Clear the rest of the bytes with 0
 
dw 0xAA55						; Boot Signiture

include 'A20.inc'

; additional bootloader data
load_msg_preffix db "Loading '",0
load_msg_suffix db "'...",0
ok_msg db "OK",13,10,0
start32_msg db "Starting 32 bit kernel...",13,10,0
not_supported_error db "We cannot a detect disk size",13,10,0
label memory_map at 0x9000

; split string from DS:SI by '/'
split_file_name:
	push si
 @@:
	lodsb
	cmp al, "/"
	je @f
	test al, al
	jz @f
	jmp @b
 @@:
	mov byte[si - 1], 0
	mov ax, si
	pop si
	ret
; load file with name DS:SI to the buffer DI:0. file size (in sectors) we return in AX
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

get_memory_map:
	mov di, memory_map
	xor ebx, ebx
 @@:
	mov eax, 0xE820
	mov edx, 0x534D4150
	mov ecx, 24
	mov dword[di + 20], 1
	int 0x15
	jc .not_supported ; jump if carry (cf=1), it means 0x15 0xE820 function isn't supported
	add di, 24
	test ebx, ebx
	jnz @b
 @@:
	cmp di, 0x9000
	ja .ok
 .not_supported:
	mov si, not_supported_error
	call write_str
	hlt
 .ok:
	xor ax, ax
	mov cx, 24 / 2
	rep stosw
	ret

stage2:
	; load kernel
	mov si, kernel_name
	mov bx, 0x11000 / 16
	call load_file

	jmp .prepare_start32
	
.prepare_page_tables:
	; clear page tables
	xor ax, ax
	mov cx, 3 * 4096 / 2; count of words (stosw), / 2 because one word is 2 bytes
	mov di, 0x1000 ; address (stosw)
	rep stosw
	; fill the page directory that starts at 4096 (0x1000)
	mov dword[0x1000], 0x2000 + 111b ; points to first page table
	mov dword[0x1FFC], 0x3000 + 111b	; last page table record, 8188 (0x1FFC) = 4096 (page directory base) + 4096 (page directory size) - 4 (size of one page directory record)
	
	; fill the first page table
	mov eax, 111b
	mov cx, 0x100000 / 4096 ; map first 1 Mb
	mov di, 0x2000 ; address (stosd)
 @@:
 	stosd
	add eax, 0x1000
	loop @b
	
	; fill the last page table
	mov di, 0x3000
	mov eax, 0x11000
	or eax, 11b
	mov ecx, 1024 ; map whole table
  @@:
	stosd
	add eax, 0x1000
	loop @b
	mov dword[0x3FF4], 0x4000 + 11b ; Kernel stack
	mov dword[0x3FF8], 0x3000 + 11b ; Kernel page table

	; load page directory to CR3
	mov eax, 0x1000
 	mov cr3, eax
	ret

.prepare_start32:
	mov si, start32_msg
	call write_str

	call get_memory_map
	call .prepare_page_tables
 
	lgdt [gdtr_data]
	cli ; clear all Interrupts

	; enable A20	
	call EnableA20_KKbrd_Out
	
	; go to protected mode and turn on address translation (bit 31): 
	mov eax, cr0 
	or eax, 0x80000001
	mov cr0, eax

	; go to 32-bit code
	jmp 8:start32

align 16

dt_data: 
	dd 0 				; null descriptor
	dd 0 

; Offset 0x8 bytes from start of GDT: Descriptor code therfore is 8
 
; gdt code:				; code descriptor
	dw 0FFFFh 			; limit low
	dw 0 				; base low
	db 0 				; base middle
	db 10011010b 		; access
	db 11001111b 		; granularity
	db 0 				; base high
 
; Offset 16 bytes (0x10) from start of GDT. Descriptor code therfore is 0x10.
 
; gdt data:				; data descriptor
	dw 0FFFFh 			; limit low (Same as code)
	dw 0 				; base low
	db 0 				; base middle
	db 10010010b 		; access
	db 11001111b 		; granularity
	db 0				; base high

; user code segment
	dq 0x00CFFA000000FFFF

; user data segment
	dq 0x00CFF2000000FFFF

gdtr_data:
	dw $ - dt_data - 1
	dd dt_data

use32

include 'stdio.inc'

start32:
	; prepare segment registers
	mov eax, 16
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	movzx esp, sp
	
	call ClsScr
	mov	ebx, msg
	call Puts

	; put 'OK' to right bottom corner
	mov byte[0xB8000 + (25 * 80 - 1) * 2], "K"
	mov dword[0x3FFC], 0xB8000 + 111b ; map last page of last PTE to the video memory
	mov byte[0xFFFFF000 + (25 * 80 - 2) * 2], "O" ; display 'O' by above mapped memory

	; put the memory map address to ESI
	mov esi, memory_map
	; put the boot disk number to DL
	mov dl, [disk_id]

	jmp 0xFFC00000 ; jump to kernel

	hlt

msg db  10, 10, 10, "                     <OS Development Series Tutorial>", 0 