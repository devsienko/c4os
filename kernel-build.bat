fasm src\startup.asm bin\startup.o
SET OS_INCLUDE_PATH=src\include

gcc src\lib\stdlib.c -o bin\stdlib.o -c -I %OS_INCLUDE_PATH%
gcc src\kernel.c -o bin\main.o -c -I %OS_INCLUDE_PATH%
gcc src\drivers\tty.c -o bin\tty.o -c -I %OS_INCLUDE_PATH%

ld -T src\kernel.ld -o bin\disk\kernel.bin bin\startup.o bin\stdlib.o bin\main.o bin\tty.o
objcopy bin\disk\kernel.bin -O binary