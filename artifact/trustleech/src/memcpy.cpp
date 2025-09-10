#include "types.hpp"

extern "C" void* memcpy(void *vdst, const void *vsrc, size_t len)
{
	if (((uintptr_t) vdst % 8) == 0
			&& ((uintptr_t) vsrc % 8) == 0
			&& (len % 32) == 0) {
		uint64_t *dst = (uint64_t *) vdst;
		const uint64_t *src = (const uint64_t *) vsrc;

		len /= 32;
		while (len) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = src[3];
			dst += 4;
			src += 4;
			len--;
		}
	} else if (((uintptr_t) vdst % 8) == 0
	        && ((uintptr_t) vsrc % 8) == 0
	        && (len % 16) == 0) {
		uint64_t *dst = (uint64_t *) vdst;
		const uint64_t *src = (const uint64_t *) vsrc;

		len /= 16;
		while (len) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst += 2;
			src += 2;
			len--;
		}
	} else if (((uintptr_t) vdst % 8) == 0
	        && ((uintptr_t) vsrc % 8) == 0
	        && (len % 8) == 0) {
		uint64_t *dst = (uint64_t *) vdst;
		const uint64_t *src = (const uint64_t *) vsrc;

		len /= 8;
		while (len) {
			*dst = *src;
			dst++;
			src++;
			len--;
		}
	} else {
		uint8_t *dst = (uint8_t *) vdst;
		const uint8_t *src = (const uint8_t *) vsrc;

		while (len) {
			*dst = *src;
			dst++;
			src++;
			len--;
		}
	}

	return vdst;
}

extern "C" void * memmove(void *dst, const void *src, size_t len)
{
	char *d = (char *) dst;
	const char *s = (const char *) src;

	if (d == s) {
		return d;
	}

	if (d < s) {
		for (size_t i = 0; i < len; i++) {
			d[i] = s[i];
		}
	} else {
		for (size_t i = len; 0 < i; i--) {
			d[i - 1] = s[i - 1];
		}
	}

	return (void *) d;
}
