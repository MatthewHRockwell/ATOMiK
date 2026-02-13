// ATOMiK Bump Allocator with Integrity Tracking — Implementation
#include "atomik_alloc.h"
#include "atomik.h"

// Linker-provided heap boundaries
extern uint32_t _heap_start;
extern uint32_t _heap_end;

// Allocator state
static uint32_t heap_ptr;
static uint32_t heap_limit;
static uint32_t alloc_fingerprint;  // XOR of all allocation headers
static uint32_t alloc_count;

// Allocation header: stored as XOR(address, size) fingerprint component
// No actual header in memory — tracked only via ATOMiK accumulator.

void atomik_heap_init(void)
{
    heap_ptr = (uint32_t)&_heap_start;
    heap_limit = (uint32_t)&_heap_end;
    alloc_fingerprint = 0;
    alloc_count = 0;

    // Initialize ATOMiK bank 0 accumulator for heap tracking
    atomik_load(0, 0);
}

void *atomik_malloc(uint32_t size)
{
    // Align to 4 bytes
    size = (size + 3) & ~3;

    if (heap_ptr + size > heap_limit)
        return (void *)0;  // NULL

    uint32_t addr = heap_ptr;
    heap_ptr += size;
    alloc_count++;

    // Track this allocation: XOR (address ^ size) into fingerprint
    uint32_t tag = addr ^ size;
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
    // Compare hardware accumulator value with our software-tracked fingerprint.
    uint32_t hw_fp = atomik_get_delta(0);
    return hw_fp == alloc_fingerprint;
}

uint32_t atomik_heap_used(void)
{
    return heap_ptr - (uint32_t)&_heap_start;
}

uint32_t atomik_heap_total(void)
{
    return (uint32_t)&_heap_end - (uint32_t)&_heap_start;
}
