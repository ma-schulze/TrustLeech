#include "eret.hpp"

#include "exception/inject.hpp"
#include "log.hpp"
#include "sysops.hpp"
#include <assert.hpp>
#include <exception/types.hpp>
#include <plat/feat.hpp>
#include <registers.hpp>
#include <vcpu_ctx.hpp>

#include <lib/libc/stdio.hpp>

#include <dbg/utils.hpp>

namespace exception {
	namespace {
		/** Check whether an ERET by a VM should trap to vEL2.
		 *
		 * Roughly modelled after `AArch64.CheckForERetTrap()`
		 */
		bool check_for_eret_trap() {
			const bool route_to_vel2 = vcpu().virtual_el() == EL1
				&& (plat::supports(feat::nv) && vcpu().hcr_el2().nv());

			if (!route_to_vel2) {
				return false;
			}

			// todo("Forward ERET trap to vEL2");
			return true;
		}

		/** Check whether this exception return is illegal
		 *
		 * Roughly modelled after `IllegalExceptionReturn()`
		 */
		bool is_illegal_exception_return(const saved_program_status spsr) {
			// (valid, target) = ELFromSPSR(spsr);
			if (spsr.is_aarch32()) {
				return true;
			}

			if (spsr.must_be_zero()) {
				return true;
			}

			if (spsr.el() == EL0 && spsr.use_sp_elx()) {
				return true;
			}

			// Check for return to higher Exception level
			// if UInt(target) > UInt(PSTATE.EL) then return TRUE;
			if (spsr.el() > vcpu().virtual_el()) {
				return true;
			}

			// ...AArch32 portion skipped...

			// Check for illegal return to EL1 when HCR.TGE is set and when either of
			// * SecureEL2 is enabled.
			// * SecureEL2 is not enabled and EL1 is in Non-secure state.
			if (spsr.el() == EL1 && vcpu().hcr_el2().tge()) {
				return true;
			}

			// Skipped...
			// if (IsFeatureImplemented(FEAT_GCS) && PSTATE.EXLOCK == '0' &&
			// PSTATE.EL == target && GetCurrentEXLOCKEN()) then
			// return TRUE;

			return false;
		}

		/** Let the vCPU perform an exception return
		 *
		 * Roughly modelled after `AArch64.ExceptionReturn()`
		 */
		void exception_return(exception_link new_pc, saved_program_status spsr) {
			// TODO FEAT_TME

			// if constexpr (plat::supports(feat::iesb)) {
			// 	const bool sync_errors = vcpu().is_vel2
			// 		? vcpu().sctlr_el2().iesb()
			// 		: vcpu().sctlr_el1().iesb();

			// 	if (sync_errors) {
			// 		todo("TakeUnmaskedPhysicalSErrorInterrupts");
			// 	}
			// }

			const bool is_illegal = is_illegal_exception_return(spsr);

			if (is_illegal) {
				spsr.set_illegal_execution_state(true);
			}

			if (vcpu().virtual_el() == EL2) {
				if (spsr.el() == EL2) {
                    // trapped ERET to kernel at vel2
					spsr.set_el(EL1);
				} else if (spsr.el() == EL1) {
                    // trapped eret at vel2 to VM at el1
					vcpu().dispatch_vel2 = false;
				} else if (spsr.el() == EL0) {
                  // if((vcpu().ich_hcr_el2().v & 0b1) ) {
                  if(read_elr_el2().v == 0xffffffc080076d90) {
                    // trapped eret at vel2 to VM at el0, FIXME!
					vcpu().dispatch_vel2 = false;
                    // printf("guest enter to el0 0x%lx\n", vcpu().ich_hcr_el2().v);
                  }
                }
			}

			vcpu().pc = new_pc.return_address;
			vcpu().cpsr = spsr;
		}
	}

	void handle_eret(const exception_syndrome esr, exception_frame* frame) {
		(void) frame;

		assert(esr.exception_class() == exception_class::eret);
		// ERET is undefined at EL0, so we want to let the guest handle this.
		// Also we only want to enter this function when executing a guest, and
		// those always operate at either EL1 or EL0.
		assert(vcpu().cpsr.el() == EL1);

		if (esr.eret.pauth_eret()) {
			todo("PAuth ERET not yet implemented");
		}

		if (!vcpu().is_vel2) {
            // TODO Check that EffectiveHCR_EL2_NVx() == '111'
            // TODO this is fucking stupid, there must be a better way
            // if(read_elr_el2().v == 0xffffffc0800121b4) {
            //     uint64_t vncr = vcpu().memory_backend.vncr_el2_ipa().v; 
            //
            //     uint64_t spsr_el1 = *(uint64_t *)(vncr + 0x160);
            //     uint64_t elr_el1 = *(uint64_t *)(vncr + 0x230);
            //
            //     LOG_INFO("NV RET TO USER with SPSR 0x%llx and PC 0x%llx\n", spsr_el1, elr_el1);
            //     vcpu().pc = {elr_el1};
            //     vcpu().cpsr = {spsr_el1};
            // } else {
		        aarch64::take_sync_exception(EL2, esr);
            // }
			return;
		}

		if (vcpu().is_vel2) {
			exception_return(vcpu().elr_el2(), vcpu().spsr_el2());
		} else {
			exception_return(vcpu().elr_el1(), vcpu().spsr_el1());
		}
	}
}
