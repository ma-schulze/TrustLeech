#pragma once

#include <types.hpp>

#define KiB(x) (1024 * x)
#define MiB(x) (1024 * KiB(x))
#define GiB(x) (1024 * MiB(x))
#define TiB(x) (1024 * GiB(x))

#define offsetof(type, member) __builtin_offsetof(type, member)

constexpr bool is_aligned(uintptr_t ptr, size_t alignment) {
	return (ptr % alignment) == 0;
}

inline bool is_aligned(const void *ptr, size_t alignment) {
	return is_aligned(reinterpret_cast<uintptr_t>(ptr), alignment);
}

constexpr uintptr_t align(const uintptr_t ptr, const size_t alignment) {
	return alignment * (ptr / alignment);
}

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
	if (v < lo) {
		return lo;
	}

	if (hi < v) {
		return hi;
	}

	return v;
}

template<typename T>
consteval T as_consteval(T t) {
	return t;
}
