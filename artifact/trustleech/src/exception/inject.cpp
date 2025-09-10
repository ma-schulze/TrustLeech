#include <exception/inject.hpp>

#include <vcpu_ctx.hpp>
#include <plat/feat.hpp>

#include <lib/libc/stdio.hpp>

namespace exception::aarch64 {
	namespace {
		enum class exception_origin : size_t {
			current_el_sp_el0 = 0x0,
			current_el_sp_elx = 0x200,
			lower_el_aarch64 = 0x400,
			lower_el_aarch32 = 0x600,
		};

		enum class exception_type : size_t {
			sync   = 0x0,
			irq    = 0x80,
			fiq    = 0x100,
			serror = 0x180,
		};

		exception_origin get_exception_origin(const exception_level target_el) {
			if (vcpu().cpsr.is_aarch32()) {
				return exception_origin::lower_el_aarch32;
			}

			assert(target_el >= vcpu().virtual_el());

			if (vcpu().virtual_el() < target_el) {
				return exception_origin::lower_el_aarch64;
			}

			return vcpu().cpsr.use_sp_elx()
				? exception_origin::current_el_sp_elx
				: exception_origin::current_el_sp_el0;
		}

		constexpr saved_program_status get_new_cpsr(const exception_level target_el, const union sctlr sctlr, const saved_program_status old_cpsr) {
			saved_program_status new_cpsr { old_cpsr };

			// We always return to EL1 when injecting an exception
			new_cpsr.set_el(EL1);

			// PSTATE.nRW = '0';
			new_cpsr.set_is_aarch32(false);
			// PSTATE.SP = '1';
			new_cpsr.set_use_sp_elx(true);
			// PSTATE.SS = '0';
			new_cpsr.set_software_step(false);

			// PSTATE.<D,A,I,F> = '1111';
			new_cpsr.set_fiq_mask(true);
			new_cpsr.set_irq_mask(true);
			new_cpsr.set_serror_mask(true);
			new_cpsr.set_debug_mask(true);

			// PSTATE.IL = '0';
			new_cpsr.set_illegal_execution_state(false);

			// if (IsFeatureImplemented(FEAT_PAN) && (PSTATE.EL == EL1 ||
			//       (PSTATE.EL == EL2 && ELIsInHost(EL0))) &&
			//       SCTLR_ELx[].SPAN == '0') then
			//     PSTATE.PAN = '1';
			if (plat::supports(feat::pan)
					&& (target_el == EL1
							|| (target_el == EL2 && vcpu().hcr_el2().e2h() && vcpu().hcr_el2().tge()))
					&& !sctlr.span()) {
				new_cpsr.set_privileged_access_never(true);
			}

			// if IsFeatureImplemented(FEAT_UAO) then PSTATE.UAO = '0';
			if constexpr (plat::supports(feat::uao)) {
				new_cpsr.set_user_access_override(false);
			}

			// if IsFeatureImplemented(FEAT_BTI) then PSTATE.BTYPE = '00';
			if constexpr (plat::supports(feat::bti)) {
				new_cpsr.set_branch_type_indicator(0b00);
			}

			// if IsFeatureImplemented(FEAT_SSBS) then PSTATE.SSBS = SCTLR_ELx[].DSSBS;
			if constexpr (plat::supports(feat::ssbs)) {
				new_cpsr.set_speculative_store_bypass(sctlr.dssbs());
			}

			// if IsFeatureImplemented(FEAT_MTE) then PSTATE.TCO = '1';
			if constexpr (plat::supports(feat::mte)) {
				new_cpsr.set_tag_check_override(true);
			}

			return new_cpsr;
		}

		/** Cause the vcpu_ctx to take an exception
		 *
		 *  This is roughly modelled after the ARMv8-A ARM DDI 0487K.a
		 *  `aarch64/exceptions/takeexception/AArch64.TakeException`
		 *  pseudocode function
		 */
		void take_exception(exception_level target_el, const exception_type type) {
			assert(target_el == EL1 || target_el == EL2);

			const auto vbar = target_el == EL2
				? vcpu().vbar_el2()
				: vcpu().vbar_el1();

			const auto new_cpsr = get_new_cpsr(
				target_el,
				target_el == EL2 ? vcpu().sctlr_el2() : vcpu().sctlr_el1(),
				vcpu().cpsr
			);
			const auto spsr = vcpu().cpsr.with_el(vcpu().virtual_el());

			if (target_el == EL2) {
				vcpu().set_elr_el2(exception_link { vcpu().pc });
				vcpu().set_spsr_el2(spsr);
			} else {
				vcpu().set_elr_el1(exception_link { vcpu().pc });
				vcpu().set_spsr_el1(spsr);
			}

			vcpu().dispatch_vel2 = target_el == EL2;
			vcpu().pc = vbar.address + static_cast<size_t>(get_exception_origin(target_el)) + static_cast<size_t>(type);
			vcpu().cpsr = new_cpsr;
            if((new_cpsr.v & 0b1111) == 0) {
              printf("this is not allowed ._.\n");
            }
		}

		exception_syndrome sync_external_abort_syndrome(const exception_syndrome original_cause) {
			assert(is_lower_el_mmu_abort(original_cause.exception_class()));

			exception_syndrome new_esr {};

			if (original_cause.instruction_length_32bit()) {
				new_esr.set_instruction_length_32bit(true);
			}

			// TODO(transparency)
			// - Check kvm_handle_guest_abort
			// - Update for vPAS faults during stage 1 table walks
			const mmu_abort_status_code derived_status_code = mmu_abort_status_code::sync_external_abort_nowalk;

			new_esr.mmu_abort.set_status_code(derived_status_code);

			const bool from_el0 = vcpu().cpsr.el() == EL0;
			if (original_cause.exception_class() == exception_class::instruction_abort_lower_el) {
				new_esr.set_exception_class(from_el0
					? exception_class::instruction_abort_lower_el : exception_class::instruction_abort_same_el);
			} else {
				new_esr.set_exception_class(from_el0
					? exception_class::data_abort_lower_el : exception_class::data_abort_same_el);

				new_esr.data_abort.set_write_not_read(original_cause.data_abort.write_not_read());
				new_esr.data_abort.set_cache_maintenance(original_cause.data_abort.cache_maintenance());
			}

			return new_esr;
		}
	}

	void take_sync_exception(const exception_level target_el, const exception_syndrome esr) {
		if (target_el == EL2) {
			vcpu().set_esr_el2(esr);
		} else {
			vcpu().set_esr_el1(esr);
		}

		return take_exception(target_el, exception_type::sync);
	}

	void take_irq(const exception_level target_el) {
		return take_exception(target_el, exception_type::irq);
	}

	void take_sync_external_abort(const exception_syndrome original_cause, const faulting_address far) {
		const auto new_esr = sync_external_abort_syndrome(original_cause);

		const auto target_el = vcpu().is_vel2
			? EL2 : EL1;

		if (target_el == EL2) {
			vcpu().set_far_el2(far);
		} else {
			vcpu().set_far_el1(far);
		}

		return take_sync_exception(target_el, new_esr);
	}

	void take_sync_exception(
		const exception_syndrome esr,
		const faulting_address far,
		const hpfar_el2 hpfar
	) {
		vcpu().set_far_el2(far);
		vcpu().set_hpfar_el2(hpfar);

		take_sync_exception(EL2, esr);
	}
}
