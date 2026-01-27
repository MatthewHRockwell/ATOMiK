#include <stdio.h>
#include <assert.h>
#include "../atomik/video/streaming/h264_delta.h"

void test_load() {
    atomik_h264_delta_t manager;
    atomik_h264_delta_init(&manager);
    atomik_h264_delta_load(&manager, 0x1234567890ABCDEFULL);
    
    assert(atomik_h264_delta_get_initial_state(&manager) == 0x1234567890ABCDEFULL);
    assert(atomik_h264_delta_get_accumulator(&manager) == 0);
    printf("[PASS] test_load\n");
}

void test_accumulate() {
    atomik_h264_delta_t manager;
    atomik_h264_delta_init(&manager);
    atomik_h264_delta_load(&manager, 0);
    
    atomik_h264_delta_accumulate(&manager, 0x1111111111111111ULL);
    assert(atomik_h264_delta_get_accumulator(&manager) == 0x1111111111111111ULL);
    
    atomik_h264_delta_accumulate(&manager, 0x2222222222222222ULL);
    assert(atomik_h264_delta_get_accumulator(&manager) == 0x3333333333333333ULL);
    printf("[PASS] test_accumulate\n");
}

void test_reconstruct() {
    atomik_h264_delta_t manager;
    atomik_h264_delta_init(&manager);
    atomik_h264_delta_load(&manager, 0xAAAAAAAAAAAAAAAAULL);
    atomik_h264_delta_accumulate(&manager, 0x5555555555555555ULL);
    
    /* 0xAAAA XOR 0x5555 = 0xFFFF */
    assert(atomik_h264_delta_reconstruct(&manager) == 0xFFFFFFFFFFFFFFFFULL);
    printf("[PASS] test_reconstruct\n");
}

void test_self_inverse() {
    atomik_h264_delta_t manager;
    atomik_h264_delta_init(&manager);
    atomik_h264_delta_load(&manager, 0xAAAAAAAAAAAAAAAAULL);
    
    uint64_t delta = 0x1234567890ABCDEFULL;
    atomik_h264_delta_accumulate(&manager, delta);
    atomik_h264_delta_accumulate(&manager, delta);  /* Apply same delta twice */
    
    /* Self-inverse: delta XOR delta = 0 */
    assert(atomik_h264_delta_is_accumulator_zero(&manager));
    assert(atomik_h264_delta_reconstruct(&manager) == 0xAAAAAAAAAAAAAAAAULL);
    printf("[PASS] test_self_inverse\n");
}

void test_rollback() {
    atomik_h264_delta_t manager;
    atomik_h264_delta_init(&manager);
    atomik_h264_delta_load(&manager, 0);
    
    atomik_h264_delta_accumulate(&manager, 0x1111111111111111ULL);
    atomik_h264_delta_accumulate(&manager, 0x2222222222222222ULL);
    atomik_h264_delta_accumulate(&manager, 0x4444444444444444ULL);
    assert(atomik_h264_delta_get_accumulator(&manager) == 0x7777777777777777ULL);
    
    /* Rollback last 2 operations */
    size_t count = atomik_h264_delta_rollback(&manager, 2);
    assert(count == 2);
    assert(atomik_h264_delta_get_accumulator(&manager) == 0x1111111111111111ULL);
    printf("[PASS] test_rollback\n");
}

int main() {
    printf("Testing H264Delta...\n");

    test_load();
    test_accumulate();
    test_reconstruct();
    test_self_inverse();
    test_rollback();

    printf("\nAll tests passed!\n");
    return 0;
}
