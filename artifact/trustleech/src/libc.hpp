#pragma once

#include <types.hpp>

extern "C" void memset(void *dst, uint8_t c, size_t len);
extern "C" void* memcpy(void *__restrict__ dest, const void *__restrict__ src, size_t n);
