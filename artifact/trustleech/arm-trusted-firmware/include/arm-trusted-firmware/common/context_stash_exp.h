#ifndef ARM_TRUSTED_FIRMWARE_EXPORT_COMMON_CONTEXT_STASH_EXP_H
#define ARM_TRUSTED_FIRMWARE_EXPORT_COMMON_CONTEXT_STASH_EXP_H

typedef struct trustleech_context_stash {
	uintptr_t elr_el3;
	uintptr_t sp_el2;
	uint64_t sctlr_el2;
	uint64_t spsr_el3;
} trustleech_context_stash_t;

#endif /* ARM_TRUSTED_FIRMWARE_EXPORT_COMMON_CONTEXT_STASH_EXP_H */
