#pragma once

#include <registers.hpp>

struct exception_frame;

namespace exception {
	struct fault_info {
		hpfar_el2 hpfar;

		constexpr uintptr_t faulting_address() const {
			return hpfar.fipa() << 12;
		}
	};

	void handle_mmu_abort(const exception_syndrome esr, exception_frame* frame);
}
