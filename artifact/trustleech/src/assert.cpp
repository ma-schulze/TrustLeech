#include "assert.hpp"
#include <log.hpp>

[[noreturn]] void die_with_message(const char * exp, const char * func, const char * file, int line) {

    LOG_ERROR("Died with error %s in func %s in file %s at line %d", exp, func, file, line);

	while (true) {
		asm volatile ("");
	}
}
