fasm src\kernel\startup.asm bin\startup.o

SET OS_INCLUDE_PATH=src\include

gcc src\kernel\interrupts.c -o bin\interrupts.o -c -I %OS_INCLUDE_PATH%
gcc src\lib\stdlib.c -o bin\stdlib.o -c -I %OS_INCLUDE_PATH%
gcc src\kernel\kernel.c -o bin\main.o -c -I %OS_INCLUDE_PATH%
gcc src\drivers\tty.c -o bin\tty.o -c -I %OS_INCLUDE_PATH%

ld -T src\kernel\kernel.ld -o bin\disk\kernel.bin bin\startup.o bin\stdlib.o bin\main.o bin\tty.o bin\interrupts.o
objcopy bin\disk\kernel.bin -O binary