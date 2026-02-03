#include <stdio.h>
#include <assert.h>
#include "../atomik/finance/trading/price_tick.h"

void test_load() {
    atomik_price_tick_t manager;
    atomik_price_tick_init(&manager);
    atomik_price_tick_load(&manager, 0x1234567890ABCDEFULL);
    
    assert(atomik_price_tick_get_initial_state(&manager) == 0x1234567890ABCDEFULL);
    assert(atomik_price_tick_get_accumulator(&manager) == 0);
    printf("[PASS] test_load\n");
}

void test_accumulate() {
    atomik_price_tick_t manager;
    atomik_price_tick_init(&manager);
    atomik_price_tick_load(&manager, 0);
    
    atomik_price_tick_accumulate(&manager, 0x1111111111111111ULL);
    assert(atomik_price_tick_get_accumulator(&manager) == 0x1111111111111111ULL);
    
    atomik_price_tick_accumulate(&manager, 0x2222222222222222ULL);
    assert(atomik_price_tick_get_accumulator(&manager) == 0x3333333333333333ULL);
    printf("[PASS] test_accumulate\n");
}

void test_reconstruct() {
    atomik_price_tick_t manager;
    atomik_price_tick_init(&manager);
    atomik_price_tick_load(&manager, 0xAAAAAAAAAAAAAAAAULL);
    atomik_price_tick_accumulate(&manager, 0x5555555555555555ULL);
    
    /* 0xAAAA XOR 0x5555 = 0xFFFF */
    assert(atomik_price_tick_reconstruct(&manager) == 0xFFFFFFFFFFFFFFFFULL);
    printf("[PASS] test_reconstruct\n");
}

void test_self_inverse() {
    atomik_price_tick_t manager;
    atomik_price_tick_init(&manager);
    atomik_price_tick_load(&manager, 0xAAAAAAAAAAAAAAAAULL);
    
    uint64_t delta = 0x1234567890ABCDEFULL;
    atomik_price_tick_accumulate(&manager, delta);
    atomik_price_tick_accumulate(&manager, delta);  /* Apply same delta twice */
    
    /* Self-inverse: delta XOR delta = 0 */
    assert(atomik_price_tick_is_accumulator_zero(&manager));
    assert(atomik_price_tick_reconstruct(&manager) == 0xAAAAAAAAAAAAAAAAULL);
    printf("[PASS] test_self_inverse\n");
}

void test_rollback() {
    atomik_price_tick_t manager;
    atomik_price_tick_init(&manager);
    atomik_price_tick_load(&manager, 0);
    
    atomik_price_tick_accumulate(&manager, 0x1111111111111111ULL);
    atomik_price_tick_accumulate(&manager, 0x2222222222222222ULL);
    atomik_price_tick_accumulate(&manager, 0x4444444444444444ULL);
    assert(atomik_price_tick_get_accumulator(&manager) == 0x7777777777777777ULL);
    
    /* Rollback last 2 operations */
    size_t count = atomik_price_tick_rollback(&manager, 2);
    assert(count == 2);
    assert(atomik_price_tick_get_accumulator(&manager) == 0x1111111111111111ULL);
    printf("[PASS] test_rollback\n");
}

int main() {
    printf("Testing PriceTick...\n");

    test_load();
    test_accumulate();
    test_reconstruct();
    test_self_inverse();
    test_rollback();

    printf("\nAll tests passed!\n");
    return 0;
}
