/*
 * example_atomik.c — Minimal libatomik usage example
 *
 * Demonstrates the four ATOMiK operations on hardware:
 *   LOAD  — set initial state
 *   ACCUM — apply XOR delta
 *   READ  — reconstruct current state
 *   SWAP  — update reference, clear accumulator
 *
 * Build (Zynq rv32ima):
 *   make zynq CROSS=riscv32-buildroot-linux-gnu-
 *
 * Run:
 *   mknod /dev/mem c 1 1 2>/dev/null
 *   ./example_atomik
 */

#include <stdio.h>
#include <stdint.h>
#include "libatomik.h"

int main(void)
{
    /* Open ATOMiK via /dev/mem at LiteX CSR base address */
    atomik_t *a = atomik_open_devmem(0xF0000000, ATOMIK_LAYOUT_CSR);
    if (!a) {
        printf("Cannot open ATOMiK at 0xF0000000\n");
        printf("Ensure: mknod /dev/mem c 1 1; chmod 666 /dev/mem\n");
        return 1;
    }

    printf("ATOMiK v%u, %u bank(s)\n\n", atomik_version(a), atomik_bank_count(a));

    /* LOAD: initialize context 0 with a known state */
    uint64_t initial = 0xCAFEBABEDEADBEEFULL;
    atomik_load(a, 0, initial);
    printf("LOAD(0, 0x%016llx)\n", (unsigned long long)initial);

    /* READ: should return the initial state (accumulator is zero) */
    uint64_t state = atomik_read(a);
    printf("READ() = 0x%016llx  %s\n", (unsigned long long)state,
           state == initial ? "[OK]" : "[MISMATCH]");

    /* ACCUM: XOR a delta */
    uint64_t delta = 0x00000000000000FFULL;
    atomik_accum(a, delta);
    printf("ACCUM(0x%016llx)\n", (unsigned long long)delta);

    /* READ: state = initial XOR delta */
    state = atomik_read(a);
    uint64_t expected = initial ^ delta;
    printf("READ() = 0x%016llx  %s (expected 0x%016llx)\n",
           (unsigned long long)state,
           state == expected ? "[OK]" : "[MISMATCH]",
           (unsigned long long)expected);

    /* Self-inverse: apply same delta again to undo */
    atomik_accum(a, delta);
    state = atomik_read(a);
    printf("ACCUM(same delta) → READ() = 0x%016llx  %s (undo)\n",
           (unsigned long long)state,
           state == initial ? "[OK]" : "[MISMATCH]");

    /* Change detection: is accumulator zero? */
    printf("acc_zero = %d  %s\n", atomik_acc_zero(a),
           atomik_acc_zero(a) ? "[no deltas pending]" : "[deltas pending]");

    /* SWAP: snapshot current state as new reference */
    atomik_accum(a, 0x1111111111111111ULL);
    atomik_swap(a, 0);
    state = atomik_read(a);
    printf("SWAP(0) → READ() = 0x%016llx  (new reference)\n",
           (unsigned long long)state);
    printf("acc_zero = %d  [accumulator cleared by SWAP]\n", atomik_acc_zero(a));

    atomik_close(a);
    printf("\nDone.\n");
    return 0;
}
