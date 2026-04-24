/* ============================================================================
 * Minimal FAT32 read-only reader
 *
 * Supports:
 *   - MBR partition table (picks first FAT32-type entry: 0x0B or 0x0C)
 *   - FAT32 BPB parsing
 *   - Cluster chain walk
 *   - File lookup by 8.3 short filename in the root directory
 *   - Read whole file to destination buffer
 *
 * Does NOT support:
 *   - Long filenames (LFN)
 *   - Subdirectories (root only)
 *   - Writing
 *   - Caching — every block is read fresh from SD
 * ========================================================================= */

#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include "sdhc.h"

typedef struct {
    sd_card_t *sd;

    uint32_t part_lba;         /* LBA of start of FAT32 partition    */
    uint32_t fat_lba;          /* LBA of first FAT                   */
    uint32_t data_lba;         /* LBA of start of cluster 2          */
    uint32_t sectors_per_clus; /* 1, 2, 4, 8, ... 128                */
    uint32_t root_cluster;     /* first cluster of root directory    */
    uint32_t fat_size_sec;
} fat32_t;

typedef enum {
    FAT_OK          = 0,
    FAT_ESD         = -10,   /* underlying SD read failed */
    FAT_EBADMBR     = -11,
    FAT_EBADBPB     = -12,
    FAT_ENOFILE     = -13,
    FAT_EFILETOOBIG = -14,
    FAT_EBADCLUS    = -15,
} fat_err_t;

/* Mount a FAT32 partition from the first MBR partition entry with FAT32 type.
 * Fills fs->* for later file reads. */
fat_err_t fat32_mount(fat32_t *fs, sd_card_t *sd);

/* Read an entire file by 8.3 filename (e.g. "IMAGE69") from root directory.
 * Writes contents to `dst`. `max_bytes` caps the read; returns actual size
 * via *size_out. Name is case-insensitive and may include dot + extension. */
fat_err_t fat32_read_file(fat32_t *fs, const char *name83,
                          void *dst, uint32_t max_bytes,
                          uint32_t *size_out);

#endif
