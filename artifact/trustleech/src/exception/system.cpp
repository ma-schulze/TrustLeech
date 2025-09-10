#include "system.hpp"

#include "lib/libc/stdio.hpp"
#include <assert.hpp>
#include <exception/inject.hpp>
#include <exception/types.hpp>
#include <paging/shadow/shadow.hpp>
#include <registers.hpp>
#include <sysops.hpp>
#include <types.hpp>
#include <vcpu_ctx.hpp>

namespace exception {
	namespace {
		/** Encoding of system registers and instructions
		 *
		 * The value is the following concatenation:
		 * op0 (2 bits) : op1 (3 bits) : CRn (4 bits) : CRm (4 bits) : op2 (3 bits)
		 */
		enum class encoding : uint16_t {
			cntp_ctl_el0         = 0b11'011'1110'0010'001,
			cntp_cval_el0        = 0b11'011'1110'0010'010,
			cntp_tval_el0        = 0b11'011'1110'0010'000,
			cntv_ctl_el02        = 0b11'101'1110'0011'001,
			cntv_cval_el02       = 0b11'101'1110'0011'010,
			cntv_tval_el02       = 0b11'101'1110'0011'000,
			cntp_ctl_el02        = 0b11'101'1110'0010'001,
			cntp_cval_el02       = 0b11'101'1110'0010'010,
			cntp_tval_el02       = 0b11'101'1110'0010'000,
			cntvctss_el0         = 0b11'011'1110'0000'110,
			mdcr_el2             = 0b11'100'0001'0001'001,
			ich_vtr_el2          = 0b11'100'1100'1011'001,
			ich_elrsr_el2        = 0b11'100'1100'1011'101,
			cnthctl_el2          = 0b11'100'1110'0001'000,
			icc_sre_el2          = 0b11'100'1100'1001'101,
			cntkctl_el12         = 0b11'101'1110'0001'000,
			tlbi_vae2os          = 0b01'100'1000'0001'001,
			tlbi_ipas2e1is       = 0b01'100'1000'0000'001,
			tlbi_ripas2e1is      = 0b01'100'1000'0000'010,
			hpfar_el2            = 0b11'100'0110'0000'100,
			id_aa64afr0_el1      = 0b11'000'0000'0101'100,
			id_aa64afr1_el1      = 0b11'000'0000'0101'101,
			id_aa64dfr0_el1      = 0b11'000'0000'0101'000,
			id_aa64dfr1_el1      = 0b11'000'0000'0101'001,
			id_aa64isar0_el1     = 0b11'000'0000'0110'000,
			id_aa64isar1_el1     = 0b11'000'0000'0110'001,
			id_aa64isar2_el1     = 0b11'000'0000'0110'010,
            id_aa64isar3_el1     = 0b11'000'0000'0110'011,
			id_aa64mmfr0_el1     = 0b11'000'0000'0111'000,
			id_aa64mmfr1_el1     = 0b11'000'0000'0111'001,
			id_aa64mmfr2_el1     = 0b11'000'0000'0111'010,
			id_aa64mmfr3_el1     = 0b11'000'0000'0111'011,
			id_aa64mmfr4_el1     = 0b11'000'0000'0111'100,
			id_aa64pfr0_el1      = 0b11'000'0000'0100'000,
			id_aa64pfr1_el1      = 0b11'000'0000'0100'001,
			id_aa64pfr2_el1      = 0b11'000'0000'0100'010,
			id_aa64smfr0_el1     = 0b11'000'0000'0100'101,
            id_aa64smfr1_el1     = 0b11'000'0000'0100'111,
			id_aa64zfr0_el1      = 0b11'000'0000'0100'100,
			id_afr0_el1          = 0b11'000'0000'0001'011,
			id_dfr0_el1          = 0b11'000'0000'0001'010,
			id_dfr1_el1          = 0b11'000'0000'0011'101,
			id_isar0_el1         = 0b11'000'0000'0010'000,
			id_isar1_el1         = 0b11'000'0000'0010'001,
			id_isar2_el1         = 0b11'000'0000'0010'010,
			id_isar3_el1         = 0b11'000'0000'0010'011,
			id_isar4_el1         = 0b11'000'0000'0010'100,
			id_isar5_el1         = 0b11'000'0000'0010'101,
			id_isar6_el1         = 0b11'000'0000'0010'111,
			id_mmfr0_el1         = 0b11'000'0000'0001'100,
			id_mmfr1_el1         = 0b11'000'0000'0001'101,
			id_mmfr2_el1         = 0b11'000'0000'0001'110,
			id_mmfr3_el1         = 0b11'000'0000'0001'111,
			id_mmfr4_el1         = 0b11'000'0000'0010'110,
			id_mmfr5_el1         = 0b11'000'0000'0011'110,
			id_pfr0_el1          = 0b11'000'0000'0001'000,
			id_pfr1_el1          = 0b11'000'0000'0001'001,
			id_pfr2_el1          = 0b11'000'0000'0011'100,
			mvfr0_el1            = 0b11'000'0000'0011'000,
			mvfr1_el1            = 0b11'000'0000'0011'001,
			mvfr2_el1            = 0b11'000'0000'0011'010,
			apiakeylo_el1        = 0b11'000'0010'0001'000,
			apiakeyhi_el1        = 0b11'000'0010'0001'001,
			apibkeylo_el1        = 0b11'000'0010'0001'010,
			apibkeyhi_el1        = 0b11'000'0010'0001'011,
			apdakeylo_el1        = 0b11'000'0010'0010'000,
			apdakeyhi_el1        = 0b11'000'0010'0010'001,
			apdbkeylo_el1        = 0b11'000'0010'0010'010,
			apdbkeyhi_el1        = 0b11'000'0010'0010'011,
			apgakeylo_el1        = 0b11'000'0010'0011'000,
			apgakeyhi_el1        = 0b11'000'0010'0011'001,
			clidr_el1            = 0b11'001'0000'0000'001,
			mdscr_el1            = 0b10'000'0000'0010'010,
			oslar_el1            = 0b10'000'0001'0000'100,
			oslsr_el1            = 0b10'000'0001'0001'100,
			osdlr_el1            = 0b10'000'0001'0011'100,
			dbgbcr0_el1          = 0b10'000'0000'0000'101,
			dbgbcr1_el1          = 0b10'000'0000'0001'101,
			dbgbcr2_el1          = 0b10'000'0000'0010'101,
			dbgbcr3_el1          = 0b10'000'0000'0011'101,
			dbgbcr4_el1          = 0b10'000'0000'0100'101,
			dbgbcr5_el1          = 0b10'000'0000'0101'101,
			dbgbcr6_el1          = 0b10'000'0000'0110'101,
			dbgbcr7_el1          = 0b10'000'0000'0111'101,
			dbgbcr8_el1          = 0b10'000'0000'1000'101,
			dbgbcr9_el1          = 0b10'000'0000'1001'101,
			dbgbcr10_el1         = 0b10'000'0000'1010'101,
			dbgbcr11_el1         = 0b10'000'0000'1011'101,
			dbgbcr12_el1         = 0b10'000'0000'1100'101,
			dbgbcr13_el1         = 0b10'000'0000'1101'101,
			dbgbcr14_el1         = 0b10'000'0000'1110'101,
			dbgbcr15_el1         = 0b10'000'0000'1111'101,
            dbgbvr0_el1          = 0b10'000'0000'0000'100,
			dbgbvr1_el1          = 0b10'000'0000'0001'100,
			dbgbvr2_el1          = 0b10'000'0000'0010'100,
			dbgbvr3_el1          = 0b10'000'0000'0011'100,
			dbgbvr4_el1          = 0b10'000'0000'0100'100,
			dbgbvr5_el1          = 0b10'000'0000'0101'100,
			dbgbvr6_el1          = 0b10'000'0000'0110'100,
			dbgbvr7_el1          = 0b10'000'0000'0111'100,
			dbgbvr8_el1          = 0b10'000'0000'1000'100,
			dbgbvr9_el1          = 0b10'000'0000'1001'100,
			dbgbvr10_el1         = 0b10'000'0000'1010'100,
			dbgbvr11_el1         = 0b10'000'0000'1011'100,
			dbgbvr12_el1         = 0b10'000'0000'1100'100,
			dbgbvr13_el1         = 0b10'000'0000'1101'100,
			dbgbvr14_el1         = 0b10'000'0000'1110'100,
			dbgbvr15_el1         = 0b10'000'0000'1111'100,
            isr_el1              = 0b11'000'1100'0001'000,

		};

		constexpr encoding encoding_from_syndrome(const exception_syndrome esr) {
			assert(esr.exception_class() == exception_class::system);

			const uint16_t op0 = esr.system.op0();
			const uint16_t op1 = esr.system.op1();
			const uint16_t crn = esr.system.crn();
			const uint16_t crm = esr.system.crm();
			const uint16_t op2 = esr.system.op2();

			return static_cast<encoding>((op0 << 14) | (op1 << 11) | (crn << 7) | (crm << 3) | (op2 << 0));
		}
	}

// Note that this does not handle non-E2H traps, and traps by VMs for now
#define EMULATE_E2H_REDIR(source, target) \
	case encoding::source: \
		if (!vcpu().is_vel2 || !vcpu().hcr_el2().e2h()) { \
				aarch64::take_sync_exception(EL2, esr); \
				vcpu().pc -=  \
					esr.instruction_length_32bit() ? 4 : 2; \
		} \
		\
		if (is_read) { \
			frame->at(reg) = vcpu().target().v; \
		} else { \
			vcpu().set_ ## target({ frame->at(reg) }); \
		} \
		break;


#define EMULATE_EL2_ACCESS(el2_field) \
	case encoding::el2_field: \
		if (!vcpu().is_vel2) { \
		    aarch64::take_sync_exception(EL2, esr); \
			vcpu().pc -= \
			  esr.instruction_length_32bit() ? 4 : 2; \
		} \
		\
		if (is_read) { \
			frame->at(reg) = vcpu().el2_field().v; \
		} else { \
			vcpu().set_ ## el2_field({ frame->at(reg) }); \
		} \
		break;

#define EMULATE_EL2_ACCESS_RO(el2_field) \
	case encoding::el2_field: \
		if (!vcpu().is_vel2) { \
		    aarch64::take_sync_exception(EL2, esr); \
			vcpu().pc -= \
			  esr.instruction_length_32bit() ? 4 : 2; \
		} \
		\
		if (is_read) { \
			frame->at(reg) = vcpu().el2_field().v; \
		} else { \
		    aarch64::take_sync_exception(EL2, esr); \
		} \
		break;



#define EMULATE_EL1_ACCESS(el1_field) \
	case encoding::el1_field: \
		if (vcpu().is_vel2) { \
			todo("Unsure why this would be trapped"); \
		} else { \
			aarch64::take_sync_exception(EL2, esr); \
			vcpu().pc -= esr.instruction_length_32bit() ? 4 : 2; \
		} \
		break;


	void handle_system(const exception_syndrome esr, exception_frame* frame) {
		(void) esr;
		(void) frame;
		assert(esr.exception_class() == exception_class::system);
		// We require access to the frame since we read/write registers when
		// emulating system instructions
		assert(frame);

		const bool is_read = esr.system.is_read();
		const reg_enc reg = esr.system.rt();

		switch (encoding_from_syndrome(esr)) {
			EMULATE_E2H_REDIR(cntp_ctl_el0, cnthp_ctl_el2);
			EMULATE_E2H_REDIR(cntp_cval_el0, cnthp_cval_el2);
			EMULATE_E2H_REDIR(cntp_tval_el0, cnthp_tval_el2);
			EMULATE_E2H_REDIR(cntv_ctl_el02, cntv_ctl_el0);
			EMULATE_E2H_REDIR(cntv_cval_el02, cntv_cval_el0);
			EMULATE_E2H_REDIR(cntv_tval_el02, cntv_tval_el0);
			EMULATE_E2H_REDIR(cntp_ctl_el02, cntp_ctl_el0);
			EMULATE_E2H_REDIR(cntp_cval_el02, cntp_cval_el0);
			EMULATE_E2H_REDIR(cntp_tval_el02, cntp_tval_el0);
			case encoding::cntvctss_el0: {
				assert(is_read);

				if (!vcpu().is_vel2) {
				  aarch64::take_sync_exception(EL2, esr);
				  vcpu().pc -=
					  esr.instruction_length_32bit() ? 4 : 2;
				}

				const uint64_t physical = read_cntpctss_el0().v;

				if (vcpu().hcr_el2().e2h() && vcpu().hcr_el2().tge()) [[likely]] {
					frame->at(reg) = physical;
				} else {
					frame->at(reg) = physical - vcpu().cntvoff_el2().v;
				}

				break;
			}
			// EMULATE_EL2_ACCESS(mdcr_el2);
            case encoding::mdcr_el2:
              break;
			EMULATE_EL2_ACCESS_RO(ich_vtr_el2);
            case encoding::ich_elrsr_el2: {
              uint64_t i = 0;
              if (!(vcpu().memory_backend.ich_lr0_el2().v & (3UL << 62)))
                i |= 1;
              if (!(vcpu().memory_backend.ich_lr1_el2().v & (3UL << 62)))
                i |= 2;
              if (!(vcpu().memory_backend.ich_lr2_el2().v & (3UL << 62)))
                i |= 4;
              if (!(vcpu().memory_backend.ich_lr3_el2().v & (3UL << 62)))
                i |= 8;

              frame->at(reg) = i;

              break;
            }
			EMULATE_EL2_ACCESS(cnthctl_el2);
			EMULATE_EL2_ACCESS(icc_sre_el2);
			EMULATE_EL2_ACCESS(hpfar_el2);
			EMULATE_E2H_REDIR(cntkctl_el12, cntkctl_el1);
			case encoding::tlbi_vae2os: {
				if (vcpu().virtual_el() != EL2) {
					todo("Should not be trapped for now");
				}
				tlbi_vae1os(frame->at(reg));
				break;
			}
			case encoding::tlbi_ipas2e1is:
			case encoding::tlbi_ripas2e1is: {
				paging::shadow::clear_cache();

				const auto old_vttbr = read_vttbr_el2();
				write_vttbr_el2(vcpu().vttbr_el2());
				tlbi_vmalle1is();
				write_vttbr_el2(old_vttbr);

				break;
			}
			EMULATE_EL1_ACCESS(id_aa64afr0_el1)
			EMULATE_EL1_ACCESS(id_aa64afr1_el1)
			EMULATE_EL1_ACCESS(id_aa64dfr0_el1)
			EMULATE_EL1_ACCESS(id_aa64dfr1_el1)
			EMULATE_EL1_ACCESS(id_aa64isar0_el1)
			EMULATE_EL1_ACCESS(id_aa64isar1_el1)
			EMULATE_EL1_ACCESS(id_aa64isar2_el1)
            EMULATE_EL1_ACCESS(id_aa64isar3_el1)
			EMULATE_EL1_ACCESS(id_aa64mmfr0_el1)
			EMULATE_EL1_ACCESS(id_aa64mmfr1_el1)
			EMULATE_EL1_ACCESS(id_aa64mmfr2_el1)
			EMULATE_EL1_ACCESS(id_aa64mmfr3_el1)
			EMULATE_EL1_ACCESS(id_aa64mmfr4_el1)
			EMULATE_EL1_ACCESS(id_aa64pfr0_el1)
			EMULATE_EL1_ACCESS(id_aa64pfr1_el1)
			EMULATE_EL1_ACCESS(id_aa64pfr2_el1)
			EMULATE_EL1_ACCESS(id_aa64smfr0_el1)
        	EMULATE_EL1_ACCESS(id_aa64smfr1_el1)
			EMULATE_EL1_ACCESS(id_aa64zfr0_el1)
			EMULATE_EL1_ACCESS(id_afr0_el1)
			EMULATE_EL1_ACCESS(id_dfr0_el1)
			EMULATE_EL1_ACCESS(id_dfr1_el1)
			EMULATE_EL1_ACCESS(id_isar0_el1)
			EMULATE_EL1_ACCESS(id_isar1_el1)
			EMULATE_EL1_ACCESS(id_isar2_el1)
			EMULATE_EL1_ACCESS(id_isar3_el1)
			EMULATE_EL1_ACCESS(id_isar4_el1)
			EMULATE_EL1_ACCESS(id_isar5_el1)
			EMULATE_EL1_ACCESS(id_isar6_el1)
			EMULATE_EL1_ACCESS(id_mmfr0_el1)
			EMULATE_EL1_ACCESS(id_mmfr1_el1)
			EMULATE_EL1_ACCESS(id_mmfr2_el1)
			EMULATE_EL1_ACCESS(id_mmfr3_el1)
			EMULATE_EL1_ACCESS(id_mmfr4_el1)
			EMULATE_EL1_ACCESS(id_mmfr5_el1)
			EMULATE_EL1_ACCESS(id_pfr0_el1)
			EMULATE_EL1_ACCESS(id_pfr1_el1)
			EMULATE_EL1_ACCESS(id_pfr2_el1)
			EMULATE_EL1_ACCESS(mvfr0_el1)
			EMULATE_EL1_ACCESS(mvfr1_el1)
			EMULATE_EL1_ACCESS(mvfr2_el1)
			EMULATE_EL1_ACCESS(apdakeyhi_el1)
			EMULATE_EL1_ACCESS(apdakeylo_el1)
			EMULATE_EL1_ACCESS(apdbkeyhi_el1)
			EMULATE_EL1_ACCESS(apdbkeylo_el1)
			EMULATE_EL1_ACCESS(apgakeyhi_el1)
			EMULATE_EL1_ACCESS(apgakeylo_el1)
			EMULATE_EL1_ACCESS(apiakeyhi_el1)
			EMULATE_EL1_ACCESS(apiakeylo_el1)
			EMULATE_EL1_ACCESS(apibkeyhi_el1)
			EMULATE_EL1_ACCESS(apibkeylo_el1)
			EMULATE_EL1_ACCESS(clidr_el1)
            case encoding::isr_el1:
               assert(is_read);
                frame->at(reg) = 0; // read_isr_el1().v;
                break;
			// EMULATE_EL1_ACCESS(mdscr_el1)
            case encoding::mdscr_el1: 
              break;
			EMULATE_EL1_ACCESS(oslar_el1)
			EMULATE_EL1_ACCESS(oslsr_el1)
			EMULATE_EL1_ACCESS(osdlr_el1)
            case encoding::dbgbcr0_el1:
            case encoding::dbgbcr1_el1:
            case encoding::dbgbcr2_el1:
            case encoding::dbgbcr3_el1:
            case encoding::dbgbcr4_el1:
            case encoding::dbgbcr5_el1:
            case encoding::dbgbcr6_el1:
            case encoding::dbgbcr7_el1:
            case encoding::dbgbcr8_el1:
            case encoding::dbgbcr9_el1:
            case encoding::dbgbcr10_el1:
            case encoding::dbgbcr11_el1:
            case encoding::dbgbcr12_el1:
            case encoding::dbgbcr13_el1:
            case encoding::dbgbcr14_el1:
            case encoding::dbgbcr15_el1:
            case encoding::dbgbvr0_el1:
            case encoding::dbgbvr1_el1:
            case encoding::dbgbvr2_el1:
            case encoding::dbgbvr3_el1:
            case encoding::dbgbvr4_el1:
            case encoding::dbgbvr5_el1:
            case encoding::dbgbvr6_el1:
            case encoding::dbgbvr7_el1:
            case encoding::dbgbvr8_el1:
            case encoding::dbgbvr9_el1:
            case encoding::dbgbvr10_el1:
            case encoding::dbgbvr11_el1:
            case encoding::dbgbvr12_el1:
            case encoding::dbgbvr13_el1:
            case encoding::dbgbvr14_el1:
            case encoding::dbgbvr15_el1:
              /* Prevent all access to DBG registers for now */
              break;
			default:
		{ // by default, if we do not know something we just skip it or forward it to vel2
			if (!vcpu().is_vel2) {
				aarch64::take_sync_exception(EL2, esr);
				vcpu().pc -=
					esr.instruction_length_32bit() ? 4 : 2;
			}
			break;
		}

                
		}

		// Increment the PC to the next Instruction
		vcpu().pc += esr.instruction_length_32bit() ? 4 : 2;
	}
}
