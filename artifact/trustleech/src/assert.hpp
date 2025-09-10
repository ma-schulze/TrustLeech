#pragma once

#include <type_traits.hpp>
#include <utility.hpp>

[[noreturn]] void die_with_message(const char * exp, const char * func, const char * file, int line);

#ifdef NDEBUG
#define assert(EXP) \
	do { \
		(void)sizeof(EXP); \
	} while (false)
#else
#define assert(EXP) \
	do { \
		if (__builtin_expect(!(EXP), 0)) { \
			die_with_message(#EXP, __func__, __FILE__, __LINE__); \
		} \
	} while (false)
#endif

#define todo(msg) die_with_message("TODO " msg, __func__, __FILE__, __LINE__)

#define die(msg) die_with_message(msg, __func__, __FILE__, __LINE__)

#define assert_size(T, size) \
	static_assert(sizeof(T) == size, #T " does not have the expected size " #size);

#define assert_offset(T, member, offset) \
	static_assert(is_standard_layout_v<T>, #T " is not standard layout, which is required for offsetof"); \
	static_assert(offsetof(T, member) == offset, #T "::" #member " does not have the expected offset " #offset);
