#pragma once

#include <types.hpp>

typedef struct spinlock {
	volatile uint32_t lock;
} spinlock_t;

extern "C" void spin_lock(spinlock_t *lock);
extern "C" void spin_unlock(spinlock_t *lock);

