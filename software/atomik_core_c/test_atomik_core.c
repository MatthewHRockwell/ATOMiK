/*
 * test_atomik_core.c — Tests for the atomik_core.h single-header library.
 *
 * Build: gcc -o test_atomik_core test_atomik_core.c -Wall -Wextra
 * Run:   ./test_atomik_core
 */

#define ATOMIK_IMPLEMENTATION
#include "atomik_core.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    tests_run++; \
    printf("  T%d: %-40s ", tests_run, name); \
    fflush(stdout); \
} while(0)

#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("FAIL (0x%llx != 0x%llx)\n", \
               (unsigned long long)(a), (unsigned long long)(b)); \
        return; \
    } \
} while(0)

/* === Context Tests === */

static void test_load_read(void) {
    TEST("load + read");
    atomik_ctx_t ctx;
    atomik_init(&ctx);
    atomik_load(&ctx, 0xDEADBEEFCAFEBABEULL);
    ASSERT_EQ(atomik_read(&ctx), 0xDEADBEEFCAFEBABEULL);
    PASS();
}

static void test_accum(void) {
    TEST("accum XOR");
    atomik_ctx_t ctx;
    atomik_init(&ctx);
    atomik_load(&ctx, 0xDEADBEEF00000000ULL);
    atomik_accum(&ctx, 0x00000000000000FFULL);
    ASSERT_EQ(atomik_read(&ctx), 0xDEADBEEF000000FFULL);
    PASS();
}

static void test_identity(void) {
    TEST("identity element (accum 0)");
    atomik_ctx_t ctx;
    atomik_init(&ctx);
    atomik_load(&ctx, 0xDEADBEEFULL);
    atomik_accum(&ctx, 0);
    ASSERT_EQ(atomik_read(&ctx), 0xDEADBEEFULL);
    PASS();
}

static void test_self_inverse(void) {
    TEST("self-inverse (accum twice)");
    atomik_ctx_t ctx;
    atomik_init(&ctx);
    atomik_load(&ctx, 0xDEADBEEFULL);
    atomik_accum(&ctx, 0x12345678ULL);
    atomik_accum(&ctx, 0x12345678ULL);
    ASSERT_EQ(atomik_read(&ctx), 0xDEADBEEFULL);
    PASS();
}

static void test_commutativity(void) {
    TEST("commutativity");
    atomik_ctx_t a, b;
    atomik_init(&a); atomik_init(&b);
    atomik_load(&a, 0xCAFEBABEULL);
    atomik_load(&b, 0xCAFEBABEULL);
    atomik_accum(&a, 0x11111111ULL);
    atomik_accum(&a, 0x22222222ULL);
    atomik_accum(&b, 0x22222222ULL);
    atomik_accum(&b, 0x11111111ULL);
    ASSERT_EQ(atomik_read(&a), atomik_read(&b));
    PASS();
}

static void test_swap(void) {
    TEST("swap returns state");
    atomik_ctx_t ctx;
    atomik_init(&ctx);
    atomik_load(&ctx, 0xAAAAULL);
    atomik_accum(&ctx, 0x00FFULL);
    uint64_t old = atomik_swap(&ctx);
    ASSERT_EQ(old, 0xAAAAULL ^ 0x00FFULL);
    ASSERT_EQ(atomik_is_clean(&ctx), 1);
    ASSERT_EQ(atomik_read(&ctx), old);
    PASS();
}

static void test_merge(void) {
    TEST("merge accumulators");
    atomik_ctx_t a, b;
    atomik_init(&a); atomik_init(&b);
    atomik_load(&a, 0xAAAAULL);
    atomik_load(&b, 0xAAAAULL);
    atomik_accum(&a, 0x0011ULL);
    atomik_accum(&b, 0x1100ULL);
    atomik_merge(&a, &b);
    ASSERT_EQ(atomik_read(&a), 0xAAAAULL ^ 0x0011ULL ^ 0x1100ULL);
    PASS();
}

/* === Table Tests === */

static void test_table_basic(void) {
    TEST("table load/read");
    atomik_table_t t;
    atomik_table_init(&t, 4);
    atomik_table_load(&t, 0, 0x1111ULL);
    atomik_table_load(&t, 1, 0x2222ULL);
    ASSERT_EQ(atomik_table_read(&t, 0), 0x1111ULL);
    ASSERT_EQ(atomik_table_read(&t, 1), 0x2222ULL);
    atomik_table_free(&t);
    PASS();
}

static void test_table_isolation(void) {
    TEST("table address isolation");
    atomik_table_t t;
    atomik_table_init(&t, 4);
    atomik_table_load(&t, 0, 0x1111ULL);
    atomik_table_load(&t, 1, 0x2222ULL);
    atomik_table_accum(&t, 0, 0x00FFULL);
    ASSERT_EQ(atomik_table_read(&t, 0), 0x1111ULL ^ 0x00FFULL);
    ASSERT_EQ(atomik_table_read(&t, 1), 0x2222ULL);
    atomik_table_free(&t);
    PASS();
}

static void test_table_256(void) {
    TEST("table 256 contexts");
    atomik_table_t t;
    atomik_table_init(&t, 256);
    for (uint32_t i = 0; i < 256; i++) {
        atomik_table_load(&t, i, (uint64_t)i * 0x0101010101010101ULL);
    }
    for (uint32_t i = 0; i < 256; i++) {
        ASSERT_EQ(atomik_table_read(&t, i), (uint64_t)i * 0x0101010101010101ULL);
    }
    atomik_table_free(&t);
    PASS();
}

/* === Fingerprint Tests === */

static void test_fp_no_change(void) {
    TEST("fingerprint no change");
    atomik_fingerprint_t fp;
    atomik_fp_init(&fp);
    const char *data = "Hello, ATOMiK!";
    atomik_fp_load(&fp, data, strlen(data));
    atomik_fp_update(&fp, data, strlen(data));
    ASSERT_EQ(atomik_fp_changed(&fp), 0);
    PASS();
}

static void test_fp_change(void) {
    TEST("fingerprint detects change");
    atomik_fingerprint_t fp;
    atomik_fp_init(&fp);
    char data1[] = "Hello, ATOMiK!";
    char data2[] = "Hello, ATOMIK!";
    atomik_fp_load(&fp, data1, strlen(data1));
    atomik_fp_update(&fp, data2, strlen(data2));
    ASSERT_EQ(atomik_fp_changed(&fp), 1);
    PASS();
}

static void test_fp_incremental(void) {
    TEST("fingerprint incremental delta");
    atomik_fingerprint_t fp;
    atomik_fp_init(&fp);
    uint64_t data[8] = {0};
    atomik_fp_load(&fp, data, sizeof(data));
    atomik_fp_accum_delta(&fp, 0x0000000000000000ULL, 0x00000000000000FFULL);
    ASSERT_EQ(atomik_fp_changed(&fp), 1);
    atomik_fp_accum_delta(&fp, 0x00000000000000FFULL, 0x0000000000000000ULL);
    ASSERT_EQ(atomik_fp_changed(&fp), 0);
    PASS();
}

/* === Stress === */

static void test_stress_1m(void) {
    TEST("1M deltas + undo");
    atomik_ctx_t ctx;
    atomik_init(&ctx);
    atomik_load(&ctx, 0xDEADBEEFCAFEBABEULL);
    uint64_t deltas[1000];
    for (int i = 0; i < 1000; i++) {
        deltas[i] = (uint64_t)i * 0x1234567890ABCDEFULL + 0x1111ULL;
        atomik_accum(&ctx, deltas[i]);
    }
    for (int i = 0; i < 1000; i++) {
        atomik_accum(&ctx, deltas[i]);  /* self-inverse */
    }
    ASSERT_EQ(atomik_read(&ctx), 0xDEADBEEFCAFEBABEULL);
    PASS();
}

int main(void) {
    printf("=== ATOMiK Core C Library Tests ===\n\n");

    /* Context */
    test_load_read();
    test_accum();
    test_identity();
    test_self_inverse();
    test_commutativity();
    test_swap();
    test_merge();

    /* Table */
    test_table_basic();
    test_table_isolation();
    test_table_256();

    /* Fingerprint */
    test_fp_no_change();
    test_fp_change();
    test_fp_incremental();

    /* Stress */
    test_stress_1m();

    printf("\n=== %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
