/* SPDX-License-Identifier: GPL-2.0 */
/*
 * atomik_hw.h — Hardware MMIO backend interface
 *
 * Register map matches hardware/zynq/rtl/atomik_axi4lite_wrapper.v
 * AXI4-Lite slave at 0x43C00000 (4 KB), 32-bit data path.
 *
 * 64-bit operations use split LO/HI writes. The HI write triggers
 * the operation. All multi-register sequences must be serialized
 * via the io_lock mutex.
 */

#ifndef ATOMIK_HW_H
#define ATOMIK_HW_H

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/bitmap.h>

/* --- AXI4-Lite Register Offsets --- */

#define ATOMIK_REG_LOAD_ADDR     0x00  /* W: context addr for LOAD/ACCUM/READ */
#define ATOMIK_REG_LOAD_DATA_LO  0x04  /* W: initial state [31:0] */
#define ATOMIK_REG_LOAD_DATA_HI  0x08  /* W: initial state [63:32], triggers LOAD */
#define ATOMIK_REG_ACCUM_LO      0x0C  /* W: delta [31:0] */
#define ATOMIK_REG_ACCUM_HI      0x10  /* W: delta [63:32], triggers ACCUM */
#define ATOMIK_REG_STATE_LO      0x14  /* R: current state [31:0], latches 64-bit */
#define ATOMIK_REG_STATE_HI      0x18  /* R: current state [63:32] from latch */
#define ATOMIK_REG_STATUS        0x1C  /* R: [0]=acc_zero, [15:8]=n_banks, [23:16]=ver */
#define ATOMIK_REG_SWAP_ADDR     0x20  /* W: context addr, triggers SWAP */
#define ATOMIK_REG_CONFIG        0x24  /* RW: [0]=enable */

/* STATUS register field extraction */
#define ATOMIK_STATUS_ACC_ZERO(s)  ((s) & 1)
#define ATOMIK_STATUS_N_BANKS(s)   (((s) >> 8) & 0xFF)
#define ATOMIK_STATUS_VERSION(s)   (((s) >> 16) & 0xFF)

/* Hardware limits */
#define ATOMIK_HW_MAX_CONTEXTS   256  /* 8-bit address, BRAM-backed */

struct atomik_hw_device {
	void __iomem *regs;          /* ioremap'd register base */
	struct mutex io_lock;        /* serializes multi-register MMIO ops */
	u8 n_banks;                  /* parallel banks detected from STATUS */
	u8 hw_version;               /* hardware version from STATUS */
	DECLARE_BITMAP(ctx_bitmap, ATOMIK_HW_MAX_CONTEXTS);
	int ctx_allocated;           /* number of hw contexts in use */
};

/* Global hardware device (NULL = no hardware, software-only) */
extern struct atomik_hw_device *atomik_hw_dev;

/* --- Hardware backend API --- */

/* Returns true if ATOMiK hardware is present and initialized */
bool atomik_hw_available(void);

/* Allocate a contiguous block of N hardware context slots.
 * Returns base index on success, -ENOMEM if not enough slots. */
int atomik_hw_alloc_contexts(int n);

/* Free a contiguous block starting at base. */
void atomik_hw_free_contexts(int base, int n);

/* Core operations — caller must ensure hw_slot is valid and allocated */
void atomik_hw_load(int hw_slot, u64 value);
void atomik_hw_accum(int hw_slot, u64 delta);
u64  atomik_hw_read(int hw_slot);
u64  atomik_hw_swap(int hw_slot);

/* Hardware enable/disable */
void atomik_hw_set_enable(bool enable);
bool atomik_hw_is_enabled(void);

/* Platform driver registration (called from atomik_main.c) */
int  atomik_hw_driver_register(void);
void atomik_hw_driver_unregister(void);

#endif /* ATOMIK_HW_H */
