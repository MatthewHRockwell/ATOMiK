/* ============================================================================
 * ATOMiK PS Loader — 9.1.d full SD → DDR boot loader
 *
 * Memory map (PS view of DDR):
 *   0x00000000  ARM exception vectors + low DDR (free, used as scratch headers
 *               for protocol with the orchestration script)
 *   0x00100000  NaxRiscv main_ram base (kernel goes here, 8.5 MB)
 *   0x00FF0000  Linux device tree blob
 *   0x01000000  OpenSBI fw_jump (256 KB)
 *   0x02100000  rootfs initramfs (~30 MB)
 *   0x10000000  this loader (256 KB code + stack)
 *   0x10100000  scratch / xsdb readback area (well clear of all images)
 *
 * NaxRiscv view of the same DDR (via LiteX wishbone remapper, offset 0x40000000):
 *   0x40000000 ← PS 0x00100000  (kernel)
 *   0x40EF0000 ← PS 0x00FF0000  (dtb)
 *   0x40F00000 ← PS 0x01000000  (opensbi)
 *   0x42000000 ← PS 0x02100000  (initramfs)
 *
 * Boot trigger: orchestration script (host side) sends `boot 0x40f00000\n`
 * over /dev/ttyUSB1 (LiteX BIOS serial) once this loader signals "done"
 * by writing 0xC0DEC0DE to scratch_base + 0x00.
 * ========================================================================= */

#include <stdint.h>
#include "zynq_regs.h"
#include "sdhc.h"
#include "fat32.h"

#define SCRATCH_BASE     0x10100000u
#define S_DONE           (SCRATCH_BASE + 0x00)
#define S_MAGIC          (SCRATCH_BASE + 0x04)
#define S_HEARTBEAT      (SCRATCH_BASE + 0x08)
#define S_MARKER         (SCRATCH_BASE + 0x10)
#define S_LAST_ERR       (SCRATCH_BASE + 0x14)
#define S_OCR            (SCRATCH_BASE + 0x18)
#define S_RCA            (SCRATCH_BASE + 0x1C)
#define S_CID            (SCRATCH_BASE + 0x20)   /* 16 bytes */
#define S_FAT_PART_LBA   (SCRATCH_BASE + 0x40)
#define S_FAT_DATA_LBA   (SCRATCH_BASE + 0x44)
#define S_FAT_SPC        (SCRATCH_BASE + 0x48)
#define S_FILE_RESULTS   (SCRATCH_BASE + 0x80)   /* 4 entries × 16 B */

#define DONE_MAGIC       0xC0DEC0DEu

/* PS DDR target addresses for the NaxRiscv boot images. */
#define DDR_KERNEL       0x00100000u
#define DDR_DTB          0x00FF0000u
#define DDR_OPENSBI      0x01000000u
#define DDR_INITRAMFS    0x02100000u

/* Per-file load descriptor */
typedef struct {
    const char *name;       /* full filename (LFN-friendly) */
    uint32_t    ddr_addr;   /* PS DDR target */
    uint32_t    max_size;   /* upper bound for fat32_read_file */
} boot_file_t;

static const boot_file_t boot_files[] = {
    /* Order matters: kernel last so its 8 MB doesn't overwrite DTB before DTB read */
    { "fw_jump69.bin",  DDR_OPENSBI,    1u * 1024u * 1024u  },
    { "linux69.dtb",    DDR_DTB,         256u * 1024u       },
    { "rootfs69.cpio",  DDR_INITRAMFS,   64u * 1024u * 1024u },
    { "Image69",        DDR_KERNEL,      32u * 1024u * 1024u },
};
#define N_BOOT_FILES  (sizeof(boot_files) / sizeof(boot_files[0]))

static void m32(uint32_t a, uint32_t v) { *(volatile uint32_t *)(uintptr_t)a = v; }
static uint32_t read_mpidr(void) {
    uint32_t v; __asm__ volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r"(v)); return v;
}

static void slcr_unlock_local(void) { w32(SLCR_UNLOCK, 0xDF0Du); dsb(); }

static void sdio_pinmux(void) {
    slcr_unlock_local();
    for (int pin = 40; pin <= 47; pin++) {
        w32(SLCR_MIO_PIN(pin), MIO_CFG_SDIO1);
    }
    dsb();
}

static void sdio_clock_enable_both(void) {
    slcr_unlock_local();
    uint32_t aper = r32(SLCR_APER_CLK_CTRL);
    w32(SLCR_APER_CLK_CTRL, aper | APER_CLK_SDI0 | APER_CLK_SDI1);
    w32(SLCR_SDIO_CLK_CTRL, SDIO_CLK_CTRL_50MHz);
    w32(SLCR_SDIO_RST_CTRL, 0);
    dsb();
    for (volatile int d = 0; d < 10000; d++) { }
}

void main(void)
{
    /* Always-visible liveness markers */
    m32(S_DONE,      0);
    m32(S_MAGIC,     0xA70A1CBAu);
    m32(S_HEARTBEAT, 0);
    m32(S_MARKER,    0x10);
    m32(S_LAST_ERR,  0);

    /* Bring up SDIO clocks + pinmux */
    sdio_pinmux();
    sdio_clock_enable_both();

    /* Init SDHC controller + card on SDIO0 (the channel actually wired
     * to MIO40-47 with L3_SEL=4 on this board). */
    sd_card_t card;
    sdhc_err_t e = sdhc_controller_init(&card, SD0_BASE);
    m32(S_LAST_ERR, (uint32_t)e);
    if (e != SDHC_OK) { m32(S_MARKER, 0x20); goto heartbeat; }

    e = sdhc_card_init(&card);
    m32(S_LAST_ERR, (uint32_t)e);
    if (e != SDHC_OK) { m32(S_MARKER, 0x21); goto heartbeat; }

    m32(S_OCR, card.ocr);
    m32(S_RCA, card.rca);
    for (int i = 0; i < 16; i += 4) {
        uint32_t w = ((uint32_t)card.cid[i] << 24) | ((uint32_t)card.cid[i+1] << 16)
                   | ((uint32_t)card.cid[i+2] << 8) | (uint32_t)card.cid[i+3];
        m32(S_CID + i, w);
    }
    m32(S_MARKER, 0x30);

    /* Mount FAT32 */
    fat32_t fs;
    fat_err_t fe = fat32_mount(&fs, &card);
    m32(S_LAST_ERR, (uint32_t)fe);
    m32(S_FAT_PART_LBA, fs.part_lba);
    m32(S_FAT_DATA_LBA, fs.data_lba);
    m32(S_FAT_SPC,      fs.sectors_per_clus);
    if (fe != FAT_OK) { m32(S_MARKER, 0x40); goto heartbeat; }
    m32(S_MARKER, 0x50);

    /* Load each file directly to its DDR target. fat32_read_file walks the
     * cluster chain and copies block by block. We pass long_name via
     * LFN-aware match so user-friendly filenames (Image69, rootfs69.cpio,
     * linux69.dtb, fw_jump69.bin) work as long as they exist on the card. */
    int ok_count = 0;
    for (uint32_t i = 0; i < N_BOOT_FILES; i++) {
        const boot_file_t *bf = &boot_files[i];
        m32(S_MARKER, 0x60 | i);

        uint32_t sz = 0;
        fat_err_t re = fat32_read_file(&fs, bf->name,
                                       (void *)(uintptr_t)bf->ddr_addr,
                                       bf->max_size, &sz);
        m32(S_FILE_RESULTS + i * 16 + 0,  (uint32_t)bf->ddr_addr);
        m32(S_FILE_RESULTS + i * 16 + 4,  sz);
        m32(S_FILE_RESULTS + i * 16 + 8,  (uint32_t)re);
        {
            volatile const uint8_t *vn = (const uint8_t *)bf->name;
            m32(S_FILE_RESULTS + i * 16 + 12,
                  (uint32_t)vn[0]
                | ((uint32_t)vn[1] << 8)
                | ((uint32_t)vn[2] << 16)
                | ((uint32_t)vn[3] << 24));
        }
        if (re == FAT_OK) ok_count++;
    }

    if (ok_count == (int)N_BOOT_FILES) {
        m32(S_MARKER, 0xC0);
        m32(S_DONE,   DONE_MAGIC);    /* signal orchestrator: ready to boot */
    } else {
        m32(S_MARKER, 0xE0 | (uint32_t)ok_count);
    }

heartbeat:
    {
        uint32_t i = 0;
        for (;;) {
            m32(S_HEARTBEAT, ++i);
            for (volatile int d = 0; d < 100000; ++d) { }
        }
    }
}
