#include <exception/head.hpp>

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
#include <dbg/dbg.hpp>

#include <lib/libc/stdio.hpp>

namespace {
	using namespace exception;

	using exception_handler = void (*)(const exception_syndrome esr, exception_frame* frame);

	void handle_unknown_exception_class(const exception_syndrome esr, exception_frame* frame) {
		(void) esr;
		(void) frame;
		die("Unknown exception class");
	}

	void handle_aarch32_exception(const exception_syndrome esr, exception_frame* frame) {
		(void) esr;
		(void) frame;
		die("AArch32 is out-of-scope");
	}

	void handle_unimplemented_feature_exception(const exception_syndrome esr, exception_frame* frame) {
		(void) esr;
		(void) frame;
		die("Exception for not yet implemented feature");
	}

	void forward_to_vel2(const exception_syndrome esr, exception_frame* frame) {
		(void) frame;
		if (vcpu().is_vel2) {
			die("Forwarding an exception to vEL2 while already in vEL2");
		}

		aarch64::take_sync_exception(EL2, esr);
	}

	void handle_hvc(const exception_syndrome esr, exception_frame* frame) {
		assert(esr.exception_class() == exception_class::hvc_aarch64);

		if (vcpu().is_vel2) {
			// Check for magic number to exit TrustLeech
			if (frame->x0 != 0xc3000001) {
				die("Trapped HVC while in vEL2");
			}

			teardown_cpu(frame);
		}

		aarch64::take_sync_exception(EL2, esr);
	}

	constexpr exception_handler get_exception_handler(const exception_syndrome esr) {
		switch (esr.exception_class()) {
            case exception_class::data_abort_same_el: // if a nested^2 guest accesses its EL2 registers VNCR_EL2 
			case exception_class::instruction_abort_lower_el:
			case exception_class::data_abort_lower_el:
				return handle_mmu_abort;
			// FEAT_FGT || FEAT_NV
			case exception_class::eret:
				return handle_eret;
			case exception_class::system:
				return handle_system;
			case exception_class::wait_for_x:
			case exception_class::floating_point_enable:
                // printf("Got the floating point enable!\n");
			// FEAT_PAUTH
			case exception_class::pauth:
				return forward_to_vel2;
			case exception_class::hvc_aarch64:
				return handle_hvc;
			case exception_class::mcr_mrc_cp15:
			case exception_class::mcrr_mrrc_cp15:
			case exception_class::mcr_mrc_cp14:
			case exception_class::ldc_stc_dbgdtrtxint:
			case exception_class::vmrs:
			case exception_class::mrrc_cp14:
			case exception_class::svc_aarch32:
			case exception_class::hvc_aarch32:
			case exception_class::smc_aarch32:
			case exception_class::floating_point_op_aarch32:
			case exception_class::bkpt_aarch32:
			case exception_class::vector_catch_aarch32:
				return handle_aarch32_exception;
			// FEAT_LS64
			case exception_class::ld64b_st64b:
			// FEAT_BTI
			case exception_class::branch_target_exception:
			// FEAT_SYSREG128 || FEAT_SYSINSTR128
			case exception_class::system_long:
			// FEAT_SVE
			case exception_class::non_streaming_sve:
			// // FEAT_TME
			case exception_class::transaction_start:
			// // FEAT_FPAC
			case exception_class::pac_fail:
			// // FEAT_SME
			case exception_class::streaming_sve_and_sme:
			// // FEAT_MOPS
			case exception_class::memory_operation:
                return forward_to_vel2;
			// // FEAT_GCS
			case exception_class::gcs:
			// // FEAT_EBEP
			case exception_class::pmu:
				return handle_unimplemented_feature_exception;
            case exception_class::breakpoint_lower_el: 
                return dbg::handle_bp;
            case exception_class::software_step_lower_el:
                return dbg::handle_ss;
			default:
				return handle_unknown_exception_class;
		}
	}

	/** An E2H switch changes how a vEL2 guest interacts with system registers.
	 * With E2H enabled, a guest writes directly to EL1 registers without
	 * traps, and expects these to map to EL2 registers. After disabling E2H,
	 * we need to update our in memory represenation of registers that would be
	 * redirected to EL2 by E2H.
	 *
	 * At the same time, toggling E2H changes the meaning of some EL2
	 * registers, requiring retranslation.
	 *
	 * Note: This check is technically insufficient as writes to HCR_EL2 are
	 * redirected to memory and not trapped. A guest could detect the
	 * afformentioned differences before the next trap to TrustLeech.
	 *
	 * This is not implemented for now.
	 */
	void check_e2h_toggle() {
		// if (unlikely(vcpu().hcr_el2().e2h() != vcpu().is_e2h)) {
		//     // Only a vEL2 guest can change the E2H bit
		//     assert(vcpu().is_vel2);

		//     if (!vcpu().hcr_el2().e2h()) {
		//         vcpu().read_viable_regs();
		//     }

		//     vcpu().write_viable_regs();

		//     vcpu().is_e2h = vcpu().hcr_el2().e2h();
		// }
	}

	/** Check if we need to toggle between vEL2 and VM mode, and perform the
	 * switch if needed
	 */
	void check_vel2_switch() {
		if (vcpu().is_vel2 == vcpu().dispatch_vel2) {
			return;
		}

		if (vcpu().dispatch_vel2) {
			vcpu().read_vm_registers();
			vcpu().write_viable_regs();

			vcpu().is_vel2 = true;

			prepare_vel2();

		} else {
			vcpu().read_viable_regs();
			vcpu().write_vm_registers();

			vcpu().is_vel2 = false;

			prepare_vm();
		}

	}

	/** Check whether vEL2 has updated CNTVOFF_EL2, and update the physical
	 * register accordingly.
	 *
	 * This is required since the offset is used for the timer condition of the
	 * virtual EL1 timer, even if it appears to be zero for read accesses (when
	 * E2H is active).
	 */
	void check_cntvoff_update() {
		if (vcpu().last_cntvoff != vcpu().cntvoff_el2().v) {
			write_cntvoff_el2(vcpu().cntvoff_el2());
			vcpu().last_cntvoff = vcpu().cntvoff_el2().v;
		}
	}

	template<typename Handler>
	void wrap_handler(Handler handler) {
      	isb();
	    dsb_sy();

        const auto cptr = read_cptr_el2();
        write_cptr_el2({0x1110000}); // enable EL2 access to all FP Stuff
                                     

        // Update breakpoint config 
        dbg::update_bp_config();

		vcpu().save_program_state();
		check_e2h_toggle();

		handler();

		check_cntvoff_update();
		check_vel2_switch();
        
	    isb();
	    dsb_sy();

		vcpu().restore_program_state();
        tlbi_alle2();

        write_cptr_el2(cptr);
	}
}

extern "C" void handle_sync_exception_aarch64(exception_frame* frame) {
	wrap_handler([frame]() {
		const auto esr = read_esr_el2();
		const auto exception_handler = get_exception_handler(esr);
		exception_handler(esr, frame);
	});
}

extern "C" void handle_irq_aarch64() {
	wrap_handler([]() {
		if (vcpu().is_vel2 || !vcpu().hcr_el2().imo()) {
			// todo("Why did we trap this IRQ?");
            return;
		}

		aarch64::take_irq(EL2);
	});
}
