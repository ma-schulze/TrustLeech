#pragma once


#include "exception/types.hpp"

namespace vmi {

int handle_bp_event(exception_frame *frame);
int handle_ss_event(exception_frame *frame);
int finish_dbg_event();

}

