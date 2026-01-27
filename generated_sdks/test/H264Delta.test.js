import { H264Delta } from '../src/H264Delta.js';
import assert from 'assert';

console.log('Testing H264Delta...');
console.log('='.repeat(70));

// Test 1: LOAD operation
const manager = new H264Delta();
manager.load(0x1234567890ABCDEFn);
assert.strictEqual(manager.getInitialState(), 0x1234567890ABCDEFn);
assert.strictEqual(manager.getAccumulator(), 0n);
console.log('[PASS] test_load');

// Test 2: ACCUMULATE operation
manager.load(0n);
manager.accumulate(0x1111111111111111n);
assert.strictEqual(manager.getAccumulator(), 0x1111111111111111n);
manager.accumulate(0x2222222222222222n);
assert.strictEqual(manager.getAccumulator(), 0x3333333333333333n);
console.log('[PASS] test_accumulate');

// Test 3: RECONSTRUCT operation
manager.load(0xAAAAAAAAAAAAAAAAn);
manager.accumulate(0x5555555555555555n);
// 0xAAAA XOR 0x5555 = 0xFFFF
assert.strictEqual(manager.reconstruct(), 0xFFFFFFFFFFFFFFFFn);
console.log('[PASS] test_reconstruct');

// Test 4: Self-inverse property
manager.load(0xAAAAAAAAAAAAAAAAn);
const delta = 0x1234567890ABCDEFn;
manager.accumulate(delta);
manager.accumulate(delta);  // Apply same delta twice
// Self-inverse: delta XOR delta = 0
assert.strictEqual(manager.isAccumulatorZero(), true);
assert.strictEqual(manager.reconstruct(), 0xAAAAAAAAAAAAAAAAn);
console.log('[PASS] test_self_inverse');

// Test 5: Rollback operation
manager.load(0n);
manager.accumulate(0x1111111111111111n);
manager.accumulate(0x2222222222222222n);
manager.accumulate(0x4444444444444444n);
assert.strictEqual(manager.getAccumulator(), 0x7777777777777777n);
// Rollback last 2 operations
const count = manager.rollback(2);
assert.strictEqual(count, 2);
assert.strictEqual(manager.getAccumulator(), 0x1111111111111111n);
console.log('[PASS] test_rollback');

console.log('='.repeat(70));
console.log('All tests passed!');
