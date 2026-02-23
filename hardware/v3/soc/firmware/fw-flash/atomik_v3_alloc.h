// ATOMiK v3 Bump Allocator with Integrity Tracking
// Adapted for RV64I with 64-bit fingerprinting.

#ifndef ATOMIK_V3_ALLOC_H
#define ATOMIK_V3_ALLOC_H

#include <stdint.h>

// Initialize heap allocator
void atomik_heap_init(void);

// Allocate memory (bump allocator, no free)
void *atomik_malloc(uint32_t size);

// Free memory (no-op for bump allocator)
void atomik_free(void *ptr);

// Verify heap metadata integrity via ATOMiK fingerprint
int atomik_heap_verify(void);

// Get heap usage statistics
uint32_t atomik_heap_used(void);
uint32_t atomik_heap_total(void);

#endif
