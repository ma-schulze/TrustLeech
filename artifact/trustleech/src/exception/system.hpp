#pragma once

#include <registers.hpp>

struct exception_frame;

namespace exception {
	void handle_system(const exception_syndrome esr, exception_frame* frame);
}
