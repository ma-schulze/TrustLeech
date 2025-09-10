#include "arm-trusted-firmware/plat/qemu/trustleech_exp.h"
#include "dbg/vmi_events.hpp"
#include "utility.hpp"
#include <dbg/dbg.hpp>
#include <dbg/utils.hpp>

#include <arch.hpp>
#include <assert.hpp>
#include <exception/eret.hpp>
#include <exception/inject.hpp>
#include <exception/mmu.hpp>
#include <exception/system.hpp>
#include <exception/types.hpp>
#include <exec_vel2.hpp>
#include <exec_vm.hpp>
#include <plat/feat.hpp>
#include <registers.hpp>
#include <tail.hpp>
#include <vcpu_ctx.hpp>
#include <sysops.hpp>

#include <lib/libc/stdio.hpp>

#define TEST_CASE 0 // 1 -> full dump, 2 -> crypto, 3 -> packer

namespace dbg
{

#if TEST_CASE == 1
void tc_handle_bp(const exception_syndrome esr, exception_frame *frame)
{
}

void tc_handle_ss(const exception_syndrome esr, exception_frame *frame)
{
}

void tc_init_bp()
{
	auto this_core = []() {
		uint64_t v = read_mpidr_el1().v;
		v &= static_cast<uint64_t>(0xff);
		return v;
	}();

	if (this_core == 0) {
		size_t ram_start = GiB(1);
		size_t ram_end = TRUSTLEECH_PLAT_RAM_LIMIT;
		// We just make a full memory dump.
		while (ram_start < ram_end) {
			// printf("%llx ", *(uint64_t *)ram_start);
			ram_start += 8;
		}
		// printf("Done!\n");
	}
	while (true) {
	}
}
#elif TEST_CASE == 2
void tc_init_bp()
{
	write_dbgbvr0_el1({ 0x4006e0 });
	write_dbgbcr0_el1(dbgbcr_n_el1{}
				  .with_e(1)
				  .with_ssc(0b0)
				  .with_pmc(0b11)
				  .with_hmc(0b1)
				  .with_bas(0b1111));

	// printf("Setting BP to 0x%lx and CTF 0x%lx\n", read_dbgbvr0_el1().v,
	       read_dbgbcr0_el1().v);
}

void tc_handle_ss(const exception_syndrome esr, exception_frame *frame)
{
}

void tc_handle_bp(const exception_syndrome esr, exception_frame *frame)
{
	// printf("Reached bp at 0x%lx\n", read_elr_el2());
	uint64_t x0 = frame->at(reg_enc::x0);
	// printf("Got x: 0x%lx\n", x0);
	uint64_t x0_phys = translate_el0(x0);
	// printf("Got phys x: 0x%lx\n", x0_phys);

	// printf("Found key: %15s\n", (char *)x0_phys);

	// repeat instr
	write_dbgbvr0_el1({ 0x0 });
	vcpu().pc -= esr.instruction_length_32bit() ? 4 : 2;
}

#elif TEST_CASE == 3

void tc_handle_bp(const exception_syndrome esr, exception_frame *frame)
{
	// printf("Reached bp at 0x%lx\n", read_elr_el2());
	if (read_elr_el2().v == 0x400774) {
		uint64_t x1 = frame->at(reg_enc::x1);
		// printf("Got x1: 0x%lx\n", x1);

		write_dbgbvr0_el1({ x1 + 80 });
	} else {
		uint64_t x0 = frame->at(reg_enc::x0);
		// printf("Got key: 0x%lx\n", x0);

		write_dbgbvr0_el1({ 0x0 });
	}
	vcpu().pc -= esr.instruction_length_32bit() ? 4 : 2;
}

void tc_handle_ss(const exception_syndrome esr, exception_frame *frame)
{
}

void tc_init_bp()
{
	write_dbgbvr0_el1({ 0x400774 });
	write_dbgbcr0_el1(dbgbcr_n_el1{}
				  .with_e(1)
				  .with_ssc(0b0)
				  .with_pmc(0b11)
				  .with_hmc(0b1)
				  .with_bas(0b1111));

	// printf("Setting BP to 0x%lx and CTF 0x%lx\n", read_dbgbvr0_el1().v,
	       read_dbgbcr0_el1().v);
}

#else

void tc_handle_bp(const exception_syndrome esr, exception_frame *frame)
{
}

void tc_handle_ss(const exception_syndrome esr, exception_frame *frame)
{
}

void tc_init_bp()
{
}

#endif

void init_dbg()
{
	// // printf("Initialising Breakpoint Framework!\n");
	auto v = read_mdcr_el2().with_tde(1).with_tda(1);
	write_mdcr_el2(v);
	vcpu().set_mdcr_el2(v);
	write_mdscr_el1(mdscr_el1{}.with_kde(1).with_mde(1));

	write_dbgbvr0_el1({ 0x4006e0 });
	write_dbgbcr0_el1(dbgbcr_n_el1{}
				  .with_e(1)
				  .with_ssc(0b0)
				  .with_pmc(0b11)
				  .with_hmc(0b1)
				  .with_bas(0b1111));

	// printf("Setting BP to 0x%lx and CTF 0x%lx\n", read_dbgbvr0_el1().v,
	 //      read_dbgbcr0_el1().v);

	tc_init_bp();
}

void handle_bp(const exception_syndrome esr, exception_frame *frame)
{
	// tc_handle_bp(esr, frame);

	vmi::handle_bp_event(frame);

	// vcpu().pc += esr.instruction_length_32bit() ? 4 : 2;
}
void handle_ss(const exception_syndrome esr, exception_frame *frame)
{
	// tc_handle_ss(esr, frame);
	vmi::handle_ss_event(frame);

	// vcpu().pc += esr.instruction_length_32bit() ? 4 : 2;
}

static dbgbcr_n_el1 dbgbcr0 = { 0 };
static dbgbvr_n_el1 dbgbvr0 = { 0 };

void set_dbgbcr0_el1(uint64_t val)
{
	dbgbcr0 = { val };
}

void set_dbgbvr0_el1(uint64_t val)
{
	dbgbvr0 = { val };
}

void update_bp_config()
{
	write_dbgbcr0_el1({ dbgbcr0 });
	write_dbgbvr0_el1({ dbgbvr0 });
}

}
