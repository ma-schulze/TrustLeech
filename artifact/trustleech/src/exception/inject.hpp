#pragma once

#include <arch.hpp>
#include <registers.hpp>

namespace exception::aarch64 {
	/** Cause the vCPU to take a synchronous exception */
	void take_sync_exception(const exception_level target_el, const exception_syndrome esr);

	/** Cause the vCPU to take an IRQ */
	void take_irq(const exception_level target_el);

	/** Cause the vCPU to take a synchronous external abort */
	void take_sync_external_abort(const exception_syndrome original_cause, const faulting_address far);

	/** Cause the vCPU take a synchronous exception, while setting FAR and
	 * HPFAR.
	 *
	 * Since HPFAR is only relevant for EL2, the exception is always taken to
	 * vEL2.
	 */
	void take_sync_exception(
		const exception_syndrome esr,
		const faulting_address far,
		const hpfar_el2 hpfar
	);
}
