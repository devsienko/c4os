ENTRY(_start)

KERNEL_BASE = 0xFFC00000;

SECTIONS {
 	.text KERNEL_BASE : {
		*(.text)
		*(.code)
		*(.rodata*)
	}
	.data ALIGN(0x1000) : {
		*(.data)
  	}
	.bss ALIGN(0x1000) : {
		*(.bss)
	}
	.empty ALIGN(0x1000) - 1 : {
		BYTE(0)
	}
}