#ifndef ATOMIK_VIDEO_STREAMING_H264_DELTA_H
#define ATOMIK_VIDEO_STREAMING_H264_DELTA_H

#include <stdint.h>
#include <stdbool.h>

/* History buffer for rollback */
#define ATOMIK_HISTORY_DEPTH 512

/**
 * H264Delta - Delta-state manager
 *
 * Manages delta-state operations using XOR algebra.
 */
typedef struct {
    uint64_t initial_state;  /**< Initial state */
    uint64_t accumulator;    /**< Delta accumulator (XOR of all deltas) */
    uint64_t history[ATOMIK_HISTORY_DEPTH];  /**< Delta history */
    size_t history_count;      /**< Number of deltas in history */
    size_t history_head;       /**< History buffer head index */
} atomik_h264_delta_t;

/**
 * Initialize a new H264Delta instance
 */
void atomik_h264_delta_init(atomik_h264_delta_t *manager);

/**
 * Load initial state (LOAD operation)
 */
void atomik_h264_delta_load(atomik_h264_delta_t *manager, uint64_t initial_state);

/**
 * Accumulate delta (ACCUMULATE operation)
 *
 * XORs the delta into the accumulator.
 */
void atomik_h264_delta_accumulate(atomik_h264_delta_t *manager, uint64_t delta);

/**
 * Reconstruct current state (READ operation)
 *
 * Returns current_state = initial_state XOR accumulator
 */
uint64_t atomik_h264_delta_reconstruct(const atomik_h264_delta_t *manager);

/**
 * Check if accumulator is zero (STATUS operation)
 */
bool atomik_h264_delta_is_accumulator_zero(const atomik_h264_delta_t *manager);

/**
 * Rollback the last N delta operations
 *
 * Returns the number of deltas actually rolled back.
 */
size_t atomik_h264_delta_rollback(atomik_h264_delta_t *manager, size_t count);

/**
 * Get the current accumulator value
 */
uint64_t atomik_h264_delta_get_accumulator(const atomik_h264_delta_t *manager);

/**
 * Get the initial state
 */
uint64_t atomik_h264_delta_get_initial_state(const atomik_h264_delta_t *manager);

#endif /* ATOMIK_VIDEO_STREAMING_H264_DELTA_H */
