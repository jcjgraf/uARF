/**
 * Are caches still indexed in the same way?
 *
 * Are the set bit still valid, or do they use some sort of hash?
 *
 * Allocate a huge eviction set for a victim, then reduced it by dropping elements that
 * are not critical. Observe the VAs of the final elements.
 *
 * ## L1
 * - 32KB
 * - 8 Way
 * - 2^6 = 64 sets
 * - uses UTAG
 * - uarf_get_access_time2 of 49
 *
 * ## L2
 * - 1 MB (or 512 KB)
 * - 8 Way
 * - 2^11 = 2048 sets
 * - uarf_get_access_time2 of 56
 * - L1 is inclusive in L2
 * - uses some very advanced replacement policy (not LRU)
 *
 * ## L3
 * - Up to 32 MB
 * - 16 Way
 * - Up to 2^15 = 32768 sets
 * - Slices
 * - Shared among 8 cores
 * - Not inclusive
 * - May use some advanced replacement policy
 */

#include "lib.h"
#include "mem.h"
#include "test.h"

const uint8_t CACHE_LINE_BITS = 6;
const uint8_t CACHE_LINE_SIZE = (1 << CACHE_LINE_BITS);

const uint8_t L1_SET_BITS = 6;
const uint8_t L1_WAYS = 8;
const uint8_t L2_SET_BITS = 11;
const uint8_t L2_WAYS = 8;
const uint8_t L3_SET_BITS = 15;
const uint8_t L3_WAYS = 16;

volatile uint64_t a[128];
volatile uint64_t victim;
volatile uint64_t b[128];

/**
 * Get the cache set bits of p.
 */
uint64_t get_set_bits(void *addr, uint8_t num_set_bits) {
    uint64_t mask = ((1ul << num_set_bits) - 1) << CACHE_LINE_BITS;
    return _ul(addr) & mask;
}

/**
 * Get the cache set that `addr` maps to, assuming `num_set_bits` are used.
 */
uint64_t get_set(void *addr, uint8_t num_set_bits) {
    return get_set_bits(addr, num_set_bits) >> 6;
}

/**
 * Get the L1 cache set that `addr` maps to.
 */
uint64_t get_l1_set(void *addr) {
    return get_set(addr, L1_SET_BITS);
}

/**
 * Get the L2 cache set that `addr` maps to.
 */
uint64_t get_l2_set(void *addr) {
    return get_set(addr, L2_SET_BITS);
}

/**
 * Get the L3 cache set that `addr` maps to.
 */
uint64_t get_l3_set(void *addr) {
    return get_set(addr, L3_SET_BITS);
}

/**
 * Access all `es_size` members of `es` forward, backward, forward and repeat `num_rep`
 * times.
 */
void es_access(uint64_t es[], size_t es_size, size_t num_rep) {
    for (size_t round = 0; round < num_rep; round++) {
        // Forward access
        for (size_t i = 0; i < es_size; i++) {
            if (es[i])
                *(volatile uint64_t *) es[i];
        }
        // Backward access
        for (size_t i = es_size - 1; i; i--) {
            if (es[i])
                *(volatile uint64_t *) es[i];
        }
        // Forward access
        for (size_t i = 0; i < es_size; i++) {
            if (es[i])
                *(volatile uint64_t *) es[i];
        }
    }
}

/**
 * Allocate and return pointer to an page at an arbitrary address.
 *
 * Simply using malloc does not allocate them at random addresses.
 */
void *alloc_random_page(void) {
#define MMAP_FLAGS_FIX_SOFT (MMAP_FLAGS | MAP_FIXED)
    void *map;
    do {
        uint64_t cand_addr = uarf_rand47() & ~((1 << 12) - 1);
        map = mmap(_ptr(cand_addr), 4096, PROT_RWX, MMAP_FLAGS_FIX_SOFT, -1, 0);
    } while (map == MAP_FAILED);
    return map;
}

/**
 * Create eviction set by allocating as many pages needed to add `es_size` CL aligned
 * addresses in those pages into `es`.
 */
void es_init(uint64_t es[], size_t es_size) {
    // TODO: make work with 4k and 2M pages
    // TODO: Randomize CL offset to confuse prefetcher?
    size_t i = 0;
    while (i < es_size) {
        void *map = alloc_random_page();

        // Add all CL aligned offsets into buffer
        size_t page_offset = 0;
        for (size_t page_i = 0; page_i < CACHE_LINE_SIZE && i < es_size; page_i++, i++) {
            uint64_t map_offset = (page_offset + (page_i * 8)) % 4096;
            es[i] = _ul(map + map_offset);
            page_offset += CACHE_LINE_SIZE;
        }
    };
}

/**
 * Indicate whether the victim is in L1 or not
 */
bool is_in_l1(void *victim) {
    // 49 is the access time of uarf_get_access_time2 (including some overhead of 48)
    return uarf_get_access_time2(victim) <= 49;
}

/**
 * How effective is es in evicting `victim` from l1?
 *
 * 1: pefect, 0: not at all
 */
float es_l1_effectiveness(uint64_t *es, size_t es_size, void *victim) {
    const size_t REPS = 1000;

    uint64_t num_evicted = 0;

    for (size_t i = 0; i < REPS; i++) {
        *(volatile uint64_t *) &victim;
        es_access(es, es_size, 1);
        num_evicted += is_in_l1(_ptr(victim)) ? 0 : 1;
    }
    UARF_LOG_DEBUG("ES evicted L1 %lu/%lu times\n", num_evicted, REPS);

    return (float) num_evicted / REPS;
}

UARF_TEST_CASE(l1_eviction) {

    const uint64_t ES_SIZE_INIT = 70;

    uint64_t *es = uarf_malloc_or_die(ES_SIZE_INIT * sizeof(uint64_t));

    uint64_t victim_l1_set = get_l1_set(_ptr(&victim));
    UARF_LOG_INFO("Victim maps into L1 cache set %lu\n", victim_l1_set);

    // Build eviction set for victim L1
    for (size_t i = 0; i < ES_SIZE_INIT; i++) {
        uint64_t map = _ul(alloc_random_page());
        es[i] = map + (victim_l1_set << CACHE_LINE_BITS);
        uarf_assert(get_l1_set(_ptr(es[i])) == victim_l1_set);
    }

    // es_init(es, ES_SIZE_INIT);

    size_t es_effective_size = ES_SIZE_INIT;

    for (size_t i = 0; i < 10; i++) {

        for (size_t es_kill_cand_i = 0; es_kill_cand_i < ES_SIZE_INIT; es_kill_cand_i++) {
            bool es_effectiveness = es_l1_effectiveness(es, ES_SIZE_INIT, _ptr(&victim));

            if (es_effectiveness <= 0.5) {
                UARF_LOG_WARNING("Non-effective ES detection. Should not happen\n");
                uarf_assert(0);
                return 1;
            }

            UARF_LOG_DEBUG("Check candidate %lu\n", es_kill_cand_i);
            uint64_t es_kill_cand_val = es[es_kill_cand_i];
            es[es_kill_cand_i] = 0;

            if (es_l1_effectiveness(es, ES_SIZE_INIT, _ptr(&victim))) {
                // Sanity check
                uarf_assert(es_l1_effectiveness(es, ES_SIZE_INIT, _ptr(&victim)));
                UARF_LOG_INFO("-- %lu is not relevant, remove\n", es_kill_cand_i);
                es_effective_size--;
                uarf_assert(es_effective_size > 0);
            }
            else {
                // UARF_LOG_INFO("-- Is relevant, re-add\n");
                es[es_kill_cand_i] = es_kill_cand_val;
            }
        }

        UARF_LOG_INFO("Reduced ES to %lu elements\n", es_effective_size);
    }

    // TODO: free ES pages and es
    uarf_free_or_die(es);

    UARF_TEST_PASS();
}

UARF_TEST_SUITE() {

    uarf_log_system_base_level = UARF_LOG_LEVEL_DEBUG;

    UARF_TEST_RUN_CASE(l1_eviction);

    UARF_TEST_PASS();
}
