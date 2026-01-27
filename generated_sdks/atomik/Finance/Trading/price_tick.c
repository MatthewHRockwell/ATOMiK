#include "price_tick.h"
#include <string.h>

void atomik_price_tick_init(atomik_price_tick_t *manager) {
    memset(manager, 0, sizeof(atomik_price_tick_t));
}

void atomik_price_tick_load(atomik_price_tick_t *manager, uint64_t initial_state) {
    manager->initial_state = initial_state;
    manager->accumulator = 0;
    manager->history_count = 0;
    manager->history_head = 0;
}

void atomik_price_tick_accumulate(atomik_price_tick_t *manager, uint64_t delta) {
    /* Save to history */
    manager->history[manager->history_head] = delta;
    manager->history_head = (manager->history_head + 1) % ATOMIK_HISTORY_DEPTH;
    if (manager->history_count < ATOMIK_HISTORY_DEPTH) {
        manager->history_count++;
    }

    /* XOR delta into accumulator */
    manager->accumulator ^= delta;
}

uint64_t atomik_price_tick_reconstruct(const atomik_price_tick_t *manager) {
    return manager->initial_state ^ manager->accumulator;
}

bool atomik_price_tick_is_accumulator_zero(const atomik_price_tick_t *manager) {
    return manager->accumulator == 0;
}

size_t atomik_price_tick_rollback(atomik_price_tick_t *manager, size_t count) {
    size_t actual_count = (count < manager->history_count) ? count : manager->history_count;
    
    for (size_t i = 0; i < actual_count; i++) {
        /* Calculate index (going backwards from head) */
        size_t index = (manager->history_head + ATOMIK_HISTORY_DEPTH - 1 - i) % ATOMIK_HISTORY_DEPTH;
        
        /* XOR removes the delta (self-inverse property) */
        manager->accumulator ^= manager->history[index];
    }
    
    /* Update history tracking */
    manager->history_count -= actual_count;
    manager->history_head = (manager->history_head + ATOMIK_HISTORY_DEPTH - actual_count) % ATOMIK_HISTORY_DEPTH;
    
    return actual_count;
}

uint64_t atomik_price_tick_get_accumulator(const atomik_price_tick_t *manager) {
    return manager->accumulator;
}

uint64_t atomik_price_tick_get_initial_state(const atomik_price_tick_t *manager) {
    return manager->initial_state;
}
