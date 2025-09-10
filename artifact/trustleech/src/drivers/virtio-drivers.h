#pragma once 

#include <types.hpp>

extern "C" uint64_t device_enumerate(uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3);

extern "C" void rs_log(int log_level, const char *str);
