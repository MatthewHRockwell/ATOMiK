/* ============================================================================
 * sd_verify — Cortex-A9 baremetal SD-card readback verifier.
 *
 * Reads specific LBAs from the SD card into PS-DDR scratch so the host can
 * compare via JTAG. Reuses the proven sdhc.c driver.
 *
 * Memory map (PS view):
 *   0x10000000  this binary (256 KB)
 *   0x10100000  control/status scratch
 *   0x10200000  LBA 0     (MBR)      — 512 B
 *   0x10201000  LBA 8192  (VBR)      — 512 B
 *   0x10202000  LBA 8224  (FAT1[0])  — 512 B
 *   0x10203000  LBA 8256  (Root dir) — 512 B
 *   0x10204000  LBA 8320  (BOOT.bin start) — 512 B
 *
 * Scratch:
 *   0x10100000  STATUS (see SDV_*)
 *   0x10100004  current LBA being read
 *   0x10100008  last SDHC error code
 *   0x10100018  done magic = 0xC0DEC0DE on success
 * ========================================================================= */

#include <stdint.h>
#include "zynq_regs.h"
#include "sdhc.h"

#define SCRATCH_BASE        0x10100000u
#define S_STATUS            (SCRATCH_BASE + 0x00)
#define S_LBA               (SCRATCH_BASE + 0x04)
#define S_ERR               (SCRATCH_BASE + 0x08)
#define S_DONE              (SCRATCH_BASE + 0x18)

#define DONE_MAGIC          0xC0DEC0DEu

#define SDV_BOOT            0xBAD10001u
#define SDV_CTRL_INIT_FAIL  0xBAD10002u
#define SDV_CARD_INIT_FAIL  0xBAD10003u
#define SDV_READ_FAIL       0xBAD10004u

typedef struct {
    uint32_t lba;
    uint32_t dst_ddr;   /* 512-B destination */
} read_job_t;

static const read_job_t jobs[] = {
    { 0,    0x10200000u },   /* MBR */
    { 8192, 0x10201000u },   /* VBR */
    { 8224, 0x10202000u },   /* FAT1[0] */
    { 8256, 0x10203000u },   /* Root dir */
    { 8320, 0x10204000u },   /* BOOT.bin first sector */
};
#define N_JOBS ((int)(sizeof(jobs)/sizeof(jobs[0])))

static inline void put_status(uint32_t v) { *(volatile uint32_t *)S_STATUS = v; }
static inline void put_lba(uint32_t v)    { *(volatile uint32_t *)S_LBA    = v; }
static inline void put_err(uint32_t v)    { *(volatile uint32_t *)S_ERR    = v; }

void main(void)
{
    put_status(SDV_BOOT);
    put_err(0);

    sd_card_t card;
    sdhc_err_t e;

    e = sdhc_controller_init(&card, SD0_BASE);
    if (e != SDHC_OK) {
        put_err((uint32_t)e); put_status(SDV_CTRL_INIT_FAIL);
        while (1) {}
    }

    e = sdhc_card_init(&card);
    if (e != SDHC_OK) {
        put_err((uint32_t)e); put_status(SDV_CARD_INIT_FAIL);
        while (1) {}
    }

    for (int j = 0; j < N_JOBS; j++) {
        put_lba(jobs[j].lba);
        e = sdhc_read_block(&card, jobs[j].lba,
                            (void *)(uintptr_t)jobs[j].dst_ddr);
        if (e != SDHC_OK) {
            put_err((uint32_t)e); put_status(SDV_READ_FAIL);
            while (1) {}
        }
    }

    *(volatile uint32_t *)S_DONE = DONE_MAGIC;
    put_status(DONE_MAGIC);
    while (1) {}
}
