/*
 * atomik_coprocessor.c - ATOMiK RV64I Co-Processor Dispatch Implementation
 *
 * ATOMiK Project — March 2026
 * SPDX-License-Identifier: MIT
 */

#include "atomik_coprocessor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>

/* Mailbox physical base address (from block_design.tcl) */
#define MBOX_PHYS_BASE  0x43C10000

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

coproc_t *coproc_open(const char *uio_dev, const char *mem_dev,
                      uint32_t fw_phys, uint32_t fw_size)
{
    coproc_t *cp = calloc(1, sizeof(coproc_t));
    if (!cp) return NULL;

    cp->mbox_fd = -1;
    cp->mem_fd  = -1;
    cp->fw_phys = fw_phys ? fw_phys : COPROC_DEFAULT_FW_ADDR;
    cp->fw_max_size = fw_size ? fw_size : COPROC_DEFAULT_FW_SIZE;

    /* Open /dev/mem for DDR3 firmware region */
    if (!mem_dev) mem_dev = "/dev/mem";
    cp->mem_fd = open(mem_dev, O_RDWR | O_SYNC);
    if (cp->mem_fd < 0) {
        perror("coproc: open /dev/mem");
        goto fail;
    }

    /* mmap firmware region in DDR3 */
    cp->fw_region = mmap(NULL, cp->fw_max_size, PROT_READ | PROT_WRITE,
                         MAP_SHARED, cp->mem_fd, cp->fw_phys);
    if (cp->fw_region == MAP_FAILED) {
        perror("coproc: mmap firmware region");
        cp->fw_region = NULL;
        goto fail;
    }

    /* mmap mailbox registers */
    if (uio_dev) {
        /* Via UIO device */
        cp->mbox_fd = open(uio_dev, O_RDWR);
        if (cp->mbox_fd < 0) {
            perror("coproc: open UIO");
            goto fail;
        }
        cp->mbox_regs = mmap(NULL, MBOX_MMAP_SIZE, PROT_READ | PROT_WRITE,
                             MAP_SHARED, cp->mbox_fd, 0);
    } else {
        /* Direct via /dev/mem */
        cp->mbox_regs = mmap(NULL, MBOX_MMAP_SIZE, PROT_READ | PROT_WRITE,
                             MAP_SHARED, cp->mem_fd, MBOX_PHYS_BASE);
    }

    if (cp->mbox_regs == MAP_FAILED) {
        perror("coproc: mmap mailbox");
        cp->mbox_regs = NULL;
        goto fail;
    }

    /* Ensure co-processor is stopped */
    coproc_stop(cp);

    /* Set firmware address in mailbox */
    coproc_reg_write(cp, MBOX_REG_FW_ADDR, cp->fw_phys);

    return cp;

fail:
    coproc_close(cp);
    return NULL;
}

void coproc_close(coproc_t *cp)
{
    if (!cp) return;

    /* Stop co-processor if running */
    if (cp->mbox_regs)
        coproc_stop(cp);

    /* Unmap mailbox */
    if (cp->mbox_regs) {
        munmap((void *)cp->mbox_regs, MBOX_MMAP_SIZE);
        cp->mbox_regs = NULL;
    }

    /* Unmap firmware region */
    if (cp->fw_region) {
        munmap((void *)cp->fw_region, cp->fw_max_size);
        cp->fw_region = NULL;
    }

    /* Close file descriptors */
    if (cp->mbox_fd >= 0) {
        close(cp->mbox_fd);
        cp->mbox_fd = -1;
    }
    if (cp->mem_fd >= 0) {
        close(cp->mem_fd);
        cp->mem_fd = -1;
    }

    free(cp);
}

/* =========================================================================
 * Firmware Loading
 * ========================================================================= */

int coproc_load_firmware(coproc_t *cp, const char *filename)
{
    FILE *f;
    long fsize;
    size_t nread;

    if (!cp || !filename) {
        errno = EINVAL;
        return -1;
    }

    if (coproc_is_running(cp)) {
        fprintf(stderr, "coproc: cannot load firmware while running\n");
        errno = EBUSY;
        return -1;
    }

    f = fopen(filename, "rb");
    if (!f) {
        perror("coproc: open firmware file");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0 || (size_t)fsize > cp->fw_max_size) {
        fprintf(stderr, "coproc: firmware size %ld exceeds max %u\n",
                fsize, cp->fw_max_size);
        fclose(f);
        errno = EFBIG;
        return -1;
    }

    /* Clear firmware region first */
    memset((void *)cp->fw_region, 0, cp->fw_max_size);

    /* Load firmware binary */
    nread = fread((void *)cp->fw_region, 1, (size_t)fsize, f);
    fclose(f);

    if (nread != (size_t)fsize) {
        fprintf(stderr, "coproc: short read (%zu of %ld bytes)\n",
                nread, fsize);
        errno = EIO;
        return -1;
    }

    cp->fw_loaded = (uint32_t)fsize;

    /* Update mailbox with firmware info */
    coproc_reg_write(cp, MBOX_REG_FW_ADDR, cp->fw_phys);
    coproc_reg_write(cp, MBOX_REG_FW_SIZE, cp->fw_loaded);

    return 0;
}

int coproc_load_firmware_buf(coproc_t *cp, const void *data, size_t size)
{
    if (!cp || !data || size == 0) {
        errno = EINVAL;
        return -1;
    }

    if (coproc_is_running(cp)) {
        errno = EBUSY;
        return -1;
    }

    if (size > cp->fw_max_size) {
        errno = EFBIG;
        return -1;
    }

    memset((void *)cp->fw_region, 0, cp->fw_max_size);
    memcpy((void *)cp->fw_region, data, size);

    cp->fw_loaded = (uint32_t)size;
    coproc_reg_write(cp, MBOX_REG_FW_ADDR, cp->fw_phys);
    coproc_reg_write(cp, MBOX_REG_FW_SIZE, cp->fw_loaded);

    return 0;
}

/* =========================================================================
 * CPU Lifecycle
 * ========================================================================= */

int coproc_start(coproc_t *cp)
{
    if (!cp) return -1;

    if (cp->fw_loaded == 0) {
        fprintf(stderr, "coproc: no firmware loaded\n");
        errno = ENOENT;
        return -1;
    }

    /* Deassert reset: CTRL[0] = 1 (run) */
    coproc_reg_write(cp, MBOX_REG_CTRL, MBOX_CTRL_RUN);
    return 0;
}

void coproc_stop(coproc_t *cp)
{
    if (!cp || !cp->mbox_regs) return;

    /* Assert reset: CTRL = 0 */
    coproc_reg_write(cp, MBOX_REG_CTRL, 0);
}

int coproc_is_running(coproc_t *cp)
{
    if (!cp || !cp->mbox_regs) return 0;
    return MBOX_STATUS_RUNNING(coproc_reg_read(cp, MBOX_REG_STATUS));
}

uint32_t coproc_read_pc(coproc_t *cp)
{
    if (!cp) return 0;
    return coproc_reg_read(cp, MBOX_REG_DEBUG_PC);
}

/* =========================================================================
 * Command / Response Protocol
 * ========================================================================= */

int coproc_send_cmd(coproc_t *cp, uint32_t cmd,
                    uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    uint32_t status;

    if (!cp) return -1;

    /* Check if previous command is still pending */
    status = coproc_reg_read(cp, MBOX_REG_STATUS);
    if (MBOX_STATUS_CMD_PENDING(status)) {
        errno = EBUSY;
        return -1;
    }

    /* Write arguments first, then command (CMD write sets cmd_pending) */
    coproc_reg_write(cp, MBOX_REG_ARG0, arg0);
    coproc_reg_write(cp, MBOX_REG_ARG1, arg1);
    coproc_reg_write(cp, MBOX_REG_ARG2, arg2);
    coproc_reg_write(cp, MBOX_REG_CMD, cmd);

    return 0;
}

/* Get monotonic time in milliseconds */
static uint64_t time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

int coproc_wait_resp(coproc_t *cp, uint32_t *resp0, uint32_t *resp1,
                     uint32_t timeout_ms)
{
    uint64_t deadline;
    uint32_t status;

    if (!cp) return -1;

    deadline = timeout_ms ? time_ms() + timeout_ms : 0;

    for (;;) {
        status = coproc_reg_read(cp, MBOX_REG_STATUS);
        if (MBOX_STATUS_RESP_READY(status))
            break;

        if (deadline && time_ms() >= deadline) {
            errno = ETIMEDOUT;
            return -1;
        }

        /* Brief yield to avoid burning CPU */
        usleep(10);
    }

    /* Read RESP0 first (clears resp_ready flag in hardware) */
    if (resp0)
        *resp0 = coproc_reg_read(cp, MBOX_REG_RESP0);
    else
        (void)coproc_reg_read(cp, MBOX_REG_RESP0);  /* Still read to clear flag */

    if (resp1)
        *resp1 = coproc_reg_read(cp, MBOX_REG_RESP1);

    return 0;
}

void coproc_doorbell(coproc_t *cp)
{
    if (!cp) return;
    coproc_reg_write(cp, MBOX_REG_DOORBELL, 0x1);  /* Bit 0 = ARM→RV64I */
}
