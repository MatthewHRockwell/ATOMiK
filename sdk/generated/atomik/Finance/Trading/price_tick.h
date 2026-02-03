#ifndef ATOMIK_FINANCE_TRADING_PRICE_TICK_H
#define ATOMIK_FINANCE_TRADING_PRICE_TICK_H

#include <stdint.h>
#include <stdbool.h>

/* History buffer for rollback */
#define ATOMIK_HISTORY_DEPTH 4096

/**
 * PriceTick - Delta-state manager
 *
 * Manages delta-state operations using XOR algebra.
 */
typedef struct {
    uint64_t initial_state;  /**< Initial state */
    uint64_t accumulator;    /**< Delta accumulator (XOR of all deltas) */
    uint64_t history[ATOMIK_HISTORY_DEPTH];  /**< Delta history */
    size_t history_count;      /**< Number of deltas in history */
    size_t history_head;       /**< History buffer head index */
} atomik_price_tick_t;

/**
 * Initialize a new PriceTick instance
 */
void atomik_price_tick_init(atomik_price_tick_t *manager);

/**
 * Load initial state (LOAD operation)
 */
void atomik_price_tick_load(atomik_price_tick_t *manager, uint64_t initial_state);

/**
 * Accumulate delta (ACCUMULATE operation)
 *
 * XORs the delta into the accumulator.
 */
void atomik_price_tick_accumulate(atomik_price_tick_t *manager, uint64_t delta);

/**
 * Reconstruct current state (READ operation)
 *
 * Returns current_state = initial_state XOR accumulator
 */
uint64_t atomik_price_tick_reconstruct(const atomik_price_tick_t *manager);

/**
 * Check if accumulator is zero (STATUS operation)
 */
bool atomik_price_tick_is_accumulator_zero(const atomik_price_tick_t *manager);

/**
 * Rollback the last N delta operations
 *
 * Returns the number of deltas actually rolled back.
 */
size_t atomik_price_tick_rollback(atomik_price_tick_t *manager, size_t count);

/**
 * Get the current accumulator value
 */
uint64_t atomik_price_tick_get_accumulator(const atomik_price_tick_t *manager);

/**
 * Get the initial state
 */
uint64_t atomik_price_tick_get_initial_state(const atomik_price_tick_t *manager);

#endif /* ATOMIK_FINANCE_TRADING_PRICE_TICK_H */
