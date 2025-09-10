#include <macros.S>

.section .text.boot
.globl _start
.extern setup_cpu
.extern timer_interrupt

_start:
	save_scratch_gpregs
    mrs x0, sctlr_el2 
    and x1, x0, #1
    cbnz x1, .Ltimer_int // If x1 is zero, continue execution at 'continue'
	bl setup_cpu
    b .Lend
.Ltimer_int:
    save_nonscratch_gpregs
    bl handle_timer_interrupt
    restore_nonscratch_gpregs
  	restore_scratch_gpregs
    movz x0, #0x0002               
    movk x0, #0xc300, lsl #16    
    smc #0 

.Lend:
	restore_scratch_gpregs
	eret
