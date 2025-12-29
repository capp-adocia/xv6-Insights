/* JOS 内核的简单链接脚本。
   查看 GNU ld 'info' 手册（"info ld"）以了解语法。 */

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
	/* 将内核链接到此地址："." 表示当前地址 */
        /* Must be equal to KERNLINK */
	. = 0x80100000;

	.text : AT(0x100000) {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}

	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	/* 在内核内存中包含调试信息 */
	.stab : {
		PROVIDE(__STAB_BEGIN__ = .);
		*(.stab);
		PROVIDE(__STAB_END__ = .);
	}

	.stabstr : {
		PROVIDE(__STABSTR_BEGIN__ = .);
		*(.stabstr);
		PROVIDE(__STABSTR_END__ = .);
	}

	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);

	/* 传统上，Unix 链接器在文本段、数据段和 BSS 段的末尾提供伪符号 etext、edata 和 end。
	/* 对于内核映射，我们需要数据段开始的地址，但那不是传统符号之一，因为在文本段和数据段之间有只读的 rodata 段之前，这种惯例就已经存在了。 */
	PROVIDE(data = .);

	/* The data segment */
	.data : {
		*(.data)
	}

	PROVIDE(edata = .);

	.bss : {
		*(.bss)
	}

	PROVIDE(end = .);

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}
