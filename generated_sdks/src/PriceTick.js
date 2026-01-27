/**
 * PriceTick - Delta-state module
 * 
 * Generated from ATOMiK schema
 * Vertical: Finance
 * Field: Trading
 * 
 * @module @atomik/finance/trading
 */

/**
 * PriceTick delta-state manager
 * 
 * Manages delta-state operations using XOR algebra.
 */
export class PriceTick {
    /**
     * Create a new PriceTick instance
     */
    constructor() {
        /** @type {BigInt} */
        this.initialState = 0n;
        /** @type {BigInt} */
        this.accumulator = 0n;
        /** @type {BigInt[]} */
        this.history = [];
        /** @type {number} */
        this.maxHistory = 4096;
    }

    /**
     * Load initial state (LOAD operation)
     * 
     * @param {BigInt} initialState - Initial state value
     */
    load(initialState) {
        this.initialState = initialState;
        this.accumulator = 0n;
        this.history = [];
    }

    /**
     * Accumulate delta (ACCUMULATE operation)
     * 
     * XORs the delta into the accumulator.
     * 
     * @param {BigInt} delta - Delta value to accumulate
     */
    accumulate(delta) {
        // Save to history
        this.history.push(delta);
        if (this.history.length > this.maxHistory) {
            this.history.shift();
        }
        // XOR delta into accumulator
        this.accumulator ^= delta;
    }

    /**
     * Reconstruct current state (READ operation)
     * 
     * Returns current_state = initial_state XOR accumulator
     * 
     * @returns {BigInt} Current state
     */
    reconstruct() {
        return this.initialState ^ this.accumulator;
    }

    /**
     * Check if accumulator is zero (STATUS operation)
     * 
     * @returns {boolean} True if accumulator is zero
     */
    isAccumulatorZero() {
        return this.accumulator === 0n;
    }

    /**
     * Rollback the last N delta operations
     * 
     * @param {number} count - Number of deltas to rollback
     * @returns {number} Number of deltas actually rolled back
     */
    rollback(count) {
        const actualCount = Math.min(count, this.history.length);
        for (let i = 0; i < actualCount; i++) {
            const delta = this.history.pop();
            // XOR removes the delta (self-inverse property)
            this.accumulator ^= delta;
        }
        return actualCount;
    }

    /**
     * Get the current accumulator value
     * 
     * @returns {BigInt} Accumulator value
     */
    getAccumulator() {
        return this.accumulator;
    }

    /**
     * Get the initial state
     * 
     * @returns {BigInt} Initial state
     */
    getInitialState() {
        return this.initialState;
    }

    /**
     * Get the number of deltas in history
     * 
     * @returns {number} History size
     */
    historySize() {
        return this.history.length;
    }

}

export default PriceTick;
