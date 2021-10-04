fasm src\kernel\startup.asm bin\startup.o
fasm src\kernel\interrupts.asm bin\interrupts_asm.o

SET OS_INCLUDE_PATH=src\include

gcc src\kernel\interrupts.c -o bin\interrupts.o -c -I %OS_INCLUDE_PATH%
gcc src\lib\stdlib.c -o bin\stdlib.o -c -I %OS_INCLUDE_PATH%
gcc src\kernel\multitasking.c -o bin\multitasking.o -c -I %OS_INCLUDE_PATH%
gcc src\kernel\memory_manager.c -o bin\memory_manager.o -c -I %OS_INCLUDE_PATH%
gcc src\kernel\kernel.c -o bin\main.o -c -I %OS_INCLUDE_PATH%
gcc src\kernel\timer.c -o bin\timer.o -c -I %OS_INCLUDE_PATH%
gcc src\drivers\tty.c -o bin\tty.o -c -I %OS_INCLUDE_PATH%
gcc src\drivers\dma.c -o bin\dma.o -c -I %OS_INCLUDE_PATH%
gcc src\drivers\floppy.c -o bin\floppy.o -c -I %OS_INCLUDE_PATH%

ld -T src\kernel\kernel.ld -o bin\disk\kernel.bin bin\startup.o bin\stdlib.o bin\multitasking.o bin\memory_manager.o bin\main.o bin\timer.o bin\tty.o bin\dma.o bin\floppy.o bin\interrupts_asm.o bin\interrupts.o
objcopy bin\disk\kernel.bin -O binary