#ifndef ARM_H
#define ARM_H

#include "stdint.h"

#define Q(a) \
    #a

#define mrc(coproc, opcode1, reg, subReg, opcode2) \
({\
    uint32_t r; \
    asm volatile("mrc " Q(coproc) ", " Q(opcode1) ", %0, " Q(reg) ", " Q(subReg) ", " Q(opcode2) : "=r" (r) :: "memory"); \
    r;\
})

#define mcr(coproc, opcode1, reg, subReg, opcode2, value) \
({\
    asm volatile("mcr " Q(coproc) ", " Q(opcode1) ", %0, " Q(reg) ", " Q(subReg) ", " Q(opcode2) : : "r" (value) : "memory"); \
})

inline void irq_enable() {
    asm volatile("cpsie i");
}

inline void irq_disable() {
    asm volatile("cpsid i");
}

inline void tlb_flush_all() {
    mcr(p15, 0, c8, c7, 0, 0);
}

inline static void tlb_invalidate(uint32_t page) {
    mcr(p15, 0, c8, c7, 1, page);
}

inline void dmb() {
    mcr(p15, 0, c7, c10, 5, 0);
}

inline void dsb() {
#ifdef RPI2
    asm volatile ("dsb" ::: "memory");
#else
    mcr(p15, 0, c7, c10, 4, 0);
#endif
}

inline void flush_prefetch_buffer() {
#ifdef RPI2

#else
    mcr(p15, 0, c7, c5, 4, 0);
#endif
}

inline void flush_branch_prediction() {
    mcr(p15, 0, c7, c5, 6, 0);
}

inline void isb() {
#ifdef RPI2
    asm volatile ("isb" ::: "memory");
#else
    mcr(p15, 0, c7, c5, 4, 0);
#endif
}

uint32_t get_cache_level_id() {
    return mrc(p15, 1, c0, c0, 1);
}

uint32_t read_cache_size(uint32_t level, uint32_t type) {
    uint32_t sel = level << 1 | type;
    mcr(p15, 2, c0, c0, 0, sel); // select which cache we're working on
    return mrc(p15, 1, c0, c0, 0);
}

static inline uint32_t log_2_n_round_up(uint32_t n)
{
    uint32_t log2n = -1;
    uint32_t temp = n;

    while (temp) {
        log2n++;
        temp >>= 1;
    }

    if (n & (n - 1))
        return log2n + 1; /* not power of 2 - round up */
    else
        return log2n; /* power of 2 */
}

#include "stdbool.h"
#define CCSIDR_LINE_SIZE_OFFSET          0
#define CCSIDR_LINE_SIZE_MASK            0x7
#define CCSIDR_ASSOCIATIVITY_OFFSET      3
#define CCSIDR_ASSOCIATIVITY_MASK        (0x3FF << 3)
#define CCSIDR_NUM_SETS_OFFSET           13
#define CCSIDR_NUM_SETS_MASK             (0x7FFF << 13)

void flush_cache_level(uint32_t level, bool clean) {
    uint32_t ccsidr = read_cache_size(level, 0);
    int way, set, setway;
    uint32_t log2_line_len = ((ccsidr & CCSIDR_LINE_SIZE_MASK) >>
                                   CCSIDR_LINE_SIZE_OFFSET) + 2;

    // Converting from words to bytes
    log2_line_len += 2;

    uint32_t num_ways  = ((ccsidr & CCSIDR_ASSOCIATIVITY_MASK) >>
                               CCSIDR_ASSOCIATIVITY_OFFSET) + 1;
    uint32_t num_sets  = ((ccsidr & CCSIDR_NUM_SETS_MASK) >>
                               CCSIDR_NUM_SETS_OFFSET) + 1;
    //
    // According to ARMv7 ARM number of sets and number of ways need
    // not be a power of 2
    //
    uint32_t log2_num_ways = log_2_n_round_up(num_ways);
    uint32_t way_shift     = (32 - log2_num_ways);

    // Invoke the Clean & Invalidate cache line by set/way on the CP15.
    for (way = num_ways - 1; way >= 0 ; way--)
    {
        for (set = num_sets - 1; set >= 0; set--)
        {
            setway = (level << 1) | (set << log2_line_len) |
                     (way << way_shift);
            //
            // Clean & Invalidate data/unified
            // cache line by set/way
            //
            if (clean)
            {
                asm volatile (" mcr p15, 0, %0, c7, c14, 2"
                                : : "r" (setway));
            }
            else
            {
                asm volatile (" mcr p15, 0, %0, c7, c6, 2"
                                : :  "r" (setway));
            }
        }
    }
    dsb();
    return;
}

#define CACHE_LEVEL_INSTRUCTION         1
#define CACHE_LEVEL_DATA                2
#define CACHE_LEVEL_INSTRUCTION_DATA    3
#define CACHE_LEVEL_UNIFIED             4

void flush_data_cache(bool clean) {
    uint32_t levelId = get_cache_level_id();
    uint32_t cacheType, startBit = 0;

    for (uint32_t level = 0; level < 7; level++)
    {
        cacheType = (levelId >> startBit) & 7;
        if (cacheType == CACHE_LEVEL_INSTRUCTION ||
            cacheType == CACHE_LEVEL_DATA ||
            cacheType == CACHE_LEVEL_UNIFIED)
        {
            flush_cache_level(level, clean);
        }
        startBit += 3;
    }
}

void flush_instruction_cache() {
    dsb();
    mcr(p15, 0, c7, c5, 0, 0);
    flush_branch_prediction();
    isb();
}

#endif
