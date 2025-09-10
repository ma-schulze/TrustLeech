#pragma once

#include <arch.hpp>
#include <assert.hpp>
#include <types.hpp>

enum class exception_class : uint8_t {
	/** Unknown reason. */
	unknown = 0b000000,
	/** Trapped WF* instruction execution.
	 *
	 * Conditional WF* instructions that fail their condition code check do not
	 * cause an exception.
	 */
	wait_for_x = 0b000001,
	/** Trapped MCR or MRC access with (coproc==`0b1111`) that is not reported
	 * using exception class `0b000000`.
	 *
	 * When AArch32 is supported
	 */
	mcr_mrc_cp15 = 0b000011,
	/** Trapped MCRR or MRRC access with (coproc==`0b1111`) that is not reported
	 * using EC `0b000000`.
	 *
	 * When AArch32 is supported
	 */
	mcrr_mrrc_cp15 = 0b000100,
	/** Trapped MCR or MRC access with (coproc==`0b1110`).
	 *
	 * When AArch32 is supported
	 */
	mcr_mrc_cp14 = 0b000101,
	/** Trapped LDC or STC access.
	 *
	 * The only architected uses of these instruction are:
	 * - An STC to write data to memory from DBGDTRRXint.
	 * - An LDC to read data from memory to DBGDTRTXint.
	 *
	 * When AArch32 is supported
	 */
	ldc_stc_dbgdtrtxint = 0b000110,
	/** Access to SME, SVE, Advanced SIMD or floating-point functionality
	 * trapped by CPACR_EL1.FPEN, CPTR_EL2.FPEN, CPTR_EL2.TFP, or CPTR_EL3.TFP
	 * control.
	 *
	 * Excludes exceptions resulting from CPACR_EL1 when the value of
	 * HCR_EL2.TGE is 1, or because SVE or Advanced SIMD and floating-point are
	 * not implemented. These are reported with EC value `0b000000`.
	 */
	floating_point_enable = 0b000111,
	/** Trapped VMRS access, from ID group trap, that is not reported using EC
	 * `0b000111`.
	 *
	 * When AArch32 is supported
	 */
	vmrs = 0b001000,
	/** Trapped use of a Pointer authentication instruction because
	 * `HCR_EL2.API == 0 || SCR_EL3.API == 0`.
	 *
	 * When FEAT_PAuth is implemented
	 */
	pauth = 0b001001,
	/** An exception from an LD64B or ST64B* instruction.
	 *
	 * When FEAT_LS64 is implemented
	 */
	ld64b_st64b = 0b001010,
	/** Trapped MRRC access with (coproc==`0b1110`).
	 *
	 * When AArch32 is supported
	 */
	mrrc_cp14 = 0b001100,
	/** Branch Target Exception.
	 *
	 * When FEAT_BTI is implemented
	 */
	branch_target_exception = 0b001101,
	/** Illegal Execution state. */
	illegal_execution_state = 0b001110,
	/** SVC instruction execution in AArch32 state.
	 *
	 * This is reported in ESR_EL2 only when the exception is generated because the
	 * value of HCR_EL2.TGE is 1.
	 *
	 * When AArch32 is supported
	 */
	svc_aarch32 = 0b010001,
	/** HVC instruction execution in AArch32 state, when HVC is not disabled.
	 *
	 * When AArch32 is supported
	 */
	hvc_aarch32 = 0b010010,
	/** SMC instruction execution in AArch32 state, when SMC is not disabled.
	 *
	 * This is reported in ESR_EL2 only when the exception is generated because the
	 * value of HCR_EL2.TSC is 1.
	 *
	 * When AArch32 is supported
	 */
	smc_aarch32 = 0b010011,
	/** Trapped MSRR, MRRS or System instruction execution in AArch64 state, that
	 * is not reported using EC `0b000000`.
	 *
	 * When FEAT_SYSREG128 is implemented or FEAT_SYSINSTR128 is implemented
	 */
	system_long = 0b010100,
	/** SVC instruction execution in AArch64 state.
	 *
	 * When AArch64 is supported
	 */
	svc_aarch64 = 0b010101,
	/** HVC instruction execution in AArch64 state, when HVC is not disabled.
	 *
	 * When AArch64 is supported
	 */
	hvc_aarch64 = 0b010110,
	/** SMC instruction execution in AArch64 state, when SMC is not disabled.
	 *
	 * This is reported in ESR_EL2 only when the exception is generated because the
	 * value of HCR_EL2.TSC is 1.
	 *
	 * When AArch64 is supported
	 */
	smc_aarch64 = 0b010111,
	/** Trapped MSR, MRS or System instruction execution in AArch64 state, that is
	 * not reported using EC `0b000000`, `0b000001` or `0b000111`.
	 *
	 * This includes all instructions that cause exceptions that are part of the
	 * encoding space defined in 'System instruction class encoding overview',
	 * except for those exceptions reported using EC values 0b000000, 0b000001, or
	 * 0b000111.
	 *
	 * When AArch64 is supported
	 */
	system = 0b011000,
	/** Access to SVE functionality trapped as a result of CPACR_EL1.ZEN,
	 * CPTR_EL2.ZEN, CPTR_EL2.TZ, or CPTR_EL3.EZ, that is not reported using EC
	 * `0b000000`.
	 *
	 * When FEAT_SVE is implemented
	 */
	non_streaming_sve = 0b011001,
	/** Trapped ERET, ERETAA, or ERETAB instruction execution.
	 *
	 * When FEAT_FGT is implemented or FEAT_NV is implemented
	 */
	eret = 0b011010,
	/** Exception from an access to a TSTART instruction at EL0 when
	 * `SCTLR_EL1.TME0 == 0`, EL0 when `SCTLR_EL2.TME0 == 0`, at EL1 when
	 * `SCTLR_EL1.TME == 0`, at EL2 when `SCTLR_EL2.TME == 0` or at EL3 when
	 * `SCTLR_EL3.TME == 0`.
	 *
	 * When FEAT_TME is implemented
	 */
	transaction_start = 0b011011,
	/** Exception from a PAC Fail
	 *
	 * When FEAT_FPAC is implemented
	 */
	pac_fail = 0b011100,
	/** Access to SME functionality trapped as a result of CPACR_EL1.SMEN,
	 * CPTR_EL2.SMEN, CPTR_EL2.TSM, CPTR_EL3.ESM, or an attempted execution of an
	 * instruction that is illegal because of the value of PSTATE.SM or PSTATE.ZA,
	 * that is not reported using EC `0b000000`.
	 *
	 *
	 * When FEAT_SME is implemented
	 */
	streaming_sve_and_sme = 0b011101,
	/** Instruction Abort from a lower Exception level.
	 *
	 * Used for MMU faults generated by instruction accesses and synchronous
	 * External aborts, including synchronous parity or ECC errors. Not used for
	 * debug-related exceptions.
	 */
	instruction_abort_lower_el = 0b100000,
	/** Instruction Abort taken without a change in Exception level.
	 *
	 * Used for MMU faults generated by instruction accesses and synchronous
	 * External aborts, including synchronous parity or ECC errors. Not used for
	 * debug-related exceptions.
	 */
	instruction_abort_same_el = 0b100001,
	/** PC alignment fault exception. */
	pc_alignment_fault = 0b100010,
	/** Data Abort exception from a lower Exception level, excluding Data Abort
	 * exceptions taken to EL2 as a result of accesses generated associated with
	 * VNCR_EL2 as part of nested virtualization support.
	 *
	 * These Data Abort exceptions might be generated from Exception levels in any
	 * Execution state.
	 *
	 * Used for MMU faults generated by data accesses, alignment faults other than
	 * those caused by Stack Pointer misalignment, and synchronous External aborts,
	 * including synchronous parity or ECC errors. Not used for debug-related
	 * exceptions.
	 */
	data_abort_lower_el = 0b100100,
	/** Data Abort exception without a change in Exception level, or Data Abort
	 * exceptions taken to EL2 as a result of accesses generated associated with
	 * VNCR_EL2 as part of nested virtualization support.
	 *
	 * Used for MMU faults generated by data accesses, alignment faults other than
	 * those caused by Stack Pointer misalignment, and synchronous External aborts,
	 * including synchronous parity or ECC errors. Not used for debug-related
	 * exceptions.
	 */
	data_abort_same_el = 0b100101,
	/** SP alignment fault exception. */
	sp_alignment_fault = 0b100110,
	/** Memory Operation Exception.
	 *
	 * When FEAT_MOPS is implemented
	 */
	memory_operation = 0b100111,
	/** Trapped floating-point exception taken from AArch32 state.
	 *
	 * This EC value is valid if the implementation supports trapping of
	 * floating-point exceptions, otherwise it is reserved. Whether a
	 * floating-point implementation supports trapping of floating-point exceptions
	 * is IMPLEMENTATION DEFINED.
	 *
	 * When AArch32 is supported
	 */
	floating_point_op_aarch32 = 0b101000,
	/** Trapped floating-point exception taken from AArch64 state.
	 *
	 * This EC value is valid if the implementation supports trapping of
	 * floating-point exceptions, otherwise it is reserved. Whether a
	 * floating-point implementation supports trapping of floating-point exceptions
	 * is IMPLEMENTATION DEFINED.
	 *
	 * When AArch64 is supported
	 */
	floating_point_op_aarch64 = 0b101100,
	/** GCS exception.
	 *
	 * When FEAT_GCS is implemented
	 */
	gcs = 0b101101,
	/** SError exception. */
	serror = 0b101111,
	/** Breakpoint exception from a lower Exception level. */
	breakpoint_lower_el = 0b110000,
	/** Breakpoint exception taken without a change in Exception level. */
	breakpoint_same_el = 0b110001,
	/** Software Step exception from a lower Exception level. */
	software_step_lower_el = 0b110010,
	/** Software Step exception taken without a change in Exception level. */
	software_step_same_el = 0b110011,
	/** Watchpoint from a lower Exception level, excluding Watchpoint Exceptions
	 * taken to EL2 as a result of accesses generated associated with VNCR_EL2 as
	 * part of nested virtualization support.
	 *
	 * These Watchpoint Exceptions might be generated from Exception levels using
	 * any Execution state.
	 */
	watchpoint_lower_el = 0b110100,
	/** Watchpoint exceptions without a change in Exception level, or Watchpoint
	 * exceptions taken to EL2 as a result of accesses generated associated with
	 * VNCR_EL2 as part of nested virtualization support.
	 */
	watchpoint_same_el = 0b110101,
	/** BKPT instruction execution in AArch32 state.
	 *
	 * When AArch32 is supported
	 */
	bkpt_aarch32 = 0b111000,
	/** Vector Catch exception from AArch32 state.
	 *
	 * The only case where a Vector Catch exception is taken to an Exception level
	 * that is using AArch64 is when the exception is routed to EL2 and EL2 is
	 * using AArch64.
	 *
	 * When AArch32 is supported
	 */
	vector_catch_aarch32 = 0b111010,
	/** BRK instruction execution in AArch64 state.
	 *
	 * When AArch64 is supported
	 */
	brk_aarch64 = 0b111100,
	/** PMU exception
	 *
	 * When FEAT_EBEP is implemented
	 */
	pmu = 0b111101,
	/** Reserved synchronous exception class start. */
	reserved_sync_start = 0b000000,
	/** Reserved synchronous exception class end. */
	reserved_sync_end = 0b101100,
	/** Reserved synchronous or asynchronous exception class start. */
	reserved_sync_async_start = 0b101101,
	/** Reserved synchronous or asynchronous exception class end. */
	reserved_sync_async_end = 0b111111,
};

/** Encoding of ESR_ELx.ISS.DFSC/IFSC for Data/Instruction Abort exceptions. */
enum class mmu_abort_status_code : uint8_t {
	/** Address size fault, level 0 of translation or translation table base
	 * register.
	 */
	address_size_l0 = 0b000000,
	/** Address size fault, level 1. */
	address_size_l1 = 0b000001,
	/** Address size fault, level 2. */
	address_size_l2 = 0b000010,
	/** Address size fault, level 3. */
	address_size_l3 = 0b000011,
	/** Translation fault, level 0. */
	translation_l0 = 0b000100,
	/** Translation fault, level 1. */
	translation_l1 = 0b000101,
	/** Translation fault, level 2. */
	translation_l2 = 0b000110,
	/** Translation fault, level 3. */
	translation_l3 = 0b000111,
	/** Access flag fault, level 1. */
	access_flag_l1 = 0b001001,
	/** Access flag fault, level 2. */
	access_flag_l2 = 0b001010,
	/** Access flag fault, level 3. */
	access_flag_l3 = 0b001011,
	/** Access flag fault, level 0.
	 *
	 * When FEAT_LPA2 is implemented
	 */
	access_flag_l0 = 0b001000,
	/** Permission fault, level 0.
	 *
	 * When FEAT_LPA2 is implemented
	 */
	permission_l0 = 0b001100,
	/** Permission fault, level 1. */
	permission_l1 = 0b001101,
	/** Permission fault, level 2. */
	permission_l2 = 0b001110,
	/** Permission fault, level 3. */
	permission_l3 = 0b001111,
	/** Synchronous External abort, not on translation table walk or hardware
	 * update of translation table.
	 */
	sync_external_abort_nowalk = 0b010000,
	/** Synchronous Tag Check Fault.
	 *
	 * When FEAT_MTE2 is implemented
	 */
	sync_tag_check = 0b010001,
	/** Synchronous External abort on translation table walk or hardware update
	 * of translation table, level -2.
	 *
	 * When FEAT_D128 is implemented
	 */
	sync_external_abort_lm2 = 0b010010,
	/** Synchronous External abort on translation table walk or hardware update
	 * of translation table, level -1.
	 *
	 * When FEAT_LPA2 is implemented
	 */
	sync_external_abort_lm1 = 0b010011,
	/** Synchronous External abort on translation table walk or hardware update
	 * of translation table, level 0.
	 */
	sync_external_abort_l0 = 0b010100,
	/** Synchronous External abort on translation table walk or hardware update
	 * of translation table, level 1.
	 */
	sync_external_abort_l1 = 0b010101,
	/** Synchronous External abort on translation table walk or hardware update
	 * of translation table, level 2.
	 */
	sync_external_abort_l2 = 0b010110,
	/** Synchronous External abort on translation table walk or hardware update
	 * of translation table, level 3.
	 */
	sync_external_abort_l3 = 0b010111,
	/** Synchronous parity or ECC error on memory access, not on translation
	 * table walk.
	 *
	 * When FEAT_RAS is not implemented
	 */
	sync_parity_ecc_nowalk = 0b011000,
	/** Synchronous parity or ECC error on memory access on translation table
	 * walk or hardware update of translation table, level -1.
	 *
	 * When FEAT_LPA2 is implemented and FEAT_RAS is not implemented
	 */
	sync_parity_ecc_lm1 = 0b011011,
	/** Synchronous parity or ECC error on memory access on translation table
	 * walk or hardware update of translation table, level 0.
	 *
	 * When FEAT_RAS is not implemented
	 */
	sync_parity_ecc_l0 = 0b011100,
	/** Synchronous parity or ECC error on memory access on translation table
	 * walk or hardware update of translation table, level 1.
	 *
	 * When FEAT_RAS is not implemented
	 */
	sync_parity_ecc_l1 = 0b011101,
	/** Synchronous parity or ECC error on memory access on translation table
	 * walk or hardware update of translation table, level 2.
	 *
	 * When FEAT_RAS is not implemented
	 */
	sync_parity_ecc_l2 = 0b011110,
	/** Synchronous parity or ECC error on memory access on translation table
	 * walk or hardware update of translation table, level 3.
	 *
	 * When FEAT_RAS is not implemented
	 */
	sync_parity_ecc_l3 = 0b011111,
	/** Alignment fault. */
	alignment = 0b100001,
	/** Granule Protection Fault on translation table walk or hardware update
	 * of translation table, level -2.
	 *
	 * When FEAT_D128 is implemented and FEAT_RME is implemented
	 */
	granule_protection_lm2 = 0b100010,
	/** Granule Protection Fault on translation table walk or hardware update
	 * of translation table, level -1.
	 *
	 * When FEAT_RME is implemented and FEAT_LPA2 is implemented
	 */
	granule_protection_lm1 = 0b100011,
	/** Granule Protection Fault on translation table walk or hardware update
	 * of translation table, level 0.
	 *
	 * When FEAT_RME is implemented
	 */
	granule_protection_l0 = 0b100100,
	/** Granule Protection Fault on translation table walk or hardware update
	 * of translation table, level 1.
	 *
	 * When FEAT_RME is implemented
	 */
	granule_protection_l1 = 0b100101,
	/** Granule Protection Fault on translation table walk or hardware update
	 * of translation table, level 2.
	 *
	 * When FEAT_RME is implemented
	 */
	granule_protection_l2 = 0b100110,
	/** Granule Protection Fault on translation table walk or hardware update
	 * of translation table, level 3.
	 *
	 * When FEAT_RME is implemented
	 */
	granule_protection_l3 = 0b100111,
	/** Granule Protection Fault, not on translation table walk or hardware
	 * update of translation table.
	 *
	 * When FEAT_RME is implemented
	 */
	granule_protection_nowalk = 0b101000,
	/** Address size fault, level -1.
	 *
	 * When FEAT_LPA2 is implemented
	 */
	address_size_lm1 = 0b101001,
	/** Translation fault, level -2.
	 *
	 * When FEAT_D128 is implemented
	 */
	translation_lm2 = 0b101010,
	/** Translation fault, level -1.
	 *
	 * When FEAT_LPA2 is implemented
	 */
	translation_lm1 = 0b101011,
	/** Address Size fault, level -2.
	 *
	 * When FEAT_D128 is implemented
	 */
	address_size_lm2 = 0b101100,
	/** TLB conflict abort. */
	tlb_conflict = 0b110000,
	/** Unsupported atomic hardware update fault.
	 *
	 * When FEAT_HAFDBS is implemented
	 */
	unsupported_atomic_hw_update = 0b110001,
	/** IMPLEMENTATION DEFINED fault (Lockdown). */
	lockdown = 0b110100,
	/** IMPLEMENTATION DEFINED fault (Unsupported Exclusive or Atomic access). */
	unsupported_exclusive_atomic = 0b110101,
};

#define EXCEPTION_FRAME_SWITCH_CASE(input) \
	switch(input) { \
		case reg_enc::x0: return x0; \
		case reg_enc::x1: return x1; \
		case reg_enc::x2: return x2; \
		case reg_enc::x3: return x3; \
		case reg_enc::x4: return x4; \
		case reg_enc::x5: return x5; \
		case reg_enc::x6: return x6; \
		case reg_enc::x7: return x7; \
		case reg_enc::x8: return x8; \
		case reg_enc::x9: return x9; \
		case reg_enc::x10: return x10; \
		case reg_enc::x11: return x11; \
		case reg_enc::x12: return x12; \
		case reg_enc::x13: return x13; \
		case reg_enc::x14: return x14; \
		case reg_enc::x15: return x15; \
		case reg_enc::x16: return x16; \
		case reg_enc::x17: return x17; \
		case reg_enc::x18: return x18; \
		case reg_enc::x19: return x19; \
		case reg_enc::x20: return x20; \
		case reg_enc::x21: return x21; \
		case reg_enc::x22: return x22; \
		case reg_enc::x23: return x23; \
		case reg_enc::x24: return x24; \
		case reg_enc::x25: return x25; \
		case reg_enc::x26: return x26; \
		case reg_enc::x27: return x27; \
		case reg_enc::x28: return x28; \
		case reg_enc::x29: return x29; \
		case reg_enc::x30: return x30; \
		case reg_enc::xzr: return xzr; \
		default: todo("Unknown register encoding"); \
	}

struct exception_frame {
	uint64_t x29;
	uint64_t xzr;
	uint64_t x27;
	uint64_t x28;
	uint64_t x25;
	uint64_t x26;
	uint64_t x23;
	uint64_t x24;
	uint64_t x21;
	uint64_t x22;
	uint64_t x19;
	uint64_t x20;
	uint64_t x18;
	uint64_t x30;
	uint64_t x16;
	uint64_t x17;
	uint64_t x14;
	uint64_t x15;
	uint64_t x12;
	uint64_t x13;
	uint64_t x10;
	uint64_t x11;
	uint64_t x8;
	uint64_t x9;
	uint64_t x6;
	uint64_t x7;
	uint64_t x4;
	uint64_t x5;
	uint64_t x2;
	uint64_t x3;
	uint64_t x0;
	uint64_t x1;

	uint64_t at(const reg_enc enc) const {
		EXCEPTION_FRAME_SWITCH_CASE(enc);
	}

	uint64_t& at(const reg_enc enc) {
		EXCEPTION_FRAME_SWITCH_CASE(enc);
	}
};

#undef EXCEPTION_FRAME_SWITCH_CASE

namespace exception {
	constexpr bool is_lower_el_mmu_abort(const exception_class ec) {
		return ec == exception_class::instruction_abort_lower_el
			|| ec == exception_class::data_abort_lower_el;
	}

	constexpr bool is_permission_fault(const mmu_abort_status_code dfsc) {
		return dfsc == mmu_abort_status_code::permission_l0
			|| dfsc == mmu_abort_status_code::permission_l1
			|| dfsc == mmu_abort_status_code::permission_l2
			|| dfsc == mmu_abort_status_code::permission_l3;
	}

	constexpr bool is_translation_fault(const mmu_abort_status_code dfsc) {
		return dfsc == mmu_abort_status_code::translation_l0
			|| dfsc == mmu_abort_status_code::translation_l1
			|| dfsc == mmu_abort_status_code::translation_l2
			|| dfsc == mmu_abort_status_code::translation_l3
			|| dfsc == mmu_abort_status_code::translation_lm1
			|| dfsc == mmu_abort_status_code::translation_lm2;
	}
}
