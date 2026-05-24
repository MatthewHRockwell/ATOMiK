/* ============================================================================
 * fat32.c — minimal FAT32 read-only reader for the PS loader
 * ========================================================================= */

#include "fat32.h"

#define SECTOR_SIZE  512

/* Debug markers (optional) */
static void fat_mark(uint32_t v) { *(volatile uint32_t *)0x00200700u = v; (void)v; }
__attribute__((unused)) static void fat_mark_unused(void) { fat_mark(0); }

/* Little-endian reads from a byte buffer. With MMU disabled, memory is
 * Strongly Ordered and unaligned LDR/LDRH trap. Force byte-at-a-time via
 * volatile pointers so GCC doesn't combine loads. */
static uint16_t le16(const uint8_t *p) {
    volatile const uint8_t *vp = p;
    return (uint16_t)(vp[0] | ((uint16_t)vp[1] << 8));
}
static uint32_t le32(const uint8_t *p) {
    volatile const uint8_t *vp = p;
    return (uint32_t)vp[0] | ((uint32_t)vp[1] << 8)
         | ((uint32_t)vp[2] << 16) | ((uint32_t)vp[3] << 24);
}

static uint8_t upper(uint8_t c)
{
    return (c >= 'a' && c <= 'z') ? (uint8_t)(c - 32) : c;
}

/* Split "IMAGE69" or "LINUX69.DTB" into 8-char name + 3-char ext (space-padded,
 * uppercase) matching the on-disk short-name entry format. */
static void parse_83(const char *src, uint8_t out[11])
{
    for (int i = 0; i < 11; i++) out[i] = ' ';
    int i = 0, j = 0;
    while (src[i] && src[i] != '.' && j < 8) {
        out[j++] = upper((uint8_t)src[i++]);
    }
    if (src[i] == '.') i++;
    j = 8;
    while (src[i] && j < 11) {
        out[j++] = upper((uint8_t)src[i++]);
    }
}

/* Read one sector from the SD card */
static fat_err_t rd_sector(sd_card_t *sd, uint32_t lba, uint8_t buf[SECTOR_SIZE])
{
    return (sdhc_read_block(sd, lba, buf) == SDHC_OK) ? FAT_OK : FAT_ESD;
}

/* Follow FAT32 chain; return next cluster, or 0x0FFFFFFF (EOF) / 0 (bad). */
static fat_err_t fat_next(fat32_t *fs, uint32_t cur, uint32_t *next_out)
{
    uint32_t fat_byte = cur * 4;
    uint32_t sec = fs->fat_lba + (fat_byte / SECTOR_SIZE);
    uint32_t off = fat_byte % SECTOR_SIZE;
    static uint8_t fat_buf[SECTOR_SIZE] __attribute__((aligned(4)));
    fat_err_t e = rd_sector(fs->sd, sec, fat_buf);
    if (e) return e;
    *next_out = le32(fat_buf + off) & 0x0FFFFFFFu;
    return FAT_OK;
}

fat_err_t fat32_mount(fat32_t *fs, sd_card_t *sd)
{
    static uint8_t buf[SECTOR_SIZE] __attribute__((aligned(4)));
    fat_err_t e;

    fat_mark(0xF1);
    fs->sd = sd;
    fs->part_lba = 0;
    fs->fat_lba  = 0;
    fs->data_lba = 0;
    fs->sectors_per_clus = 0;
    fs->root_cluster = 0;
    fs->fat_size_sec = 0;

    /* Read MBR (LBA 0) */
    e = rd_sector(sd, 0, buf);
    if (e) { fat_mark(0xF2); return e; }
    if (buf[510] != 0x55 || buf[511] != 0xAA) { fat_mark(0xF3); return FAT_EBADMBR; }

    fat_mark(0xF4);

    /* Walk partition table, accept first entry that has a plausible LBA
     * start AND valid sector count (both nonzero, within reasonable range
     * for an SD card — < 512 million sectors = 256 GB). */
    uint32_t p_lba = 0;
    int found = 0;
    for (int i = 0; i < 4; i++) {
        fat_mark(0xA0 | i);
        const uint8_t *pe = buf + 0x1BE + i * 16;
        fat_mark(0xB0 | i);
        uint32_t start  = le32(pe + 8);
        uint32_t length = le32(pe + 12);
        fat_mark(0xC0 | i);
        /* Reject clearly garbage entries */
        if (start == 0 || start > (512u * 1024u * 1024u)) continue;
        if (length == 0 || length > (512u * 1024u * 1024u)) continue;
        p_lba = start;
        found = 1;
        break;
    }
    fat_mark(0xF5);

    /* Fallback for superfloppy / non-standard MBR: probe the common BPB LBAs
     * and pick the first one with a valid FAT32 signature. */
    if (!found) {
        static const uint32_t candidates[] = { 8192, 2048, 63, 0, 1, 128, 256 };
        for (uint32_t ci = 0; ci < sizeof(candidates)/sizeof(candidates[0]); ci++) {
            e = rd_sector(sd, candidates[ci], buf);
            if (e) continue;
            if (buf[510] == 0x55 && buf[511] == 0xAA
                && (buf[0] == 0xEB || buf[0] == 0xE9)
                && le16(buf + 11) == SECTOR_SIZE) {
                p_lba = candidates[ci];
                found = 1;
                break;
            }
        }
    }
    if (!found) { fat_mark(0xF7); return FAT_EBADMBR; }
    fs->part_lba = p_lba;

    /* Re-read BPB from the chosen LBA (may already be in buf but be safe) */
    fat_mark(0xF8);
    e = rd_sector(sd, p_lba, buf);
    if (e) { fat_mark(0xE8); return e; }
    if (buf[510] != 0x55 || buf[511] != 0xAA) { fat_mark(0xF9); return FAT_EBADBPB; }
    if (buf[0] != 0xEB && buf[0] != 0xE9) { fat_mark(0xE9); return FAT_EBADBPB; }

    uint16_t bps = le16(buf + 11);           /* BPB_BytsPerSec */
    if (bps != SECTOR_SIZE) return FAT_EBADBPB;

    uint8_t  spc   = buf[13];
    uint16_t rsvd  = le16(buf + 14);
    uint8_t  nfats = buf[16];
    uint32_t fsz32 = le32(buf + 36);
    uint32_t rootc = le32(buf + 44);

    if (spc == 0 || nfats == 0 || fsz32 == 0) { fat_mark(0xFA); return FAT_EBADBPB; }

    fs->sectors_per_clus = spc;
    fs->fat_lba          = p_lba + rsvd;
    fs->data_lba         = p_lba + rsvd + (uint32_t)nfats * fsz32;
    fs->root_cluster     = rootc;
    fs->fat_size_sec     = fsz32;
    fat_mark(0xFF);
    return FAT_OK;
}

/* LFN reassembly state. LFN entries precede their short-name entry in the
 * directory, with the last-in-name LFN entry first (sequence number bits
 * [4:0] start at 1). Each LFN entry holds 13 UCS-2 chars at bytes
 * 1..10 (5 chars), 14..25 (6 chars), 28..31 (2 chars).  We only handle
 * ASCII (low byte of each UCS-2 pair). */
#define LFN_MAX_CHARS  256
/* buf is sized LFN_MAX_CHARS + 2 (not +1) so that next_ord/valid land on an
 * even offset. With +1, GCC -Os merges the two trailing byte writes in
 * lfn_reset() into a single STRH at an odd address — which Cortex-A9 traps
 * as an unaligned access whenever MMU is off (memory is Strongly Ordered). */
typedef struct {
    char     buf[LFN_MAX_CHARS + 2];   /* assembled long name, NUL-terminated */
    uint8_t  next_ord;                 /* expected next ordinal (counts down) */
    uint8_t  valid;
} lfn_state_t;

static void lfn_reset(lfn_state_t *st)
{
    st->buf[0] = 0;
    st->next_ord = 0;
    st->valid = 0;
}

static void lfn_consume(lfn_state_t *st, const uint8_t *de)
{
    uint8_t ord = de[0];
    int last = (ord & 0x40) != 0;
    uint8_t seq = ord & 0x1F;       /* 1..20 */
    if (seq < 1 || seq > 20) { lfn_reset(st); return; }

    if (last) {
        /* Last (highest-numbered) LFN entry of a name — encountered first */
        lfn_reset(st);
        st->next_ord = seq;
    }
    if (seq != st->next_ord) { lfn_reset(st); return; }
    st->next_ord--;

    /* Slot holds 13 chars; we place them at offset (seq-1)*13 in buf */
    int off = (seq - 1) * 13;
    if (off + 13 >= LFN_MAX_CHARS) { lfn_reset(st); return; }

    /* Bytes 1-10: 5 chars; 14-25: 6 chars; 28-31: 2 chars (UCS-2 LE) */
    static const int positions[] = {1,3,5,7,9, 14,16,18,20,22,24, 28,30};
    int pad = 0;
    for (int i = 0; i < 13; i++) {
        uint8_t lo = de[positions[i]];
        uint8_t hi = de[positions[i] + 1];
        if (pad || (lo == 0 && hi == 0)) { st->buf[off + i] = 0; pad = 1; continue; }
        if (lo == 0xFF && hi == 0xFF)    { st->buf[off + i] = 0; pad = 1; continue; }
        st->buf[off + i] = (char)lo;     /* ASCII only */
    }

    if (st->next_ord == 0) {
        /* All LFN entries collected; ensure NUL termination */
        st->valid = 1;
        for (int i = 0; i < LFN_MAX_CHARS; i++) {
            if (st->buf[i] == 0) break;
        }
    }
}

/* Case-insensitive ASCII compare */
static int ieq(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

/* Scan root directory for a name match — by short name (8.3) OR by long
 * filename (LFN). Returns first cluster and size, or FAT_ENOFILE. */
static fat_err_t find_root_entry(fat32_t *fs,
                                 const uint8_t name83[11],
                                 const char    *long_name,
                                 uint32_t *first_clus_out,
                                 uint32_t *size_out)
{
    static uint8_t buf[SECTOR_SIZE] __attribute__((aligned(4)));
    uint32_t clus = fs->root_cluster;
    lfn_state_t lfn;
    lfn_reset(&lfn);

    uint32_t safety = 100;
    while (clus >= 2 && clus < 0x0FFFFFF8u && safety--) {
        uint32_t sec0 = fs->data_lba + (clus - 2) * fs->sectors_per_clus;
        for (uint32_t s = 0; s < fs->sectors_per_clus; s++) {
            fat_err_t e = rd_sector(fs->sd, sec0 + s, buf);
            if (e) return e;
            for (int ent = 0; ent < SECTOR_SIZE; ent += 32) {
                uint8_t *de = buf + ent;
                if (de[0] == 0x00) return FAT_ENOFILE;     /* end of dir   */
                if (de[0] == 0xE5) { lfn_reset(&lfn); continue; } /* deleted */
                if (de[11] == 0x0F) { lfn_consume(&lfn, de); continue; }
                if (de[11] & 0x08)  { lfn_reset(&lfn); continue; } /* vol label */

                /* Short-name entry: try LFN match first, then 8.3 match */
                int match = 0;
                if (lfn.valid && long_name && ieq(lfn.buf, long_name)) {
                    match = 1;
                }
                if (!match) {
                    match = 1;
                    for (int k = 0; k < 11; k++) {
                        if (de[k] != name83[k]) { match = 0; break; }
                    }
                }
                if (match) {
                    uint16_t hi = le16(de + 20);
                    uint16_t lo = le16(de + 26);
                    *first_clus_out = ((uint32_t)hi << 16) | lo;
                    *size_out = le32(de + 28);
                    return FAT_OK;
                }
                lfn_reset(&lfn);    /* short-name entry consumed */
            }
        }
        uint32_t next;
        fat_err_t e = fat_next(fs, clus, &next);
        if (e) return e;
        clus = next;
    }
    return FAT_ENOFILE;
}

fat_err_t fat32_read_file(fat32_t *fs, const char *name83,
                          void *dst, uint32_t max_bytes,
                          uint32_t *size_out)
{
    uint8_t name[11];
    parse_83(name83, name);

    uint32_t first_clus, fsize;
    /* Pass long_name (== name83 for ASCII filenames) for LFN match too */
    fat_err_t e = find_root_entry(fs, name, name83, &first_clus, &fsize);
    if (e) return e;
    if (fsize > max_bytes) return FAT_EFILETOOBIG;
    *size_out = fsize;

    if (fsize == 0) return FAT_OK;
    if (first_clus < 2 || first_clus >= 0x0FFFFFF8u) return FAT_EBADCLUS;

    uint32_t bytes_per_clus = (uint32_t)fs->sectors_per_clus * SECTOR_SIZE;
    uint8_t  *out = (uint8_t *)dst;
    uint32_t remaining = fsize;
    uint32_t clus = first_clus;

    while (remaining > 0) {
        uint32_t sec0 = fs->data_lba + (clus - 2) * fs->sectors_per_clus;

        /* Full sectors in this cluster */
        for (uint32_t s = 0; s < fs->sectors_per_clus && remaining > 0; s++) {
            if (remaining >= SECTOR_SIZE) {
                e = rd_sector(fs->sd, sec0 + s, out);
                if (e) return e;
                out       += SECTOR_SIZE;
                remaining -= SECTOR_SIZE;
            } else {
                /* Tail: read to a temp sector, then memcpy the tail */
                static uint8_t tmp[SECTOR_SIZE] __attribute__((aligned(4)));
                e = rd_sector(fs->sd, sec0 + s, tmp);
                if (e) return e;
                for (uint32_t k = 0; k < remaining; k++) out[k] = tmp[k];
                remaining = 0;
            }
        }
        if (remaining == 0) break;

        uint32_t next;
        e = fat_next(fs, clus, &next);
        if (e) return e;
        if (next < 2 || next >= 0x0FFFFFF8u) return FAT_EBADCLUS;
        clus = next;
        (void)bytes_per_clus;
    }
    return FAT_OK;
}
