// ATOMiK Bump Allocator with Integrity Tracking
// Uses ATOMiK hardware to fingerprint allocation metadata for corruption detection.
#ifndef ATOMIK_ALLOC_H
#define ATOMIK_ALLOC_H

#include <stdint.h>

// Initialize the heap allocator (call once at startup)
void atomik_heap_init(void);

// Allocate size bytes (4-byte aligned). Returns NULL if out of memory.
void *atomik_malloc(uint32_t size);

// Free is a no-op on a bump allocator. Provided for API completeness.
void atomik_free(void *ptr);

// Verify heap metadata integrity using ATOMiK fingerprint.
// Returns 1 if integrity OK, 0 if corruption detected.
int atomik_heap_verify(void);

// Return bytes used / bytes total
uint32_t atomik_heap_used(void);
uint32_t atomik_heap_total(void);

#endif
