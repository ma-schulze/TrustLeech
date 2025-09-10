#pragma once

#include <types.hpp>

constexpr uint8_t VEL2_VMID = 0;

/** Prepare execution for payloads designed to run on EL2. */
void prepare_vel2();
