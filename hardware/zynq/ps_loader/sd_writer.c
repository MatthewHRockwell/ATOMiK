/* ============================================================================
 * ATOMiK SD Writer — Cortex-A9 baremetal SD-card raw-sector writer.
 *
 * Run via JTAG xsdb after the host has pre-loaded source files into PS DDR.
 * Reuses the proven sdhc.c driver. Writes BOOT.bin and nax64.bit.bin to their
 * target LBAs on the FAT32-formatted SD card.
 *
 * Memory map (PS view):
 *   0x10000000  this loader (256 KB)
 *   0x10100000  scratch (host watches via xsdb mrd)
 *   0x18100000  BOOT.bin source (3.9 MB, loaded by JTAG before run)
 *   0x1C100000  nax64.bit.bin source (3.9 MB, loaded by JTAG before run)
 *
 * Scratch layout (host watches these):
 *   0x10100000  STATUS code (see SDW_* below)
 *   0x10100004  current LBA being written
 *   0x10100008  bytes written so far
 *   0x1010000C  last SDHC error code
 *   0x10100010  card OCR
 *   0x10100014  card RCA (16-bit, shifted)
 *   0x10100018  done magic
 *
 * Final status:
 *   STATUS == 0xC0DEC0DE  → success
 *   STATUS == 0xBADXXXXX  → failure (XXXXX = phase)
 * ========================================================================= */

#include <stdint.h>
#include "zynq_regs.h"
#include "sdhc.h"

#define SCRATCH_BASE        0x10100000u
#define S_STATUS            (SCRATCH_BASE + 0x00)
#define S_LBA               (SCRATCH_BASE + 0x04)
#define S_BYTES             (SCRATCH_BASE + 0x08)
#define S_ERR               (SCRATCH_BASE + 0x0C)
#define S_OCR               (SCRATCH_BASE + 0x10)
#define S_RCA               (SCRATCH_BASE + 0x14)
#define S_DONE              (SCRATCH_BASE + 0x18)
#define S_PHASE             (SCRATCH_BASE + 0x20)
#define S_TEST_READBACK     (SCRATCH_BASE + 0x40)  /* 512 B */

#define DONE_MAGIC          0xC0DEC0DEu

#define SDW_BOOT            0xBAD00001u
#define SDW_CTRL_INIT_FAIL  0xBAD00002u
#define SDW_CARD_INIT_FAIL  0xBAD00003u
#define SDW_TEST_WRITE_FAIL 0xBAD00004u
#define SDW_TEST_READ_FAIL  0xBAD00005u
#define SDW_TEST_MISMATCH   0xBAD00006u
#define SDW_BOOTBIN_FAIL    0xBAD00007u
#define SDW_BITSTR_FAIL     0xBAD00008u
#define SDW_MBR_FAIL        0xBAD00009u
#define SDW_PARTHDR_FAIL    0xBAD0000Au

/* File-write jobs */
typedef struct {
    uint32_t src_ddr;   /* PS DDR source address (4-byte aligned) */
    uint32_t dst_lba;   /* SD card start LBA */
    uint32_t n_bytes;   /* total bytes (will be rounded up to sector) */
    uint32_t fail_code;
} write_job_t;

static const write_job_t jobs[] = {
    /* MBR sector — partition table pointing FAT32 at LBA 8192 */
    { 0x14000000u, 0u,        512u, SDW_MBR_FAIL },
    /* FAT32 partition header: VBR + FAT1 + FAT2 + Root-dir (128 sectors) */
    { 0x14001000u, 8192u,   65536u, SDW_PARTHDR_FAIL },
    /* BOOT.bin — FSBL + ATOMiK fpga bitstream + ps_loader */
    { 0x18100000u, 8320u,  4073556u, SDW_BOOTBIN_FAIL },
    /* nax64.bit.bin — autoboot BIOS bitstream */
    { 0x1C100000u, 16320u, 4045568u, SDW_BITSTR_FAIL },
};
#define N_JOBS ((int)(sizeof(jobs)/sizeof(jobs[0])))

/* Tiny mmio helpers (zynq_regs.h provides w32/r32 already) */
static inline void put_status(uint32_t v)  { *(volatile uint32_t *)S_STATUS = v; }
static inline void put_phase(uint32_t v)   { *(volatile uint32_t *)S_PHASE  = v; }
static inline void put_err(uint32_t v)     { *(volatile uint32_t *)S_ERR    = v; }
static inline void put_lba(uint32_t v)     { *(volatile uint32_t *)S_LBA    = v; }
static inline void put_bytes(uint32_t v)   { *(volatile uint32_t *)S_BYTES  = v; }

static uint8_t test_pattern[512] __attribute__((aligned(8)));
static uint8_t test_readback[512] __attribute__((aligned(8)));

void main(void)
{
    put_status(SDW_BOOT);
    put_phase(0x01);
    put_err(0);
    put_bytes(0);

    sd_card_t card;
    sdhc_err_t e;

    /* --- Controller init (clocks, voltage, INT_CLK, SDCLK) --- */
    put_phase(0x10);
    e = sdhc_controller_init(&card, SD0_BASE);
    if (e != SDHC_OK) {
        put_err((uint32_t)e);
        put_status(SDW_CTRL_INIT_FAIL);
        while (1) {}
    }

    /* --- Card init (CMD0/8/55/41/2/3/7) --- */
    put_phase(0x20);
    e = sdhc_card_init(&card);
    if (e != SDHC_OK) {
        put_err((uint32_t)e);
        put_status(SDW_CARD_INIT_FAIL);
        while (1) {}
    }
    *(volatile uint32_t *)S_OCR = card.ocr;
    *(volatile uint32_t *)S_RCA = card.rca;

    /* --- Self-test: write+read+compare on a high "safe" LBA --- */
    /* Use LBA 0x00F00000 (~7.86M ≈ 4 GB in). On a 16 GB+ card this is
     * past most FAT data; if the user's card is smaller, fall back to LBA 1
     * (clobbers VBR) — we'll accept that since we may be re-writing the
     * partition anyway. */
    const uint32_t TEST_LBA = 0x00F00000u;
    for (int i = 0; i < 512; i++)
        test_pattern[i] = (uint8_t)(0xA5 ^ i);
    /* Recognizable header */
    ((uint32_t *)test_pattern)[0] = 0xDEADBEEFu;
    ((uint32_t *)test_pattern)[1] = 0xCAFEBABEu;
    ((uint32_t *)test_pattern)[2] = TEST_LBA;

    put_phase(0x30);
    put_lba(TEST_LBA);
    e = sdhc_write_block(&card, TEST_LBA, test_pattern);
    if (e != SDHC_OK) {
        put_err((uint32_t)e);
        put_status(SDW_TEST_WRITE_FAIL);
        while (1) {}
    }

    put_phase(0x31);
    for (int i = 0; i < 512; i++) test_readback[i] = 0;
    e = sdhc_read_block(&card, TEST_LBA, test_readback);
    if (e != SDHC_OK) {
        put_err((uint32_t)e);
        put_status(SDW_TEST_READ_FAIL);
        while (1) {}
    }

    /* Copy readback to scratch for host inspection */
    {
        volatile uint32_t *dst = (volatile uint32_t *)S_TEST_READBACK;
        const uint32_t *src = (const uint32_t *)test_readback;
        for (int i = 0; i < 128; i++) dst[i] = src[i];
    }
    put_phase(0x32);
    {
        int ok = 1;
        for (int i = 0; i < 512; i++) {
            if (test_readback[i] != test_pattern[i]) { ok = 0; break; }
        }
        if (!ok) {
            put_err(0);
            put_status(SDW_TEST_MISMATCH);
            while (1) {}
        }
    }

    /* --- Bulk write the real files --- */
    for (int j = 0; j < N_JOBS; j++) {
        put_phase(0x40 + (uint32_t)j);
        const write_job_t *jb = &jobs[j];
        uint32_t n_sectors = (jb->n_bytes + 511u) / 512u;
        const uint8_t *src = (const uint8_t *)(uintptr_t)jb->src_ddr;
        for (uint32_t i = 0; i < n_sectors; i++) {
            put_lba(jb->dst_lba + i);
            /* For the last sector if not full 512, pad the tail with 0s in a
             * temp buffer. Otherwise pass the DDR pointer directly. */
            uint32_t off = i * 512u;
            uint32_t remaining = jb->n_bytes - off;
            const void *blk;
            if (remaining >= 512u) {
                blk = src + off;
            } else {
                static uint8_t tail[512] __attribute__((aligned(8)));
                for (uint32_t k = 0; k < remaining; k++) tail[k] = src[off + k];
                for (uint32_t k = remaining; k < 512u; k++) tail[k] = 0;
                blk = tail;
            }
            e = sdhc_write_block(&card, jb->dst_lba + i, blk);
            if (e != SDHC_OK) {
                put_err((uint32_t)e);
                put_status(jb->fail_code);
                while (1) {}
            }
            put_bytes(off + (remaining >= 512u ? 512u : remaining));
        }
    }

    /* --- All done --- */
    put_phase(0xFF);
    *(volatile uint32_t *)S_DONE = DONE_MAGIC;
    put_status(DONE_MAGIC);
    while (1) {}
}
