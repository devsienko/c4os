ENTRY(_start)

KERNEL_BASE = 0xFFC00000;

SECTIONS {
 	.text KERNEL_BASE : {
		_KERNEL_CODE_BASE = .;
		*(.text)
		*(.code)
		*(.rodata*)
	}
	.data ALIGN(0x1000) : {
		_KERNEL_DATA_BASE = .;
		*(.data)
  	}
	.bss ALIGN(0x1000) : {
		_KERNEL_BSS_BASE = .;
		*(.bss)
	}
	.empty ALIGN(0x1000) - 1 : {
		BYTE(0)
		_KERNEL_END = .;
	}
}