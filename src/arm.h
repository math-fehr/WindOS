#ifndef ARM_H
#define ARM_H

/** \file arm.h
 *	\brief ARM specifically related code
 *
 * 	This file contains maintenance functions and methods to access ARM special
 *	registers. 
 */


#include "stdint.h"

#define SETWAY_LEVEL_SHIFT		1

#define L1_DATA_CACHE_SETS		    128
#define L1_DATA_CACHE_WAYS	        4
#define L1_SETWAY_WAY_SHIFT		    30
#define L1_DATA_CACHE_LINE_LENGTH	64
#define L1_SETWAY_SET_SHIFT		    6

//#ifdef RPI2
#define L2_CACHE_SETS			1024
#define L2_CACHE_WAYS			8
#define L2_SETWAY_WAY_SHIFT		29
//#endif

#define L2_CACHE_LINE_LENGTH		64
#define L2_SETWAY_SET_SHIFT		    6

#define DATA_CACHE_LINE_LENGTH_MIN	64

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
    __asm volatile ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory");
}

inline void dsb() {
    __asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory");
}

inline void flush_prefetch_buffer() {
#ifdef RPI2
    asm volatile("isb" ::: "memory");
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

inline uint32_t get_cache_level_id() {
    return mrc(p15, 1, c0, c0, 1);
}

inline uint32_t read_cache_size(uint32_t level, uint32_t type) {
    uint32_t sel = level << 1 | type;
    mcr(p15, 2, c0, c0, 0, sel); // select which cache we're working on
    return mrc(p15, 1, c0, c0, 0);
}

inline uint32_t log_2_n_round_up(uint32_t n)
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

inline void flush_cache_level(uint32_t level, bool clean) {
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

inline void flush_data_cache(bool clean) {
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

inline void flush_instruction_cache() {
    dsb();
    mcr(p15, 0, c7, c5, 0, 0);
    flush_branch_prediction();
    isb();
}


inline void cleanDataCache() {
    for(register unsigned set = 0; set < L1_DATA_CACHE_SETS; set++)
	{
		for (register unsigned way = 0; way < L1_DATA_CACHE_WAYS; way++)
		{
			register uint32_t setWayLevel =   way << L1_SETWAY_WAY_SHIFT
                | set << L1_SETWAY_SET_SHIFT
                | 0 << SETWAY_LEVEL_SHIFT;

			__asm volatile ("mcr p15, 0, %0, c7, c10,  2" : : "r" (setWayLevel) : "memory");	// DCCSW
		}
	}

    #ifndef RPI2
    return;
    #endif

	// clean L2 unified cache
	for (register unsigned set = 0; set < L2_CACHE_SETS; set++)
	{
		for (register unsigned way = 0; way < L2_CACHE_WAYS; way++)
		{
			register uint32_t setWayLevel =   way << L2_SETWAY_WAY_SHIFT
                | set << L2_SETWAY_SET_SHIFT
                | 1 << SETWAY_LEVEL_SHIFT;

			__asm volatile ("mcr p15, 0, %0, c7, c10,  2" : : "r" (setWayLevel) : "memory");	// DCCSW
		}
	}
}

inline void invalidateInstructionCache() {
    asm volatile("mcr p15, 0, %0, c7, c5, 0" :: "r" (0) : "memory");
}


#endif
