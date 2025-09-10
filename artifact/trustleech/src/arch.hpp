#pragma once

#include <types.hpp>

enum class exception_level {
	EL0 = 0b00,
	EL1 = 0b01,
	EL2 = 0b10,
	EL3 = 0b11,
};

using exception_level::EL0;
using exception_level::EL1;
using exception_level::EL2;
using exception_level::EL3;

/** Trap mode for E2H floating point controls in CPTR_EL2 */
enum class fp_trap {
	// This control causes execution of these instructions at EL2, EL1, and EL0
	// to be trapped.
	trap_el1_el0_1 = 0b00,
	// When HCR_EL2.TGE is 0, this control does not cause execution of any
	// instructions to be trapped.
	//
	// When HCR_EL2.TGE is 1, this control causes execution of these
	// instructions at EL0 to be trapped, but does not cause execution of any
	// instructions at EL2 to be trapped.
	trap_el0 = 0b01,
	// This control causes execution of these instructions at EL2, EL1, and EL0
	// to be trapped.
	trap_el1_el0_2 = 0b10,
	// This control does not cause execution of any instructions to be trapped.
	trap_none = 0b11,
};

/** Register encoding */
enum class reg_enc : uint8_t {
	x0 = 0,
	x1 = 1,
	x2 = 2,
	x3 = 3,
	x4 = 4,
	x5 = 5,
	x6 = 6,
	x7 = 7,
	x8 = 8,
	x9 = 9,
	x10 = 10,
	x11 = 11,
	x12 = 12,
	x13 = 13,
	x14 = 14,
	x15 = 15,
	x16 = 16,
	x17 = 17,
	x18 = 18,
	x19 = 19,
	x20 = 20,
	x21 = 21,
	x22 = 22,
	x23 = 23,
	x24 = 24,
	x25 = 25,
	x26 = 26,
	x27 = 27,
	x28 = 28,
	x29 = 29,
	x30 = 30,
	xzr = 31,
	sp = 31,
};
