// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_hw.c — Hardware MMIO backend (platform_driver)
 *
 * Detects ATOMiK hardware via device tree ("atomik,delta-engine"),
 * ioremap's the AXI4-Lite register space, and provides hardware-
 * accelerated LOAD/ACCUM/READ/SWAP operations.
 *
 * Register protocol (from atomik_axi4lite_wrapper.v):
 *   - 32-bit AXI data bus, 64-bit ATOMiK datapath
 *   - 64-bit ops: write LO first, write HI triggers operation
 *   - All multi-register sequences serialized via io_lock
 *   - Read: read STATE_LO latches 64-bit snapshot, then read STATE_HI
 *
 * When no hardware is present, atomik_hw_dev remains NULL and the
 * chardev falls back to the software backend (atomik_core_kern.h).
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/bitmap.h>

#include "atomik_hw.h"

/* Global singleton — one hardware device per system */
struct atomik_hw_device *atomik_hw_dev;

/* --- Register access helpers --- */

static inline void hw_write32(struct atomik_hw_device *hw, u32 offset, u32 val)
{
	iowrite32(val, hw->regs + offset);
}

static inline u32 hw_read32(struct atomik_hw_device *hw, u32 offset)
{
	return ioread32(hw->regs + offset);
}

/* --- Public API --- */

bool atomik_hw_available(void)
{
	return atomik_hw_dev != NULL;
}

int atomik_hw_alloc_contexts(int n)
{
	struct atomik_hw_device *hw = atomik_hw_dev;
	int base, end;

	if (!hw || n <= 0 || n > ATOMIK_HW_MAX_CONTEXTS)
		return -EINVAL;

	mutex_lock(&hw->io_lock);

	/* Find first contiguous run of N free slots */
	base = bitmap_find_next_zero_area(hw->ctx_bitmap,
					  ATOMIK_HW_MAX_CONTEXTS,
					  0, n, 0);
	if (base >= ATOMIK_HW_MAX_CONTEXTS) {
		mutex_unlock(&hw->io_lock);
		return -ENOMEM;
	}

	end = base + n;
	bitmap_set(hw->ctx_bitmap, base, n);
	hw->ctx_allocated += n;

	mutex_unlock(&hw->io_lock);

	/* Initialize all allocated contexts to zero via hardware LOAD */
	for (int i = base; i < end; i++)
		atomik_hw_load(i, 0);

	return base;
}

void atomik_hw_free_contexts(int base, int n)
{
	struct atomik_hw_device *hw = atomik_hw_dev;

	if (!hw || base < 0 || n <= 0 ||
	    base + n > ATOMIK_HW_MAX_CONTEXTS)
		return;

	mutex_lock(&hw->io_lock);
	bitmap_clear(hw->ctx_bitmap, base, n);
	hw->ctx_allocated -= n;
	mutex_unlock(&hw->io_lock);
}

void atomik_hw_load(int hw_slot, u64 value)
{
	struct atomik_hw_device *hw = atomik_hw_dev;

	if (!hw)
		return;

	mutex_lock(&hw->io_lock);
	hw_write32(hw, ATOMIK_REG_LOAD_ADDR, (u32)hw_slot);
	hw_write32(hw, ATOMIK_REG_LOAD_DATA_LO, (u32)(value & 0xFFFFFFFF));
	hw_write32(hw, ATOMIK_REG_LOAD_DATA_HI, (u32)(value >> 32)); /* triggers */
	mutex_unlock(&hw->io_lock);
}

void atomik_hw_accum(int hw_slot, u64 delta)
{
	struct atomik_hw_device *hw = atomik_hw_dev;

	if (!hw)
		return;

	mutex_lock(&hw->io_lock);
	hw_write32(hw, ATOMIK_REG_LOAD_ADDR, (u32)hw_slot);
	hw_write32(hw, ATOMIK_REG_ACCUM_LO, (u32)(delta & 0xFFFFFFFF));
	hw_write32(hw, ATOMIK_REG_ACCUM_HI, (u32)(delta >> 32)); /* triggers */
	mutex_unlock(&hw->io_lock);
}

u64 atomik_hw_read(int hw_slot)
{
	struct atomik_hw_device *hw = atomik_hw_dev;
	u32 lo, hi;

	if (!hw)
		return 0;

	mutex_lock(&hw->io_lock);
	hw_write32(hw, ATOMIK_REG_LOAD_ADDR, (u32)hw_slot);
	lo = hw_read32(hw, ATOMIK_REG_STATE_LO); /* latches 64-bit snapshot */
	hi = hw_read32(hw, ATOMIK_REG_STATE_HI);
	mutex_unlock(&hw->io_lock);

	return ((u64)hi << 32) | lo;
}

u64 atomik_hw_swap(int hw_slot)
{
	struct atomik_hw_device *hw = atomik_hw_dev;
	u32 lo, hi;

	if (!hw)
		return 0;

	mutex_lock(&hw->io_lock);
	hw_write32(hw, ATOMIK_REG_SWAP_ADDR, (u32)hw_slot); /* triggers swap */
	lo = hw_read32(hw, ATOMIK_REG_STATE_LO); /* old state before swap */
	hi = hw_read32(hw, ATOMIK_REG_STATE_HI);
	mutex_unlock(&hw->io_lock);

	return ((u64)hi << 32) | lo;
}

void atomik_hw_set_enable(bool enable)
{
	struct atomik_hw_device *hw = atomik_hw_dev;

	if (!hw)
		return;

	mutex_lock(&hw->io_lock);
	hw_write32(hw, ATOMIK_REG_CONFIG, enable ? 1 : 0);
	mutex_unlock(&hw->io_lock);
}

bool atomik_hw_is_enabled(void)
{
	struct atomik_hw_device *hw = atomik_hw_dev;

	if (!hw)
		return false;

	return hw_read32(hw, ATOMIK_REG_CONFIG) & 1;
}

/* --- Platform driver --- */

static int atomik_hw_probe(struct platform_device *pdev)
{
	struct atomik_hw_device *hw;
	struct resource *res;
	u32 status;

	hw = devm_kzalloc(&pdev->dev, sizeof(*hw), GFP_KERNEL);
	if (!hw)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hw->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(hw->regs))
		return PTR_ERR(hw->regs);

	mutex_init(&hw->io_lock);
	bitmap_zero(hw->ctx_bitmap, ATOMIK_HW_MAX_CONTEXTS);
	hw->ctx_allocated = 0;

	/* Read STATUS register to detect hardware configuration */
	status = hw_read32(hw, ATOMIK_REG_STATUS);
	hw->n_banks = ATOMIK_STATUS_N_BANKS(status);
	hw->hw_version = ATOMIK_STATUS_VERSION(status);

	if (hw->n_banks == 0) {
		dev_warn(&pdev->dev,
			 "ATOMiK hardware STATUS=0x%08x, n_banks=0 — invalid\n",
			 status);
		return -ENODEV;
	}

	/* Enable the hardware engine */
	hw_write32(hw, ATOMIK_REG_CONFIG, 1);

	/* Publish global device pointer (chardev uses this) */
	atomik_hw_dev = hw;

	platform_set_drvdata(pdev, hw);

	dev_info(&pdev->dev,
		 "ATOMiK hardware v%d detected: %d bank(s), %d contexts\n",
		 hw->hw_version, hw->n_banks, ATOMIK_HW_MAX_CONTEXTS);

	return 0;
}

static void atomik_hw_remove_fn(struct platform_device *pdev)
{
	struct atomik_hw_device *hw = platform_get_drvdata(pdev);

	/* Disable hardware before removing */
	hw_write32(hw, ATOMIK_REG_CONFIG, 0);

	/* Clear global pointer so chardev falls back to software */
	atomik_hw_dev = NULL;
}

static const struct of_device_id atomik_hw_of_match[] = {
	{ .compatible = "atomik,delta-engine" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, atomik_hw_of_match);

static struct platform_driver atomik_hw_driver = {
	.probe  = atomik_hw_probe,
	.remove = atomik_hw_remove_fn,
	.driver = {
		.name = "atomik-hw",
		.of_match_table = atomik_hw_of_match,
	},
};

int atomik_hw_driver_register(void)
{
	return platform_driver_register(&atomik_hw_driver);
}

void atomik_hw_driver_unregister(void)
{
	platform_driver_unregister(&atomik_hw_driver);
}
