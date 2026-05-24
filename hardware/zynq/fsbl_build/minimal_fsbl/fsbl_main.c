/* fsbl_main.c — ATOMiK minimal FSBL.
 *
 * Runs from OCM (0x00000000) before DDR is initialized.
 * Sequence:
 *   1. ps7_init()       — initialize PS (DDR, PLLs, MIO, etc.)
 *   2. SD init          — initialize SDIO0 + card
 *   3. Load bitstream   — read .bit.bin from SD to DDR staging area
 *   4. PCAP program     — configure PL with bitstream
 *   5. ps7_post_config  — enable PS-PL interfaces
 *   6. Load Linux imgs  — fw_jump69.bin, linux69.dtb, rootfs69.cpio, Image69
 *   7. Signal LiteX     — write DONE_MAGIC to 0x10100000
 *   8. Heartbeat        — LiteX BIOS auto-boots (polls scratch)
 *
 * Memory map (PS DDR after init):
 *   0x00100000  kernel (Image69, up to 32 MB)
 *   0x00FF0000  DTB (linux69.dtb, 256 KB max)
 *   0x01000000  OpenSBI (fw_jump69.bin, 1 MB max)
 *   0x02100000  rootfs (rootfs69.cpio, 64 MB max)
 *   0x04000000  bitstream staging (nax64.bit.bin, 4 MB max)
 *   0x10100000  scratch area (DONE_MAGIC written here)
 */

#include <stdint.h>
#include "zynq_regs.h"
#include "sdhc.h"
#include "fat32.h"
#include "ps7_init.h"
#include "pcap_prog.h"

/* These helpers are inline/static in ps_loader main.c; replicate here. */
static inline void slcr_unlock(void) { w32(SLCR_UNLOCK, 0xDF0Du); dsb(); }

static void sdio_pinmux(void) {
    slcr_unlock();
    for (int pin = 40; pin <= 47; pin++)
        w32(SLCR_MIO_PIN(pin), MIO_CFG_SDIO1);
    dsb();
}

static void sdio_clock_enable_both(void) {
    slcr_unlock();
    uint32_t aper = r32(SLCR_APER_CLK_CTRL);
    w32(SLCR_APER_CLK_CTRL, aper | APER_CLK_SDI0 | APER_CLK_SDI1);
    w32(SLCR_SDIO_CLK_CTRL, SDIO_CLK_CTRL_50MHz);
    w32(SLCR_SDIO_RST_CTRL, 0);
    dsb();
    for (volatile int d = 0; d < 10000; d++) {}
}

/* ps7_post_config: enable PS-PL level shifters after bitstream load.
 * Same register sequence as ps7_post_config in JTAG boot TCL. */
static void do_ps7_post_config(void)
{
    /* SLCR must be unlocked before LVL_SHFTR_EN write will take effect.
     * pcap_program re-locks SLCR at the end so we unlock again here. */
    *(volatile uint32_t *)0xF8000008u = 0x0000DF0Du;  /* SLCR unlock */
    /* Enable level shifters: bits[3:0] = 0xF enables both directions */
    *(volatile uint32_t *)0xF8000900u = 0x0000000Fu;
    /* Small delay */
    for (volatile int i = 0; i < 100000; i++) {}
}

#define SCRATCH_BASE     0x10100000u
#define S_DONE           (SCRATCH_BASE + 0x00u)
#define S_MAGIC          (SCRATCH_BASE + 0x04u)
#define S_HEARTBEAT      (SCRATCH_BASE + 0x08u)
#define S_MARKER         (SCRATCH_BASE + 0x10u)
#define S_LAST_ERR       (SCRATCH_BASE + 0x14u)
#define DONE_MAGIC       0xC0DEC0DEu

/* Bitstream staging area — 64 MB above images, 4 MB budget */
#define DDR_BITSTREAM    0x04000000u
#define DDR_KERNEL       0x00100000u
#define DDR_DTB          0x00FF0000u
#define DDR_OPENSBI      0x01000000u
#define DDR_INITRAMFS    0x02100000u

static void m32(uint32_t a, uint32_t v) { *(volatile uint32_t *)(uintptr_t)a = v; }

typedef struct {
    const char *name;
    uint32_t    ddr_addr;
    uint32_t    max_size;
} boot_file_t;

static const boot_file_t boot_files[] = {
    /* Nax64 file set (matches the proven JTAG-boot path).
     * RV32-era *69.bin files are stale; the autoboot bitstream and DTB
     * here are RV64/NaxRiscv. */
    { "fw_jump_nax64.bin",   DDR_OPENSBI,    1u  * 1024u * 1024u },
    { "linux_nax64.dtb",     DDR_DTB,        256u * 1024u         },
    { "ubuntu_rv64.cpio.gz", DDR_INITRAMFS,  64u * 1024u * 1024u },
    { "Image_nax64",         DDR_KERNEL,     32u * 1024u * 1024u },
};
#define N_BOOT_FILES (sizeof(boot_files) / sizeof(boot_files[0]))

void main(void)
{
    /* === Stage 0: liveness markers (OCM — DDR not yet initialized) === */
    /* NOTE: These writes go to OCM (we're running from 0x00000000).
     * Scratch is at 0x10100000 which is DDR — skip until after ps7_init. */

    /* === Stage 1: Initialize PS (DDR, PLLs, MIO) === */
    int rc = ps7_init();
    /* rc = 0 on success; we continue regardless (no place to report failure) */

    /* Now DDR is initialized — we can use 0x10100000+ for scratch */
    m32(S_DONE,      0u);
    m32(S_MAGIC,     0xA70A1CBAu);
    m32(S_HEARTBEAT, 0u);
    m32(S_MARKER,    0x10u);
    m32(S_LAST_ERR,  (uint32_t)rc);

    /* === Stage 2: Initialize SD card === */
    sdio_pinmux();
    sdio_clock_enable_both();

    sd_card_t card;
    sdhc_err_t e = sdhc_controller_init(&card, SD0_BASE);
    m32(S_LAST_ERR, (uint32_t)e);
    if (e != SDHC_OK) { m32(S_MARKER, 0x20u); goto heartbeat; }

    e = sdhc_card_init(&card);
    m32(S_LAST_ERR, (uint32_t)e);
    if (e != SDHC_OK) { m32(S_MARKER, 0x21u); goto heartbeat; }

    m32(S_MARKER, 0x30u);

    /* === Stage 3: Mount FAT32 === */
    fat32_t fs;
    fat_err_t fe = fat32_mount(&fs, &card);
    m32(S_LAST_ERR, (uint32_t)fe);
    if (fe != FAT_OK) { m32(S_MARKER, 0x40u); goto heartbeat; }

    /* === Stage 4: Load bitstream to DDR staging area === */
    m32(S_MARKER, 0x50u);
    uint32_t bit_sz = 0;
    fe = fat32_read_file(&fs, "nax64.bit.bin",
                         (void *)(uintptr_t)DDR_BITSTREAM,
                         4u * 1024u * 1024u, &bit_sz);
    m32(S_LAST_ERR, (uint32_t)fe);
    if (fe != FAT_OK || bit_sz == 0) { m32(S_MARKER, 0x55u); goto heartbeat; }

    /* === Stage 5: Program PL via PCAP === */
    m32(S_MARKER, 0x58u);
    int pcap_rc = pcap_program((void *)(uintptr_t)DDR_BITSTREAM, bit_sz / 4u);
    m32(S_LAST_ERR, (uint32_t)pcap_rc);
    if (pcap_rc != 0) { m32(S_MARKER, 0x5Au); goto heartbeat; }

    /* === Stage 6: ps7_post_config — enable PS-PL interfaces === */
    do_ps7_post_config();
    m32(S_MARKER, 0x60u);

    /* === Stage 7: Load Linux images from SD === */
    int ok_count = 0;
    for (uint32_t i = 0; i < N_BOOT_FILES; i++) {
        const boot_file_t *bf = &boot_files[i];
        m32(S_MARKER, 0x60u | i);
        uint32_t sz = 0;
        fe = fat32_read_file(&fs, bf->name,
                             (void *)(uintptr_t)bf->ddr_addr,
                             bf->max_size, &sz);
        if (fe == FAT_OK) ok_count++;
    }

    if (ok_count == (int)N_BOOT_FILES) {
        m32(S_MARKER, 0xC0u);
        m32(S_DONE,   DONE_MAGIC);   /* LiteX BIOS will auto-boot */
    } else {
        m32(S_MARKER, 0xE0u | (uint32_t)ok_count);
    }

heartbeat:
    {
        uint32_t i = 0;
        for (;;) {
            m32(S_HEARTBEAT, ++i);
            for (volatile int d = 0; d < 100000; d++) {}
        }
    }
}
