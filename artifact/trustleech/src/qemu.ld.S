ENTRY(_start)

#include <arm-trusted-firmware/plat/qemu/trustleech_exp.h>

SECTIONS
{
	. = TRUSTLEECH_BASE;

	. = TRUSTLEECH_TEXT_BASE;

	.text : {
		*(.text.boot*)
        *(.text .text.*)
		*(.gnu.linkonce.t.*)
		*(.note.gnu.build-id)
	}

	. = TRUSTLEECH_TEXT_LIMIT;

	. = TRUSTLEECH_CONTEXT_STASH_BASE;

	.context-stash : {
		*(data.context-stash*)
	}

	. = TRUSTLEECH_CONTEXT_STASH_LIMIT;

	. = TRUSTLEECH_STACKS_BASE;

	.stack : {
		*(data.stack*)
	}
  	. = TRUSTLEECH_STACKS_INT_BASE;

	. = TRUSTLEECH_STACKS_INT_LIMIT;

	. = TRUSTLEECH_RODATA_BASE;

	.rodata : {
		*(.rodata*)
	}

	. = TRUSTLEECH_RODATA_LIMIT;

	. = TRUSTLEECH_RWDATA_BASE;

	.data : {
		*(.data*)
	}

	.bss : {
		*(COMMON)
		*(.bss*)
	}

	. = TRUSTLEECH_RWDATA_LIMIT;

	. = TRUSTLEECH_LIMIT;
}

