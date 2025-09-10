#include "vmi_events.hpp"
#include "dbg/vmi.hpp"

namespace vmi
{

int handle_bp_event(exception_frame *frame)
{
	set_vmi_paused(true);
	// TODO get all cores to EL3
	asm volatile(
		"ldr x0, =0xc3000003\n\t" // Load the 64-bit constant 0xc3000003 into x0 using a literal pool
		"mov x1, #1\n\t" // Load 1 into x1
		"smc #0\n\t" // Issue Secure Monitor Call
	);
	receive_and_handle_command_with_response({ EVT_BREAKPOINT, 0 });
	finish_dbg_event();
	return 0;
}

int handle_ss_event(exception_frame *frame)
{
	set_vmi_paused(true);
	asm volatile(
		"ldr x0, =0xc3000003\n\t" // Load the 64-bit constant 0xc3000003 into x0 using a literal pool
		"mov x1, #1\n\t" // Load 1 into x1
		"smc #0\n\t" // Issue Secure Monitor Call
	);
	receive_and_handle_command_with_response({ EVT_STEPPING, 0 });
	finish_dbg_event();
	return 0;
}

int finish_dbg_event()
{
	asm volatile(
		"ldr x0, =0xc3000003\n\t" // Load the 64-bit constant 0xc3000003 into x0 using a literal pool
		"mov x1, #0\n\t" // Load 1 into x1
		"smc #0\n\t" // Issue Secure Monitor Call
	);
	return 0;
}

}
