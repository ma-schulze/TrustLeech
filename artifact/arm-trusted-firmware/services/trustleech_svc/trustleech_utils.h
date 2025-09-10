#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>
#include <arch_helpers.h>

struct atomic64_t {
    long counter;
} __attribute__((packed));

#define ATOMIC64_OP(op, asm_op)                        \
static inline void atomic64_##op(long i, volatile struct atomic64_t *v)            \
{                                    \
    long result;                            \
    unsigned long tmp;                        \
                                    \
    asm volatile("// atomic64_" #op "\n"                \
"1:    ldxr    %0, %2\n"                        \
"    " #asm_op "    %0, %0, %3\n"                    \
"    stxr    %w1, %0, %2\n"                        \
"    cbnz    %w1, 1b"                        \
    : "=&r" (result), "=&r" (tmp), "+Q" (v->counter)        \
    : "Ir" (i));                            \
}

#define ATOMIC64_OP_RETURN(op, asm_op)                    \
static inline long atomic64_##op##_return(long i, volatile struct atomic64_t *v)    \
{                                    \
    long result;                            \
    unsigned long tmp;                        \
                                    \
    asm volatile("// atomic64_" #op "_return\n"            \
"1:    ldxr    %0, %2\n"                        \
"    " #asm_op "    %0, %0, %3\n"                    \
"    stlxr    %w1, %0, %2\n"                        \
"    cbnz    %w1, 1b"                        \
    : "=&r" (result), "=&r" (tmp), "+Q" (v->counter)        \
    : "Ir" (i)                            \
    : "memory");                            \
                                    \
    dmbish();                            \
    return result;                            \
}

ATOMIC64_OP(add, add)
ATOMIC64_OP_RETURN(add, add)

#define atomic64_inc(v)            atomic64_add(1LL, (v))
#define atomic64_inc_return(v)        atomic64_add_return(1LL, (v))

#endif

