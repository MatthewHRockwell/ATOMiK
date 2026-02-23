// ATOMiK v3 Bump Allocator with Integrity Tracking — Implementation
#include "atomik_v3_alloc.h"
#include "atomik_v3_hal.h"

// Linker-provided heap boundaries
extern uint64_t _heap_start;
extern uint64_t _heap_end;

// Allocator state
static uint64_t heap_ptr;
static uint64_t heap_limit;
static uint64_t alloc_fingerprint;  // XOR of all allocation headers
static uint32_t alloc_count;

// Allocation header: stored as XOR(address, size) fingerprint component
// No actual header in memory — tracked only via ATOMiK accumulator.

void atomik_heap_init(void)
{
    heap_ptr = (uint64_t)&_heap_start;
    heap_limit = (uint64_t)&_heap_end;
    alloc_fingerprint = 0;
    alloc_count = 0;

    // Initialize ATOMiK bank 0 accumulator for heap tracking
    atomik_load_state(0, 0ULL);
}

void *atomik_malloc(uint32_t size)
{
    // Align to 8 bytes (64-bit alignment)
    size = (size + 7) & ~7;

    if (heap_ptr + size > heap_limit)
        return (void *)0;  // NULL

    uint64_t addr = heap_ptr;
    heap_ptr += size;
    alloc_count++;

    // Track this allocation: XOR (address ^ size) into fingerprint
    uint64_t tag = addr ^ (uint64_t)size;
    atomik_accumulate(0, tag);
    alloc_fingerprint ^= tag;

    return (void *)addr;
}

void atomik_free(void *ptr)
{
    // Bump allocator: free is a no-op
    (void)ptr;
}

int atomik_heap_verify(void)
{
    // The ATOMiK accumulator should contain the running XOR of all allocation tags.
    // In v3, we read the current state to verify
    uint64_t hw_fp = atomik_state(0);
    return hw_fp == alloc_fingerprint;
}

uint32_t atomik_heap_used(void)
{
    return (uint32_t)(heap_ptr - (uint64_t)&_heap_start);
}

uint32_t atomik_heap_total(void)
{
    return (uint32_t)((uint64_t)&_heap_end - (uint64_t)&_heap_start);
}
