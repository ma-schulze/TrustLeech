#include <macros.S>

.extern handle_sync_exception_aarch64
.extern handle_irq_aarch64

/*
 * Declare the exception vector table, enforcing it is aligned on a
 * 2KB boundary, as required by the ARMv8 architecture.
 * Use zero bytes as the fill value to be stored in the padding bytes
 * so that it inserts illegal AArch64 instructions. This increases
 * security, robustness and potentially facilitates debugging.
 */
.macro vector_base  label
	.section .text, "ax"
	.align 11, 0
	\label:
.endm


/*
 * Create an entry in the exception vector table, enforcing it is
 * aligned on a 128-byte boundary, as required by the ARMv8 architecture.
 * Use zero bytes as the fill value to be stored in the padding bytes
 * so that it inserts illegal AArch64 instructions. This increases
 * security, robustness and potentially facilitates debugging.
 */
.macro vector_entry  label
	.cfi_sections .debug_frame
	.align 7, 0
	.type \label, %function
	.cfi_startproc
	\label:
.endm

/*
 * Add the bytes until fill the full exception vector, whose size is always
 * 32 instructions. If there are more than 32 instructions in the
 * exception vector then an error is emitted.
 */
.macro end_vector_entry label
	.cfi_endproc
	.if (\label + (32 * 4) - .) < 0
		.error "Exception vector is too large"
	.else
		.fill (\label + (32 * 4) - .)
	.endif
.endm

.globl exception_vectors
vector_base exception_vectors
	/* ---------------------------------------------------------------------
	 * Current EL with SP_EL0 : 0x0 - 0x200
	 *
	 * The code is non-reentrant, so we just spin here
	 * ---------------------------------------------------------------------
	 */
	vector_entry sync_exception_sp_el_0
		b .
	end_vector_entry sync_exception_sp_el_0

	vector_entry irq_sp_el_0
		b .
	end_vector_entry irq_sp_el_0

	vector_entry fiq_sp_el_0
		b .
	end_vector_entry fiq_sp_el_0

	vector_entry serror_sp_el_0
		b .
	end_vector_entry serror_sp_el_0

	/* ---------------------------------------------------------------------
	 * Current EL with SP_ELx: 0x200 - 0x400
	 *
	 * The code is non-reentrant, so we just spin here
	 * ---------------------------------------------------------------------
	 */
	vector_entry sync_exception_sp_el_x
		b .
	end_vector_entry sync_exception_sp_el_x

	vector_entry irq_sp_el_x
		b .
	end_vector_entry irq_sp_el_x

	vector_entry fiq_sp_el_x
		b .
	end_vector_entry fiq_sp_el_x

	vector_entry serror_sp_el_x
		b .
	end_vector_entry serror_sp_el_x

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch64 : 0x400 - 0x600
	 * ---------------------------------------------------------------------
	 */
	vector_entry sync_exception_aarch64
		save_scratch_gpregs
		/* We can now use x0 - x18 since they have been saved */

		/* Check whether we need access to all saved registers in our handler,
		 * based on the exception class
		 */
		.equ ESR_ELx_EC_SHIFT, 26
		.equ ESR_ELx_EC_WIDTH, 6
		mrs x0, ESR_EL2
		ubfx x0, x0, ESR_ELx_EC_SHIFT, ESR_ELx_EC_WIDTH

		.equ ESR_ELx_EC_SYSTEM, 0b011000
		mov x1, ESR_ELx_EC_SYSTEM
		cmp x0, x1
		b.eq sync_exception_aarch64_extended

		.equ ESR_ELx_EC_HVC64, 0b010110
		mov x1, ESR_ELx_EC_HVC64
		cmp x0, x1
		b.eq sync_exception_aarch64_extended

		/* No register access needed - pass a nullptr to the handler */
		/* mov x0, xzr */ 
        mov x0, sp
        sub x0, x0, 96
		bl handle_sync_exception_aarch64

	/* The extended handler returns here to the shared code */
	sync_exception_aarch64.restore_state:
		restore_scratch_gpregs

		eret
	end_vector_entry sync_exception_aarch64

	vector_entry irq_aarch64
		save_scratch_gpregs

		bl handle_irq_aarch64

		restore_scratch_gpregs
		eret
	end_vector_entry irq_aarch64

	vector_entry fiq_aarch64
		b .
	end_vector_entry fiq_aarch64

	vector_entry serror_aarch64
		b .
	end_vector_entry serror_aarch64

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch32 : 0x600 - 0x800
	 *
	 * We do not support AArch32 for now, so we just spin here
	 * ---------------------------------------------------------------------
	 */
	vector_entry sync_exception_aarch32
		b .
	end_vector_entry sync_exception_aarch32

	vector_entry irq_aarch32
		b .
	end_vector_entry irq_aarch32

	vector_entry fiq_aarch32
		b .
	end_vector_entry fiq_aarch32

	vector_entry serror_aarch32
		b .
	end_vector_entry serror_aarch32

.cfi_startproc
sync_exception_aarch64_extended:
	save_nonscratch_gpregs

	/* Pass pointer to struct of saved registers to the handler */
	mov x0, sp
	bl handle_sync_exception_aarch64

	restore_nonscratch_gpregs
	b sync_exception_aarch64.restore_state
.cfi_endproc
