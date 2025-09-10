#pragma once

#include <registers.hpp>
#include <types.hpp>

#define DEFINE_RENAME_NV2_REDIR_REG(type, field) \
	public: \
		type field() const { \
			return (type){ _nv2_redir_area[static_cast<size_t>(nv2_redir_offset::field) / sizeof(uint64_t)] }; \
		} \
		\
		void set_ ## field(type wrapper) { \
			_nv2_redir_area[static_cast<size_t>(nv2_redir_offset::field) / sizeof(uint64_t)] = wrapper.v; \
		}


#define DEFINE_NV2_REDIR_REG(struct_or_union, field) \
	DEFINE_RENAME_NV2_REDIR_REG(struct_or_union field, field)


#define DEFINE_RENAME_STD_REG(type, field) \
	private: \
		type _ ## field { 0 }; \
	public: \
		type field() const { \
			return _ ## field; \
		} \
		\
		void set_ ## field(type wrapper) { \
			_ ## field = wrapper; \
		}


#define DEFINE_STD_REG(struct_or_union, field) \
	DEFINE_RENAME_STD_REG(struct_or_union field, field)


/** Memory backing for "all" (in time) registers accesible by vEL2 */
class memory_backed_regs {
	private:
		/** Index for nv2_redir_area
		 *
		 * According to R_CSRPQ
		 */
		enum class nv2_redir_offset : size_t {
			vttbr_el2          = 0x20,
			vsttbr_el2         = 0x30,
			vtcr_el2           = 0x40,
			vstcr_el2          = 0x48,
			vmpidr_el2         = 0x50,
			cntvoff_el2        = 0x60,
			hcr_el2            = 0x78,
			hstr_el2           = 0x80,
			vpidr_el2          = 0x88,
			tpidr_el2          = 0x90,
			hcrx_el2           = 0xA0,
			vncr_el2           = 0xB0,
			cpacr_el1          = 0x100,
			contextidr_el12    = 0x108,
			contextidr_el1     = 0x108,
			sctlr_el12         = 0x110,
			sctlr_el1          = 0x110,
			actlr_el1          = 0x118,
			tcr_el12           = 0x120,
			tcr_el1            = 0x120,
			afsr0_el12         = 0x128,
			afsr0_el1          = 0x128,
			afsr1_el12         = 0x130,
			afsr1_el1          = 0x130,
			esr_el12           = 0x138,
			esr_el1            = 0x138,
			mair_el12          = 0x140,
			mair_el1           = 0x140,
			amair_el12         = 0x148,
			amair_el1          = 0x148,
			mdscr_el1          = 0x158,
			spsr_el12          = 0x160,
			spsr_el1           = 0x160,
			cntv_cval_el02     = 0x168,
			cntv_cval_el0      = 0x168,
			cntv_ctl_el02      = 0x170,
			cntv_ctl_el0       = 0x170,
			cntp_cval_el02     = 0x178,
			cntp_cval_el0      = 0x178,
			cntp_ctl_el02      = 0x180,
			cntp_ctl_el0       = 0x180,
			scxtnum_el12       = 0x188,
			scxtnum_el1        = 0x188,
			tfsr_el12          = 0x190,
			tfsr_el1           = 0x190,
			cntpoff_el2        = 0x1A8,
			hfgrtr_el2         = 0x1B8,
			hfgwtr_el2         = 0x1C0,
			hfgitr_el2         = 0x1C8,
			hdfgrtr_el2        = 0x1D0,
			hdfgwtr_el2        = 0x1D8,
			zcr_el12           = 0x1E0,
			zcr_el1            = 0x1E0,
			hafgrtr_el2        = 0x1E8,
			smcr_el12          = 0x1F0,
			smcr_el1           = 0x1F0,
			smprimap_el2       = 0x1F8,
			ttbr0_el12         = 0x200,
			ttbr0_el1          = 0x200,
			ttbr1_el12         = 0x210,
			ttbr1_el1          = 0x210,
			far_el12           = 0x220,
			far_el1            = 0x220,
			elr_el12           = 0x230,
			elr_el1            = 0x230,
			sp_el1             = 0x240,
			vbar_el12          = 0x250,
			vbar_el1           = 0x250,
			tcr2_el12          = 0x270,
			tcr2_el1           = 0x270,
			sctlr2_el12        = 0x278,
			sctlr2_el1         = 0x278,
			ich_lr0_el2        = 0x400,
			ich_lr1_el2        = 0x408,
			ich_lr2_el2        = 0x410,
			ich_lr3_el2        = 0x418,
			ich_lr4_el2        = 0x420,
			ich_lr5_el2        = 0x428,
			ich_lr6_el2        = 0x430,
			ich_lr7_el2        = 0x438,
			ich_lr8_el2        = 0x440,
			ich_lr9_el2        = 0x448,
			ich_lr10_el2       = 0x450,
			ich_lr11_el2       = 0x458,
			ich_lr12_el2       = 0x460,
			ich_lr13_el2       = 0x468,
			ich_lr14_el2       = 0x470,
			ich_lr15_el2       = 0x478,
			ich_ap0r0_el2      = 0x480,
			ich_ap0r1_el2      = 0x488,
			ich_ap0r2_el2      = 0x490,
			ich_ap0r3_el2      = 0x498,
			ich_ap1r0_el2      = 0x4A0,
			ich_ap1r1_el2      = 0x4A8,
			ich_ap1r2_el2      = 0x4B0,
			ich_ap1r3_el2      = 0x4B8,
			ich_hcr_el2        = 0x4C0,
			ich_vmcr_el2       = 0x4C8,
			vdisr_el2          = 0x500,
			vsesr_el2          = 0x508,
			pmblimitr_el1      = 0x800,
			pmbptr_el1         = 0x810,
			pmbsr_el1          = 0x820,
			pmscr_el12         = 0x828,
			pmscr_el1          = 0x828,
			pmsevfr_el1        = 0x830,
			pmsicr_el1         = 0x838,
			pmsirr_el1         = 0x840,
			pmslatfr_el1       = 0x848,
			pmsnevfr_el1       = 0x850,
			trfcr_el12         = 0x880,
			trfcr_el1          = 0x880,
			brbcr_el12         = 0x8E0,
			brbcr_el1          = 0x8E0,
			mpam1_el12         = 0x900,
			mpam1_el1          = 0x900,
			mpamhcr_el2        = 0x930,
			mpamvpmv_el2       = 0x938,
			mpamvpm0_el2       = 0x940,
			mpamvpm1_el2       = 0x948,
			mpamvpm2_el2       = 0x950,
			mpamvpm3_el2       = 0x958,
			mpamvpm4_el2       = 0x960,
			mpamvpm5_el2       = 0x968,
			mpamvpm6_el2       = 0x970,
			mpamvpm7_el2       = 0x978,
			amevcntvoff00_el2  = 0xA00,
			amevcntvoff01_el2  = 0xA08,
			amevcntvoff02_el2  = 0xA10,
			amevcntvoff03_el2  = 0xA18,
			amevcntvoff04_el2  = 0xA20,
			amevcntvoff05_el2  = 0xA28,
			amevcntvoff06_el2  = 0xA30,
			amevcntvoff07_el2  = 0xA38,
			amevcntvoff08_el2  = 0xA40,
			amevcntvoff09_el2  = 0xA48,
			amevcntvoff010_el2 = 0xA50,
			amevcntvoff011_el2 = 0xA58,
			amevcntvoff012_el2 = 0xA60,
			amevcntvoff013_el2 = 0xA68,
			amevcntvoff014_el2 = 0xA70,
			amevcntvoff015_el2 = 0xA78,
			amevcntvoff10_el2  = 0xA80,
			amevcntvoff11_el2  = 0xA88,
			amevcntvoff12_el2  = 0xA90,
			amevcntvoff13_el2  = 0xA98,
			amevcntvoff14_el2  = 0xAA0,
			amevcntvoff15_el2  = 0xAA8,
			amevcntvoff16_el2  = 0xAB0,
			amevcntvoff17_el2  = 0xAB8,
			amevcntvoff18_el2  = 0xAC0,
			amevcntvoff19_el2  = 0xAC8,
			amevcntvoff110_el2 = 0xAD0,
			amevcntvoff111_el2 = 0xAD8,
			amevcntvoff112_el2 = 0xAE0,
			amevcntvoff113_el2 = 0xAE8,
			amevcntvoff114_el2 = 0xAF0,
			amevcntvoff115_el2 = 0xAF8,
		};

	public:
        using nv2_redir_area = uint64_t[0x1000 / sizeof(uint64_t)];

    private:
		/** Registers redirected as per R_CSRPQ */
		alignas(0x1000) nv2_redir_area _nv2_redir_area;
	public:
		/** Returns a pointer to an array suitable to be written to VNCR_EL2 */
		nv2_redir_area* get_nv2_redir_area() {
			return &_nv2_redir_area;
		}

	// Auxiliary
	DEFINE_STD_REG(struct, amair_el2);
	DEFINE_STD_REG(struct, actlr_el2);
	DEFINE_STD_REG(struct, afsr0_el2);
	DEFINE_STD_REG(struct, afsr1_el2);
	DEFINE_STD_REG(struct, hacr_el2);

	// System Control
	DEFINE_RENAME_STD_REG(union sctlr, sctlr_el2);
	DEFINE_RENAME_STD_REG(union stack_pointer, sp_el2);
	DEFINE_RENAME_NV2_REDIR_REG(union software_thread_id, tpidr_el2);

	// Exceptions
	DEFINE_RENAME_STD_REG(union vector_base, vbar_el2);
	DEFINE_RENAME_STD_REG(struct saved_program_status, spsr_el2);
	DEFINE_RENAME_STD_REG(union exception_link, elr_el2);
	DEFINE_RENAME_STD_REG(union exception_syndrome, esr_el2);
	DEFINE_RENAME_STD_REG(union faulting_address, far_el2);

	// MMU
	DEFINE_STD_REG(struct, tcr_el2);
	DEFINE_RENAME_STD_REG(union indirect_mem_attr, mair_el2);
	DEFINE_RENAME_STD_REG(struct xlate_tbl_base, ttbr0_el2);

	// Hypervisor Control
	DEFINE_NV2_REDIR_REG(struct, hcr_el2);
	DEFINE_NV2_REDIR_REG(struct, cntvoff_el2);
	DEFINE_STD_REG(struct, hpfar_el2);
	DEFINE_NV2_REDIR_REG(struct, hstr_el2);
	DEFINE_RENAME_NV2_REDIR_REG(struct mpid, vmpidr_el2);
	DEFINE_RENAME_NV2_REDIR_REG(struct pid, vpidr_el2);
	DEFINE_NV2_REDIR_REG(struct, vtcr_el2);
	DEFINE_NV2_REDIR_REG(struct, vttbr_el2);
	DEFINE_STD_REG(union, cnthctl_el2);
	DEFINE_STD_REG(union, cptr_el2);

	// Debug
	DEFINE_STD_REG(struct, mdcr_el2);

	//
	// EL1 registers
	//

	// Auxiliary
	DEFINE_NV2_REDIR_REG(struct, actlr_el1);
	DEFINE_NV2_REDIR_REG(struct, afsr0_el1);
	DEFINE_NV2_REDIR_REG(struct, afsr1_el1);
	DEFINE_NV2_REDIR_REG(struct, amair_el1);

	// System control
	DEFINE_RENAME_NV2_REDIR_REG(union sctlr, sctlr_el1);
	DEFINE_RENAME_NV2_REDIR_REG(union stack_pointer, sp_el1);
	DEFINE_RENAME_NV2_REDIR_REG(struct ctx_id, contextidr_el1);
	DEFINE_NV2_REDIR_REG(struct, cpacr_el1);

	// Generic Timer
	DEFINE_STD_REG(struct, cntkctl_el1);

	// Exceptions
	DEFINE_RENAME_NV2_REDIR_REG(union vector_base, vbar_el1);
	DEFINE_RENAME_NV2_REDIR_REG(struct saved_program_status, spsr_el1);
	DEFINE_RENAME_NV2_REDIR_REG(union exception_link, elr_el1);
	DEFINE_RENAME_NV2_REDIR_REG(union exception_syndrome, esr_el1);
	DEFINE_RENAME_NV2_REDIR_REG(union faulting_address, far_el1);

	// MMU
	DEFINE_RENAME_NV2_REDIR_REG(union indirect_mem_attr, mair_el1);
	DEFINE_NV2_REDIR_REG(struct, tcr_el1);
	DEFINE_RENAME_NV2_REDIR_REG(struct xlate_tbl_base, ttbr0_el1);
	DEFINE_RENAME_NV2_REDIR_REG(struct xlate_tbl_base, ttbr1_el1);

	// Debug
	DEFINE_NV2_REDIR_REG(struct, mdscr_el1);

	//
	// Feature dependent registers
	//

	// FEAT_TCR2
	DEFINE_NV2_REDIR_REG(struct, tcr2_el1);

	// FEAT_SCTLR2
	DEFINE_NV2_REDIR_REG(struct, sctlr2_el1);

	// FEAT_SPE
	DEFINE_NV2_REDIR_REG(struct, pmblimitr_el1);
	DEFINE_NV2_REDIR_REG(struct, pmbptr_el1);
	DEFINE_NV2_REDIR_REG(struct, pmbsr_el1);
	DEFINE_NV2_REDIR_REG(struct, pmscr_el1);
	DEFINE_NV2_REDIR_REG(struct, pmsevfr_el1);
	DEFINE_NV2_REDIR_REG(struct, pmsicr_el1);
	DEFINE_NV2_REDIR_REG(struct, pmsirr_el1);
	DEFINE_NV2_REDIR_REG(struct, pmslatfr_el1);
	DEFINE_NV2_REDIR_REG(struct, pmsnevfr_el1);

	// FEAT_TRF
	DEFINE_NV2_REDIR_REG(struct, trfcr_el1);

	// FEAT_BRBE
	DEFINE_NV2_REDIR_REG(struct, brbcr_el1);

	// FEAT_MPAM
	DEFINE_NV2_REDIR_REG(struct, mpam1_el1);
	DEFINE_NV2_REDIR_REG(struct, mpamvpmv_el2);
	DEFINE_NV2_REDIR_REG(struct, mpamhcr_el2);

	DEFINE_RENAME_NV2_REDIR_REG(mpamvpm_n_el2, mpamvpm0_el2);
	DEFINE_RENAME_NV2_REDIR_REG(mpamvpm_n_el2, mpamvpm1_el2);
	DEFINE_RENAME_NV2_REDIR_REG(mpamvpm_n_el2, mpamvpm2_el2);
	DEFINE_RENAME_NV2_REDIR_REG(mpamvpm_n_el2, mpamvpm3_el2);
	DEFINE_RENAME_NV2_REDIR_REG(mpamvpm_n_el2, mpamvpm4_el2);
	DEFINE_RENAME_NV2_REDIR_REG(mpamvpm_n_el2, mpamvpm5_el2);
	DEFINE_RENAME_NV2_REDIR_REG(mpamvpm_n_el2, mpamvpm6_el2);
	DEFINE_RENAME_NV2_REDIR_REG(mpamvpm_n_el2, mpamvpm7_el2);

	// FEAT_CSV2_2
	DEFINE_NV2_REDIR_REG(struct, scxtnum_el1);

	// FEAT_MTE2
	DEFINE_NV2_REDIR_REG(struct, tfsr_el1);

	// FEAT_SVE
	DEFINE_NV2_REDIR_REG(struct, zcr_el1);

	// FEAT_SME
	DEFINE_NV2_REDIR_REG(struct, smcr_el1);
	DEFINE_NV2_REDIR_REG(struct, smprimap_el2);

	// FEAT_NV2
	DEFINE_NV2_REDIR_REG(struct, vncr_el2);

	// FEAT_VHE
	DEFINE_RENAME_STD_REG(ctx_id, contextidr_el2);
	DEFINE_RENAME_STD_REG(xlate_tbl_base, ttbr1_el2);

	// GIC
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr0_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr1_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr2_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr3_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr4_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr5_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr6_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr7_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr8_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr9_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr10_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr11_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr12_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr13_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr14_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_lr_n_el2, ich_lr15_el2);

	DEFINE_RENAME_NV2_REDIR_REG(ich_ap0r_n_el2, ich_ap0r0_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_ap0r_n_el2, ich_ap0r1_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_ap0r_n_el2, ich_ap0r2_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_ap0r_n_el2, ich_ap0r3_el2);

	DEFINE_RENAME_NV2_REDIR_REG(ich_ap1r_n_el2, ich_ap1r0_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_ap1r_n_el2, ich_ap1r1_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_ap1r_n_el2, ich_ap1r2_el2);
	DEFINE_RENAME_NV2_REDIR_REG(ich_ap1r_n_el2, ich_ap1r3_el2);

	DEFINE_NV2_REDIR_REG(struct, ich_hcr_el2);
	DEFINE_NV2_REDIR_REG(struct, ich_vmcr_el2);

	// FEAT_HCX
	DEFINE_NV2_REDIR_REG(struct, hcrx_el2);

	// FEAT_ECV
	DEFINE_NV2_REDIR_REG(struct, cntpoff_el2);

	// FEAT_FGT
	DEFINE_NV2_REDIR_REG(struct, hfgrtr_el2);
	DEFINE_NV2_REDIR_REG(struct, hfgwtr_el2);
	DEFINE_NV2_REDIR_REG(struct, hfgitr_el2);
	DEFINE_NV2_REDIR_REG(struct, hdfgrtr_el2);
	DEFINE_NV2_REDIR_REG(struct, hdfgwtr_el2);

	// FEAT_FGT && FEAT_AMUv1
	DEFINE_NV2_REDIR_REG(struct, hafgrtr_el2);

	// FEAT_RAS
	DEFINE_NV2_REDIR_REG(struct, vdisr_el2);
	DEFINE_NV2_REDIR_REG(struct, vsesr_el2);

	// FEAT_AMUv1p1
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff00_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff01_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff02_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff03_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff04_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff05_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff06_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff07_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff08_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff09_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff010_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff011_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff012_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff013_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff014_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff0_n_el2, amevcntvoff015_el2);

	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff10_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff11_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff12_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff13_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff14_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff15_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff16_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff17_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff18_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff19_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff110_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff111_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff112_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff113_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff114_el2);
	DEFINE_RENAME_NV2_REDIR_REG(amevcntvoff1_n_el2, amevcntvoff115_el2);

    DEFINE_RENAME_STD_REG(struct vncr_el2, vncr_el2_ipa);
};

#undef DEFINE_RENAME_NV2_REDIR_REG
#undef DEFINE_NV2_REDIR_REG
#undef DEFINE_RENAME_STD_REG
#undef DEFINE_STD_REG
