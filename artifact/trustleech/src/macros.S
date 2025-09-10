#pragma once

.macro save_scratch_gpregs
	stp x0, x1, [sp, #-16]!
	stp x2, x3, [sp, #-16]!
	stp x4, x5, [sp, #-16]!
	stp x6, x7, [sp, #-16]!
	stp x8, x9, [sp, #-16]!
	stp x10, x11, [sp, #-16]!
	stp x12, x13, [sp, #-16]!
	stp x14, x15, [sp, #-16]!
	stp x16, x17, [sp, #-16]!
	stp x18, x30, [sp, #-16]!
.endm

.macro restore_scratch_gpregs
	ldp x18, x30, [sp], #16
	ldp x16, x17, [sp], #16
	ldp x14, x15, [sp], #16
	ldp x12, x13, [sp], #16
	ldp x10, x11, [sp], #16
	ldp x8, x9, [sp], #16
	ldp x6, x7, [sp], #16
	ldp x4, x5, [sp], #16
	ldp x2, x3, [sp], #16
	ldp x0, x1, [sp], #16
.endm

.macro save_nonscratch_gpregs
	stp x19, x20, [sp, #-16]!
	stp x21, x22, [sp, #-16]!
	stp x23, x24, [sp, #-16]!
	stp x25, x26, [sp, #-16]!
	stp x27, x28, [sp, #-16]!
	stp x29, xzr, [sp, #-16]!
.endm

.macro restore_nonscratch_gpregs ctx_ptr=sp
	ldp x29, xzr, [\ctx_ptr], #16
	ldp x27, x28, [\ctx_ptr], #16
	ldp x25, x26, [\ctx_ptr], #16
	ldp x23, x24, [\ctx_ptr], #16
	ldp x21, x22, [\ctx_ptr], #16
	ldp x19, x20, [\ctx_ptr], #16
.endm

/*
 * This macro is used to create a function label and place the
 * code into a separate text section based on the function name
 * to enable elimination of unused code during linking. It also adds
 * basic debug information to enable call stack printing most of the
 * time. The optional _align parameter can be used to force a
 * non-standard alignment (indicated in powers of 2). The default is
 * _align=2 because both Aarch32 and Aarch64 instructions must be
 * word aligned. Do *not* try to use a raw .align directive. Since func
 * switches to a new section, this would not have the desired effect.
 */
.macro func _name, _align=2
	/*
	 * Add Call Frame Information entry in the .debug_frame section for
	 * debugger consumption. This enables callstack printing in debuggers.
	 * This does not use any space in the final loaded binary, only in the
	 * ELF file.
	 * Note that a function manipulating the CFA pointer location (i.e. the
	 * x29 frame pointer on AArch64) should declare it using the
	 * appropriate .cfi* directives, or be prepared to have a degraded
	 * debugging experience.
	 */
	.cfi_sections .debug_frame
	.section .text.asm.\_name, "ax"
	.type \_name, %function
	/*
	 * .cfi_startproc and .cfi_endproc are needed to output entries in
	 * .debug_frame
	 */
	.cfi_startproc
	.align \_align
	\_name:
.endm

/*
 * This macro is used to mark the end of a function.
 */
.macro endfunc _name
	.cfi_endproc
	.size \_name, . - \_name
.endm
