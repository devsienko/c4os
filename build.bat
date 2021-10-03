del bin\* /S /Q

if not exist "bin\" mkdir bin
mkdir bin\disk

fasm src\boot.asm bin\boot.bios.bin

tools\dd if=bin\boot.bios.bin of=bin\boot_sector.bin bs=512 count=1
tools\dd if=bin\boot.bios.bin of=bin\disk\boot.bin bs=1 skip=512

call .\kernel-build.bat

make_listfs\make_listfs of=bin\disk.img bs=512 size=2880 boot=bin\boot_sector.bin src=.\bin\disk