/* ============================================================================
 * sd_verify - Cortex-A9 baremetal SD-card readback verifier.
 *
 * Reads requested LBAs from the SD card into PS-DDR scratch so the host can
 * compare via JTAG, and can hash a full contiguous BOOT.BIN range directly
 * on the ARM so we do not mistake sampled sectors for a complete write proof.
 * Reuses the proven sdhc.c driver.
 *
 * Scratch:
 *   0x10100000  STATUS (see SDV_*)
 *   0x10100004  current LBA being read
 *   0x10100008  last SDHC error code
 *   0x1010000C  current job index
 *   0x10100018  done magic = 0xC0DEC0DE on success
 *   0x10100020  host-provided job count, or 0 for defaults
 *   0x10100024  hash start LBA, or 0 to skip
 *   0x10100028  hash byte count, or 0 to skip
 *   0x1010002C  FNV-1a hash result
 *   0x10100030  hash bytes completed
 *   0x10100040  job table: {lba, dst_ddr} pairs, 8 bytes each
 * ========================================================================== */

#include <stdint.h>
#include "zynq_regs.h"
#include "sdhc.h"

#define SCRATCH_BASE        0x10100000u
#define S_STATUS            (SCRATCH_BASE + 0x00)
#define S_LBA               (SCRATCH_BASE + 0x04)
#define S_ERR               (SCRATCH_BASE + 0x08)
#define S_JOB_INDEX         (SCRATCH_BASE + 0x0C)
#define S_DONE              (SCRATCH_BASE + 0x18)
#define S_JOB_COUNT         (SCRATCH_BASE + 0x20)
#define S_HASH_LBA          (SCRATCH_BASE + 0x24)
#define S_HASH_BYTES        (SCRATCH_BASE + 0x28)
#define S_HASH_RESULT       (SCRATCH_BASE + 0x2C)
#define S_HASH_DONE_BYTES   (SCRATCH_BASE + 0x30)
#define S_JOB_TABLE         (SCRATCH_BASE + 0x40)

#define DONE_MAGIC          0xC0DEC0DEu
#define MAX_JOBS            16u
#define SECTOR_SIZE         512u
#define FNV1A_OFFSET        2166136261u
#define FNV1A_PRIME         16777619u

#define SDV_BOOT            0xBAD10001u
#define SDV_CTRL_INIT_FAIL  0xBAD10002u
#define SDV_CARD_INIT_FAIL  0xBAD10003u
#define SDV_READ_FAIL       0xBAD10004u
#define SDV_HASH_READ_FAIL  0xBAD10005u

typedef struct {
    uint32_t lba;
    uint32_t dst_ddr;   /* 512-B destination */
} read_job_t;

static const read_job_t jobs[] = {
    { 0,     0x10200000u },  /* MBR */
    { 8192,  0x10201000u },  /* VBR */
    { 8224,  0x10202000u },  /* FAT1[0] */
    { 9248,  0x10203000u },  /* FAT2[0] */
    { 10272, 0x10204000u },  /* Root dir */
    { 10280, 0x10205000u },  /* BOOT.bin first sector */
};
#define N_JOBS ((int)(sizeof(jobs)/sizeof(jobs[0])))

static uint8_t hash_buf[SECTOR_SIZE] __attribute__((aligned(64)));

static inline void put_status(uint32_t v) { *(volatile uint32_t *)S_STATUS = v; }
static inline void put_lba(uint32_t v)    { *(volatile uint32_t *)S_LBA    = v; }
static inline void put_err(uint32_t v)    { *(volatile uint32_t *)S_ERR    = v; }
static inline void put_job_index(uint32_t v) { *(volatile uint32_t *)S_JOB_INDEX = v; }

static read_job_t get_job(uint32_t index)
{
    uint32_t base = S_JOB_TABLE + index * 8u;
    read_job_t job;
    job.lba = *(volatile uint32_t *)(base + 0u);
    job.dst_ddr = *(volatile uint32_t *)(base + 4u);
    return job;
}

static void hash_boot_range(sd_card_t *card)
{
    uint32_t lba = *(volatile uint32_t *)S_HASH_LBA;
    uint32_t remaining = *(volatile uint32_t *)S_HASH_BYTES;
    if (lba == 0u || remaining == 0u)
        return;

    uint32_t h = FNV1A_OFFSET;
    uint32_t done = 0u;
    *(volatile uint32_t *)S_HASH_RESULT = 0u;
    *(volatile uint32_t *)S_HASH_DONE_BYTES = 0u;

    while (remaining) {
        put_lba(lba);
        sdhc_err_t e = sdhc_read_block(card, lba, hash_buf);
        if (e != SDHC_OK) {
            put_err((uint32_t)e); put_status(SDV_HASH_READ_FAIL);
            while (1) {}
        }

        uint32_t n = remaining > SECTOR_SIZE ? SECTOR_SIZE : remaining;
        for (uint32_t i = 0; i < n; i++) {
            h ^= hash_buf[i];
            h *= FNV1A_PRIME;
        }
        remaining -= n;
        done += n;
        lba++;
        *(volatile uint32_t *)S_HASH_RESULT = h;
        *(volatile uint32_t *)S_HASH_DONE_BYTES = done;
    }
}

void main(void)
{
    put_status(SDV_BOOT);
    put_err(0);
    put_job_index(0);

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

    uint32_t host_jobs = *(volatile uint32_t *)S_JOB_COUNT;
    int n_jobs = N_JOBS;
    int use_host_jobs = 0;
    if (host_jobs > 0u && host_jobs <= MAX_JOBS) {
        n_jobs = (int)host_jobs;
        use_host_jobs = 1;
    }

    for (int j = 0; j < n_jobs; j++) {
        read_job_t job = use_host_jobs ? get_job((uint32_t)j) : jobs[j];
        put_job_index((uint32_t)j);
        put_lba(job.lba);
        e = sdhc_read_block(&card, job.lba,
                            (void *)(uintptr_t)job.dst_ddr);
        if (e != SDHC_OK) {
            put_err((uint32_t)e); put_status(SDV_READ_FAIL);
            while (1) {}
        }
    }

    hash_boot_range(&card);

    *(volatile uint32_t *)S_DONE = DONE_MAGIC;
    put_status(DONE_MAGIC);
    while (1) {}
}
