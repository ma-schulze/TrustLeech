#pragma once

#include <assert.hpp>
#include <types.hpp>

constexpr int popcount(unsigned long long x) {
	return __builtin_popcountll(x);
}

constexpr bool has_single_bit(unsigned long long x) {
	return popcount(x) == 1;
}

constexpr int countr_zero(unsigned long long x) {
	if (x == 0) {
		return 0;
	}

	return __builtin_ctzll(x);
}

constexpr int countr_one(unsigned long long x) {
	return __builtin_ctzll(~x);
}

constexpr uint64_t bit_extract(uint64_t v, uint8_t msb, uint8_t lsb) {
	assert(msb >= lsb);
	assert(msb < (8 * sizeof(v)));

	const uint8_t mask_width = (msb - lsb) + 1;
	const uintptr_t mask = (uintptr_t(1) << mask_width) - 1;
	return (v >> lsb) & mask;
}

constexpr uintptr_t bit_extract(uintptr_t v, uint8_t msb, uint8_t lsb) {
	static_assert(sizeof(uintptr_t) == sizeof(uint64_t));
	return bit_extract(uint64_t(v), msb, lsb);
}
