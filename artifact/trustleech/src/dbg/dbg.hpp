#pragma once

#include <vcpu_ctx.hpp>

namespace dbg
{

void handle_bp(const exception_syndrome esr, exception_frame *frame);
void handle_ss(const exception_syndrome esr, exception_frame *frame);

void set_dbgbcr0_el1(uint64_t val);
void set_dbgbvr0_el1(uint64_t val);

void init_dbg();

void update_bp_config();

}
