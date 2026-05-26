#include <stdint.h>
#include <stddef.h>

#include "zynq_regs.h"
#include "sdhc.h"

/* The existing sdhc API is card-based. Wrap it with a single static card on
 * SD0_BASE so this file can stay focused on the boot-image swap test. */
static sd_card_t g_card;

static int sdhc_controller_init_default(void) {
    return (int)sdhc_controller_init(&g_card, SD0_BASE);
}
static int sdhc_card_init_default(void) {
    return (int)sdhc_card_init(&g_card);
}
static int sdhc_write_sectors(uint32_t lba, const void *src, uint32_t n) {
    const uint8_t *p = (const uint8_t *)src;
    for (uint32_t i = 0; i < n; i++) {
        sdhc_err_t e = sdhc_write_block(&g_card, lba + i, p + (i * 512u));
        if (e != SDHC_OK) return (int)e;
    }
    return 0;
}
static int sdhc_read_sectors(uint32_t lba, void *dst, uint32_t n) {
    uint8_t *p = (uint8_t *)dst;
    for (uint32_t i = 0; i < n; i++) {
        sdhc_err_t e = sdhc_read_block(&g_card, lba + i, p + (i * 512u));
        if (e != SDHC_OK) return (int)e;
    }
    return 0;
}

/*
 * One-purpose SD boot-image swap writer.
 *
 * The Python runner preloads three buffers into DDR:
 *   0x14000000: MBR, 512 bytes
 *   0x14001000: FAT32 VBR/FAT/root metadata, 64 KiB
 *   0x18100000: BOOT.BIN payload, byte count in SCRATCH_BOOT_BYTES
 *
 * This keeps the ALINX reference BOOT.BIN A/B test separate from sd_writer.c,
 * whose BOOT.BIN length is intentionally hard-coded to the ATOMiK image.
 */

#define SCRATCH_BASE        0x10100000u
#define SCRATCH_STATUS      (*(volatile uint32_t *)(SCRATCH_BASE + 0x00u))
#define SCRATCH_LBA         (*(volatile uint32_t *)(SCRATCH_BASE + 0x04u))
#define SCRATCH_BYTES       (*(volatile uint32_t *)(SCRATCH_BASE + 0x08u))
#define SCRATCH_ERR         (*(volatile uint32_t *)(SCRATCH_BASE + 0x0Cu))
#define SCRATCH_PHASE       (*(volatile uint32_t *)(SCRATCH_BASE + 0x20u))
#define SCRATCH_BOOT_BYTES  (*(volatile uint32_t *)(SCRATCH_BASE + 0x24u))
/* Host-controllable layout — defaults applied below if scratch holds 0 */
#define SCRATCH_PART_LBA    (*(volatile uint32_t *)(SCRATCH_BASE + 0x28u))
#define SCRATCH_PART_BYTES  (*(volatile uint32_t *)(SCRATCH_BASE + 0x2Cu))
#define SCRATCH_BOOT_LBA    (*(volatile uint32_t *)(SCRATCH_BASE + 0x30u))

#define DONE_MAGIC          0xC0DEC0DEu

#define SRC_MBR             ((const uint8_t *)0x14000000u)
#define SRC_PARTITION       ((const uint8_t *)0x14001000u)
#define SRC_BOOT            ((const uint8_t *)0x18100000u)

#define MBR_LBA             0u
#define DEFAULT_PARTITION_LBA       8192u
#define DEFAULT_BOOT_LBA            8320u
#define DEFAULT_PARTITION_BYTES     (64u * 1024u)
#define SECTOR_SIZE         512u

#define PHASE_CONTROLLER    0x10u
#define PHASE_CARD          0x20u
#define PHASE_TEST_WRITE    0x30u
#define PHASE_TEST_READ     0x31u
#define PHASE_TEST_COMPARE  0x32u
#define PHASE_WRITE_MBR     0x40u
#define PHASE_WRITE_META    0x41u
#define PHASE_WRITE_BOOT    0x42u
#define PHASE_DONE          0xffu

#define ERR_BOOT_SIZE       0xBAD20001u
#define ERR_CONTROLLER      0xBAD20002u
#define ERR_CARD            0xBAD20003u
#define ERR_TEST_WRITE      0xBAD20004u
#define ERR_TEST_READ       0xBAD20005u
#define ERR_TEST_MISMATCH   0xBAD20006u
#define ERR_WRITE_MBR       0xBAD20007u
#define ERR_WRITE_META      0xBAD20008u
#define ERR_WRITE_BOOT      0xBAD20009u

static uint8_t test_wr[SECTOR_SIZE] __attribute__((aligned(64)));
static uint8_t test_rd[SECTOR_SIZE] __attribute__((aligned(64)));

static void fail(uint32_t code, int err)
{
    SCRATCH_ERR = (uint32_t)err;
    SCRATCH_STATUS = code;
    for (;;)
        ;
}

static void fill_test_pattern(void)
{
    for (uint32_t i = 0; i < SECTOR_SIZE; ++i) {
        test_wr[i] = (uint8_t)(0xa5u ^ i ^ (i >> 3));
        test_rd[i] = 0u;
    }
}

static void write_job(const uint8_t *src, uint32_t lba, uint32_t nbytes,
                      uint32_t phase, uint32_t err_code)
{
    uint32_t remaining = nbytes;
    uint32_t cur_lba = lba;
    const uint8_t *p = src;

    SCRATCH_PHASE = phase;
    SCRATCH_LBA = cur_lba;
    SCRATCH_BYTES = 0u;

    while (remaining) {
        uint32_t chunk = remaining;
        if (chunk > 128u * SECTOR_SIZE)
            chunk = 128u * SECTOR_SIZE;
        if (chunk & (SECTOR_SIZE - 1u))
            chunk = (chunk + SECTOR_SIZE - 1u) & ~(SECTOR_SIZE - 1u);

        int err = sdhc_write_sectors(cur_lba, p, chunk / SECTOR_SIZE);
        if (err)
            fail(err_code, err);

        p += chunk;
        cur_lba += chunk / SECTOR_SIZE;
        if (remaining > chunk)
            remaining -= chunk;
        else
            remaining = 0u;

        SCRATCH_LBA = cur_lba;
        SCRATCH_BYTES += chunk;
    }
}

int main(void)
{
    uint32_t boot_bytes = SCRATCH_BOOT_BYTES;
    /* boot_bytes == 0 means "skip BOOT.bin job" — used when the BOOT.bin
     * is already embedded inside the partition image at the right offset. */
    if (boot_bytes > 64u * 1024u * 1024u)
        fail(ERR_BOOT_SIZE, 0);
    if (boot_bytes)
        boot_bytes = (boot_bytes + SECTOR_SIZE - 1u) & ~(SECTOR_SIZE - 1u);

    SCRATCH_STATUS = 0xBAD20000u;
    SCRATCH_LBA = 0u;
    SCRATCH_BYTES = 0u;
    SCRATCH_ERR = 0u;

    SCRATCH_PHASE = PHASE_CONTROLLER;
    int err = sdhc_controller_init_default();
    if (err) fail(ERR_CONTROLLER, err);

    SCRATCH_PHASE = PHASE_CARD;
    err = sdhc_card_init_default();
    if (err)
        fail(ERR_CARD, err);

    fill_test_pattern();

    SCRATCH_PHASE = PHASE_TEST_WRITE;
    err = sdhc_write_sectors(0x1f000u, test_wr, 1u);
    if (err)
        fail(ERR_TEST_WRITE, err);

    SCRATCH_PHASE = PHASE_TEST_READ;
    err = sdhc_read_sectors(0x1f000u, test_rd, 1u);
    if (err)
        fail(ERR_TEST_READ, err);

    SCRATCH_PHASE = PHASE_TEST_COMPARE;
    for (uint32_t i = 0; i < SECTOR_SIZE; ++i) {
        if (test_rd[i] != test_wr[i])
            fail(ERR_TEST_MISMATCH, (int)i);
    }

    uint32_t part_lba   = SCRATCH_PART_LBA   ? SCRATCH_PART_LBA   : DEFAULT_PARTITION_LBA;
    uint32_t part_bytes = SCRATCH_PART_BYTES ? SCRATCH_PART_BYTES : DEFAULT_PARTITION_BYTES;
    uint32_t boot_lba   = SCRATCH_BOOT_LBA   ? SCRATCH_BOOT_LBA   : DEFAULT_BOOT_LBA;

    write_job(SRC_MBR, MBR_LBA, SECTOR_SIZE, PHASE_WRITE_MBR, ERR_WRITE_MBR);
    write_job(SRC_PARTITION, part_lba, part_bytes, PHASE_WRITE_META,
              ERR_WRITE_META);
    if (boot_bytes)
        write_job(SRC_BOOT, boot_lba, boot_bytes, PHASE_WRITE_BOOT, ERR_WRITE_BOOT);

    SCRATCH_PHASE = PHASE_DONE;
    SCRATCH_STATUS = DONE_MAGIC;
    SCRATCH_ERR = 0u;

    for (;;)
        ;
}
