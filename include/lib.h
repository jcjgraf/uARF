#pragma once

#include "compiler.h"
#include "uarf_pi/uarf_pi.h"
#include <stdint.h>
#include <stdlib.h>

#define sfence() asm volatile("sfence" ::: "memory")
#define lfence() asm volatile("lfence" ::: "memory")
#define mfence() asm volatile("mfence" ::: "memory")

static inline uint32_t get_seed(void) {
    uint32_t seed;
    asm("rdrand %0" : "=r"(seed));
    return seed;
}

// Get a random 47 bit long number
static inline uint64_t rand47(void) {
    return (_ul(rand()) << 16) ^ rand();
}

static inline void clflush(const volatile void *p) {
    asm volatile("clflush %0" ::"m"(*(char const *) p) : "memory");
}

static inline void prefetchw(const void *p) {
    asm volatile("prefetchw %0" ::"m"(*(char const *) p) : "memory");
}

static inline void prefetcht0(const void *p) {
    asm volatile("prefetcht0 %0" ::"m"(*(char const *) p) : "memory");
}

static inline void prefetcht1(const void *p) {
    asm volatile("prefetcht1 %0" ::"m"(*(char const *) p) : "memory");
}

static inline void prefetcht2(const void *p) {
    asm volatile("prefetcht2 %0" ::"m"(*(char const *) p) : "memory");
}

static inline void prefetchnta(const void *p) {
    asm volatile("prefetchnta %0" ::"m"(*(char const *) p) : "memory");
}

static inline void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx,
                         uint32_t *edx) {
    asm volatile("cpuid"
                 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                 : "0"(leaf), "1"(*ebx), "2"(*ecx), "3"(*edx));
}

static inline uint32_t cpuid_eax(uint32_t leaf) {
    uint32_t eax = 0, ign = 0;

    cpuid(leaf, &eax, &ign, &ign, &ign);
    return eax;
}

static inline uint32_t cpuid_ebx(uint32_t leaf) {
    uint32_t ebx = 0, ign = 0;

    cpuid(leaf, &ign, &ebx, &ign, &ign);
    return ebx;
}

static inline uint32_t cpuid_ecx(uint32_t leaf) {
    uint32_t ecx = 0, ign = 0;

    cpuid(leaf, &ign, &ign, &ecx, &ign);
    return ecx;
}

static inline uint32_t cpuid_edx(uint32_t leaf) {
    uint32_t edx = 0, ign = 0;

    cpuid(leaf, &ign, &ign, &ign, &edx);
    return edx;
}

static inline uint64_t rdmsr(uint32_t msr_idx) {
    return pi_rdmsr(msr_idx);
}

static inline void wrmsr(uint32_t msr_idx, uint64_t value) {
    pi_wrmsr(msr_idx, value);
}

static inline void invlpg(void *addr) {
    pi_invlpg(_ul(addr));
}

static inline void flush_tlb(void) {
    pi_flush_tlb();
}

#define ASSERT(cond)                                                                     \
    do {                                                                                 \
        if (!(cond)) {                                                                   \
            LOG_WARNING("%s: Assert at %d failed: %s\n", __func__, __LINE__,             \
                        STR((cond)));                                                    \
            exit(1);                                                                     \
        }                                                                                \
    } while (0)

#define BUG() exit(1)

static inline uint64_t rdtsc(void) {
    unsigned int low, high;

    asm volatile("rdtsc" : "=a"(low), "=d"(high));

    return ((uint64_t) high << 32) | low;
}

static inline uint64_t rdtscp(void) {
    unsigned int low, high;

    asm volatile("rdtscp" : "=a"(low), "=d"(high)::"ecx");

    return ((uint64_t) high << 32) | low;
}

// Get the number of cycles needed to read from `p`
static inline uint64_t get_access_time(const void *p) {
    // TODO: Check right use of rdtsc(p) and barriers
    mfence();
    uint64_t t0 = rdtsc();
    *(volatile uint64_t *) p;
    t0 = rdtscp() - t0;
    mfence();
    return t0;
}
