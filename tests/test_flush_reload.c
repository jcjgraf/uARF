/**
 * Flush and Reload Side-Channel
 *
 * This tests simple flushing, reloading, and timing primitives, as well as the complete
 * flush and reload functionality.
 *
 */

#include "flush_reload.h"
#include "log.h"
#include "mem.h"
#include "test.h"
#include <stdbool.h>

#define ROUNDS 100

bool is_clflush_supported(void) {
    uint32_t eax = uarf_cpuid_eax(1);
    return eax & (1 << 19);
}

// Check if the CPU has support for clflush instruction
UARF_TEST_CASE(case_clflush) {
    // TODO: should be non-zero
    UARF_TEST_ASSERT(is_clflush_supported());
    UARF_LOG_DEBUG("cl flush is supported\n");

    UARF_TEST_PASS();
}

// Checks what the cached and uncached access times are reasonable
UARF_TEST_CASE(case_flush) {
    int *loc = uarf_malloc_or_die(sizeof(int));

    uint64_t t_c = 0;
    uint64_t t_n = 0;

    for (size_t i = 0; i < ROUNDS; i++) {

        *(volatile int *) loc = 0;

        uarf_mfence();

        t_c += uarf_get_access_time(loc);

        uarf_mfence();

        uarf_clflush(loc);

        uarf_mfence();

        t_n += uarf_get_access_time(loc);
    }

    t_c /= ROUNDS;
    t_n /= ROUNDS;

    UARF_LOG_DEBUG("Cached:   %lu\n", t_c);
    UARF_LOG_DEBUG("Uncached: %lu\n", t_n);

    UARF_TEST_ASSERT(t_c < t_n);
    UARF_TEST_ASSERT(t_c < FR_THRESH);

    uarf_free_or_die(loc);

    UARF_TEST_PASS();
}

// Runs the flush and reload side channel in kernel
UARF_TEST_CASE(flush_reload) {

    UarfFrConfig conf = uarf_fr_init(8, 1, NULL);
    uarf_fr_reset(&conf);

#define SECRET    1
#define NUM_RUNDS 10

    for (size_t i = 0; i < NUM_RUNDS; i++) {
        uarf_fr_flush(&conf);
        *(volatile uint8_t *) (conf.buf.addr + SECRET * FR_STRIDE);
        *(volatile uint8_t *) (conf.buf.addr + (SECRET + 2) * FR_STRIDE);
        uarf_fr_reload(&conf);
    }

    UARF_LOG_DEBUG("FR Buffer\n");
    uarf_fr_print(&conf);

    uarf_fr_deinit(&conf);

    UARF_TEST_PASS();
}

// Flush and reload side channel with only one buffer entry
UARF_TEST_CASE(flush_reload_small) {

    UarfFrConfig conf = uarf_fr_init(1, 1, NULL);
    uarf_fr_reset(&conf);

#define NUM_RUNDS 10
    for (size_t i = 0; i < NUM_RUNDS; i++) {
        uarf_fr_flush(&conf);
        if (i % 2) {
            uarf_mfence();
            uarf_lfence();
            *(volatile uint8_t *) (conf.buf.addr);
        }
        uarf_fr_reload(&conf);
    }

    UARF_LOG_DEBUG("FR Buffer\n");
    uarf_fr_print(&conf);

    uarf_fr_deinit(&conf);

    UARF_TEST_PASS();
}

// Flush and reload side channel with 256 slot buffer
UARF_TEST_CASE(flush_reload_large) {

    UarfFrConfig conf = uarf_fr_init(256, 1, NULL);
    uarf_fr_reset(&conf);

#define NUM_RUNDS 10
    for (size_t i = 0; i < NUM_RUNDS; i++) {
        uarf_fr_flush(&conf);
        if (i % 2) {
            uarf_mfence();
            uarf_lfence();
            *(volatile uint8_t *) (conf.buf.addr);
        }
        uarf_fr_reload(&conf);
    }

    UARF_LOG_DEBUG("FR Buffer\n");
    uarf_fr_print(&conf);

    uarf_fr_deinit(&conf);

    UARF_TEST_PASS();
}

// Do the flush and reload buffer entries have the correct values?
UARF_TEST_CASE(buffer_values) {

    UarfFrConfig conf = uarf_fr_init(8, 1, NULL);
    uarf_fr_reset(&conf);

    for (size_t i = 0; i < conf.num_slots; i++) {
        UARF_TEST_ASSERT(*(uint64_t *) (conf.buf.p + i * FR_STRIDE) == i + 1);
    }

    uarf_fr_deinit(&conf);

    UARF_TEST_PASS();
}

UARF_TEST_CASE(flush_reload_bin) {

    UarfFrConfig conf = uarf_fr_init(32, 6, (size_t[]) {0, 1, 2, 3, 5, 10});
    uarf_fr_reset(&conf);

#define SECRET    1
#define NUM_RUNDS 10

    for (size_t i = 0; i < NUM_RUNDS; i++) {
        uarf_fr_flush(&conf);
        *(volatile uint8_t *) (conf.buf.addr + SECRET * FR_STRIDE);
        if (i % 2 == 0) {
            *(volatile uint8_t *) (conf.buf.addr + (SECRET + 2) * FR_STRIDE);
        }
        else {
            *(volatile uint8_t *) (conf.buf.addr + 31 * FR_STRIDE);
        }
        uarf_fr_reload_binned(&conf, i);
    }

    UARF_LOG_DEBUG("FR Buffer\n");
    uarf_fr_print(&conf);

    uarf_fr_deinit(&conf);

    UARF_TEST_PASS();
}

UARF_TEST_SUITE() {
    UARF_TEST_RUN_CASE(case_clflush);
    UARF_TEST_RUN_CASE(case_flush);
    UARF_TEST_RUN_CASE(flush_reload);
    UARF_TEST_RUN_CASE(flush_reload_small);
    UARF_TEST_RUN_CASE(flush_reload_large);
    UARF_TEST_RUN_CASE(buffer_values);
    UARF_TEST_RUN_CASE(flush_reload_bin);

    return 0;
}
