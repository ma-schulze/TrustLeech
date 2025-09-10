#include <arch.hpp>
#include <exception/mmu.hpp>

#include <exception/inject.hpp>
#include <exception/types.hpp>
#include <paging/shadow/shadow.hpp>
#include <paging/vpas.hpp>
#include <vcpu_ctx.hpp>
#include <lib/libc/stdio.hpp>

#include <dbg/utils.hpp>

namespace exception {
	namespace {
		fault_info get_fault_info(const exception_syndrome::mmu_abort_t mmu_abort_info) {
			hpfar_el2 hpfar;

			// See Linux __get_fault_info for an explanation on why this is
			// necessary
			if (!mmu_abort_info.stage_1_ptw()
					&& is_permission_fault(mmu_abort_info.status_code())) {
                hpfar = {dbg::translate_el1s1(read_far_el2().v)};
			} else {
				hpfar = read_hpfar_el2();
			}

			return { hpfar };
		}
	}

	void handle_mmu_abort(const exception_syndrome esr, exception_frame* frame) {
		(void) frame;

		assert(is_lower_el_mmu_abort(esr.exception_class()));

		const auto fault_info = get_fault_info(esr.mmu_abort);

		if (paging::vpas::should_use()) {
			// If these conditions hold, the vPAS is currently active. If an
			// abort from a lower EL was triggered, it means a guest
			// * either accessed virtually secure or out of bounds memory. In
			//   this case we must forward a synchronous external abort to the
			//   guest, since this is what the guest would have seen without
			//   TrustLeech present
			// * or the guest triggered a stage 2 fault by some other mean
			//   (e.g. an address size mismatch). In that case, we die, since this
			//   is not implemented because it is an engineering effort.
			//
			// TODO Check for unhandled abort status code early on.
			// TODO I think this should actually just take FAR_EL2

			aarch64::take_sync_external_abort(esr, faulting_address { fault_info.faulting_address() });
		} else {
			paging::shadow::handle_mmu_abort(esr, fault_info);
		}
	}
}
