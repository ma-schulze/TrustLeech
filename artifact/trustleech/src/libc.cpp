#include <libc.hpp>

#include <types.hpp>
#include <utility.hpp>

extern "C" void memset(void *dst, uint8_t c, size_t len) {
	if (is_aligned(dst, 8) && is_aligned(len, 8)) {
		len /= 8;

		uint64_t cl = static_cast<uint64_t>(c);
		uint64_t combined =
			  cl <<  0 | cl <<  8 | cl << 16 | cl << 24
			| cl << 32 | cl << 40 | cl << 48 | cl << 56;

		uint64_t *ptr = reinterpret_cast<uint64_t*>(dst);
		for (; len; --len, ++ptr) {
			*ptr = combined;
		}
	} else if (is_aligned(dst, 4) && is_aligned(len, 4)) {
		len /= 4;

		uint32_t cl = static_cast<uint64_t>(c);
		uint32_t combined =
			  cl << 0 | cl << 8 | cl << 16 | cl << 24;

		uint32_t *ptr = reinterpret_cast<uint32_t*>(dst);
		for (; len; --len, ++ptr) {
			*ptr = combined;
		}
	} else if (is_aligned(dst, 2) && is_aligned(len, 2)) {
		len /= 2;

		uint16_t cl = static_cast<uint64_t>(c);
		uint16_t combined =
			  cl << 0 | cl << 8;

		uint16_t *ptr = reinterpret_cast<uint16_t*>(dst);
		for (; len; --len, ++ptr) {
			*ptr = combined;
		}
	} else {
		uint8_t *ptr = reinterpret_cast<uint8_t*>(dst);
		for (; len; --len, ++ptr) {
			*ptr = c;
		}
	}
}
