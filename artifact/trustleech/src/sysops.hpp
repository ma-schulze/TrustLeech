#pragma once

#include <types.hpp>

#define COMPILER_BARRIER() __asm__ volatile ("" ::: "memory")

/**********************************************************************
 * Macros to create inline functions for system instructions
 *********************************************************************/

/* Define function for simple system instruction */
#define DEFINE_SYSOP_FUNC(op) \
	static inline void op(void) \
	{ \
		__asm__ (#op : : : "memory"); \
	}

/* Define function for system instruction with type specifier */
#define DEFINE_SYSOP_TYPE_FUNC(op, type) \
static inline void op ## _ ## type(void) \
	{ \
		__asm__ (#op " " #type : : : "memory"); \
	}

/* Define function for system instruction with type specifier and parameter */
#define DEFINE_SYSOP_TYPE_PARAM_FUNC(op, type) \
static inline void op ## _ ## type(uint64_t val) \
	{ \
		__asm__ (#op " " #type ", %0" : : "r" (val) : "memory"); \
	}


DEFINE_SYSOP_FUNC(isb);
DEFINE_SYSOP_TYPE_FUNC(dsb, sy);
DEFINE_SYSOP_TYPE_FUNC(tlbi, alle2);
DEFINE_SYSOP_TYPE_PARAM_FUNC(tlbi, vae1os);
DEFINE_SYSOP_TYPE_PARAM_FUNC(tlbi, ipas2e1is);
DEFINE_SYSOP_TYPE_FUNC(tlbi, vmalle1is);

DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s12e1r)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s12e1w)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s12e0r)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s12e0w)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s1e1r)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s1e2r)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s1e3r)

  
#define AT(_at_inst, _va)	_at_inst(_va)
