#pragma once

#ifndef C_LABEL
/* Define a macro we can use to construct the asm name for a C symbol.  */
# define C_LABEL(name)	name##:
#endif


#ifndef C_SYMBOL_NAME
# define C_SYMBOL_NAME(name) name
#endif

/* Macros to generate eh_frame unwind information.  */
#ifdef __ASSEMBLER__
# define cfi_startproc			.cfi_startproc
# define cfi_endproc			.cfi_endproc
# define cfi_def_cfa(reg, off)		.cfi_def_cfa reg, off
# define cfi_def_cfa_register(reg)	.cfi_def_cfa_register reg
# define cfi_def_cfa_offset(off)	.cfi_def_cfa_offset off
# define cfi_adjust_cfa_offset(off)	.cfi_adjust_cfa_offset off
# define cfi_offset(reg, off)		.cfi_offset reg, off
# define cfi_rel_offset(reg, off)	.cfi_rel_offset reg, off
# define cfi_register(r1, r2)		.cfi_register r1, r2
# define cfi_return_column(reg)	.cfi_return_column reg
# define cfi_restore(reg)		.cfi_restore reg
# define cfi_same_value(reg)		.cfi_same_value reg
# define cfi_undefined(reg)		.cfi_undefined reg
# define cfi_remember_state		.cfi_remember_state
# define cfi_restore_state		.cfi_restore_state
# define cfi_window_save		.cfi_window_save
# define cfi_personality(enc, exp)	.cfi_personality enc, exp
# define cfi_lsda(enc, exp)		.cfi_lsda enc, exp
#endif /* ! ASSEMBLER */

#ifdef __LP64__
# define AARCH64_R(NAME)	R_AARCH64_ ## NAME
# define PTR_REG(n)		x##n
# define PTR_LOG_SIZE		3
# define PTR_ARG(n)
# define SIZE_ARG(n)
#else
# define AARCH64_R(NAME)	R_AARCH64_P32_ ## NAME
# define PTR_REG(n)		w##n
# define PTR_LOG_SIZE		2
# define PTR_ARG(n)		mov     w##n, w##n
# define SIZE_ARG(n)		mov     w##n, w##n
#endif

/* Define an entry point visible from C.  */
#define ENTRY(name) \
  .globl C_SYMBOL_NAME(name); \
  .type C_SYMBOL_NAME(name),%function; \
  .p2align 6; \
  C_LABEL(name) \
  cfi_startproc;

#define END(name)						\
  cfi_endproc;

/* Local label name for asm code.  */
#ifndef L
# define L(name)         .L##name
#endif
