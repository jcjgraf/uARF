/**
 * Flush and Reload Side-Channel
 *
 * This tests simple flushing, reloading, and timing primitives, as well as the complete
 * flush and reload functionality.
 *
 */

#include "compiler.h"
#include "flush_reload.h"
#include "log.h"
#include "mem.h"
#include "test.h"
#include <stdbool.h>

bool is_clflush_supported(void) {
    uint32_t eax = cpuid_eax(1);
    return eax & (1 << 19);
}

// Check if the CPU has support for clflush instruction
TEST_CASE(case_clflush) {
    // TODO: should be non-zero
    TEST_ASSERT(is_clflush_supported());
    LOG_DEBUG("cl flush is supported\n");

    TEST_PASS();
}

// Checks what the cached and uncached access times are reasonable
TEST_CASE(case_flush) {
    int *loc = malloc_or_die(sizeof(int));

    uint32_t t_c;
    uint32_t t_n;

    *(volatile int *) loc = 0;

    t_c = get_access_time(loc);
    t_c = get_access_time(loc);

    LOG_DEBUG("Cached: %u\n", t_c);

    mfence();

    clflush(loc);

    mfence();

    t_n = get_access_time(loc);
    LOG_DEBUG("Uncached: %u\n", t_n);

    free_or_die(loc);

    TEST_ASSERT(t_c < t_n);
    TEST_ASSERT(t_c < FR_THRESH);

    TEST_PASS();
}

// Runs the flush and reload side channel in kernel
TEST_CASE(flush_reload) {

    struct FrConfig conf = fr_init(8, 1, NULL);
    fr_reset(&conf);

#define SECRET    1
#define NUM_RUNDS 10

    for (size_t i = 0; i < NUM_RUNDS; i++) {
        fr_flush(&conf);
        *(volatile uint8_t *) (conf.buf.addr + SECRET * FR_STRIDE);
        *(volatile uint8_t *) (conf.buf.addr + (SECRET + 2) * FR_STRIDE);
        fr_reload(&conf);
    }

    LOG_DEBUG("FR Buffer\n");
    fr_print(&conf);

    fr_deinit(&conf);

    TEST_PASS();
}

// Do the flush and reload buffer entries have the correct values?
TEST_CASE(buffer_values) {

    struct FrConfig conf = fr_init(8, 1, NULL);
    fr_reset(&conf);

    for (size_t i = 0; i < conf.num_slots; i++) {
        TEST_ASSERT(*(uint64_t *) (conf.buf.p + i * FR_STRIDE) == i + 1);
    }

    fr_deinit(&conf);

    TEST_PASS();
}

TEST_CASE(flush_reload_bin) {

    struct FrConfig conf = fr_init(32, 6, (size_t[]){0, 1, 2, 3, 5, 10});
    fr_reset(&conf);

#define SECRET    1
#define NUM_RUNDS 10

    for (size_t i = 0; i < NUM_RUNDS; i++) {
        fr_flush(&conf);
        *(volatile uint8_t *) (conf.buf.addr + SECRET * FR_STRIDE);
        if (i % 2 == 0) {
            *(volatile uint8_t *) (conf.buf.addr + (SECRET + 2) * FR_STRIDE);
        }
        else {
            *(volatile uint8_t *) (conf.buf.addr + 31 * FR_STRIDE);
        }
        fr_reload_binned(&conf, i);
    }

    LOG_DEBUG("FR Buffer\n");
    fr_print(&conf);

    fr_deinit(&conf);

    TEST_PASS();
}

TEST_SUITE() {
    RUN_TEST_CASE(case_clflush);
    RUN_TEST_CASE(case_flush);
    RUN_TEST_CASE(flush_reload);
    RUN_TEST_CASE(buffer_values);
    RUN_TEST_CASE(flush_reload_bin);

    TEST_PASS();
}
