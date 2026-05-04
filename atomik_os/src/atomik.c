/* atomik.c — minimal ATOMiK delta-state adapter access via /dev/mem.
 *
 * The adapter sits at NaxRiscv address 0xF0020000 (matches the LiteX CSR map
 * — see hardware/zynq/litex/build/.../csr.csv `atomik_adapter` region).
 * We expose the raw 32-bit RD readback per slot. This is read-only telemetry
 * for the System Monitor app — no writes, so we never disturb other code
 * that might be using the adapter. */
#include "atomik_os.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

static int           s_mem_fd  = -1;
static volatile uint8_t *s_map = NULL;

int atomik_open(void) {
    if (s_mem_fd >= 0) return 0;
    s_mem_fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (s_mem_fd < 0) { perror("atomik_open /dev/mem"); return -1; }
    /* The adapter window we care about is one 4 KiB page. */
    s_map = mmap(NULL, 4096, PROT_READ, MAP_SHARED, s_mem_fd,
                 ATOMIK_BASE_PS & ~0xFFFUL);
    if (s_map == MAP_FAILED) {
        perror("atomik_open mmap");
        close(s_mem_fd);
        s_mem_fd = -1;
        s_map    = NULL;
        return -1;
    }
    return 0;
}

void atomik_close(void) {
    if (s_map && s_map != MAP_FAILED) munmap((void *)s_map, 4096);
    if (s_mem_fd >= 0) close(s_mem_fd);
    s_map    = NULL;
    s_mem_fd = -1;
}

int atomik_read_slots(uint32_t *out) {
    if (!s_map || s_map == MAP_FAILED) {
        for (int i = 0; i < ATOMIK_N_SLOTS; i++) out[i] = 0xDEADBEEFu;
        return -1;
    }
    /* Each slot's RD register lives at +0x0C within its 4-register block.
     * We assume slot stride = 16 bytes (4 regs × 4 bytes) and 8 slots fit
     * within the 4 KiB page. This matches the wishbone wrapper layout. */
    for (int i = 0; i < ATOMIK_N_SLOTS; i++) {
        unsigned long off = (unsigned long)i * 16 + 0x0C;
        out[i] = *(volatile uint32_t *)(s_map + (off & 0xFFFUL));
    }
    return 0;
}
