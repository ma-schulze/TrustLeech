#include <macros.S>

.globl teardown_cpu_asm
func teardown_cpu_asm
	restore_nonscratch_gpregs x0
	/* Can't use restore_scratch_gpregs, because we can't use indexing on the
	 * last ldp (since we both read and write to x0. This is UB if the register in
	 * question isn't the SP)
	 */
	ldp x18, x30, [x0], #16
	ldp x16, x17, [x0], #16
	ldp x14, x15, [x0], #16
	ldp x12, x13, [x0], #16
	ldp x10, x11, [x0], #16
	ldp x8, x9, [x0], #16
	ldp x6, x7, [x0], #16
	ldp x4, x5, [x0], #16
	ldp x2, x3, [x0], #16
	ldp x0, x1, [x0]

	smc #0
endfunc teardown_cpu_asm
