#pragma once

#include <exception/types.hpp>
#include <registers.hpp>

/** Deinitialize TrustLeech on this CPU
 *
 * This function restores the context setup by setup_cpu() and disables
 * TrustLeech by calling into the firmware.
 */
[[noreturn]] void teardown_cpu(const exception_frame* frame);

