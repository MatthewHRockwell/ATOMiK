// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_fingerprint.c — Page-level XOR fingerprinting subsystem
 *
 * Detects page-level changes via XOR-reduce without memcmp.
 * Each registered memory region gets per-page fingerprints.
 * Checking for changes is O(1) per page (single XOR comparison).
 *
 * Thread safety: fps->lock is held for the full duration of
 * fp_check to prevent concurrent fp_unregister from freeing
 * the region while pages are being scanned.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/idr.h>
#include <linux/mutex.h>

#include "atomik_core_kern.h"
#include "atomik_stats.h"
#include "include/uapi/atomik.h"

struct atomik_fp_region {
	u32 id;
	struct page **pages;
	unsigned int num_pages;
	u64 *page_fps;           /* per-page XOR fingerprints */
	u64 region_fp;           /* XOR of all page_fps */
	unsigned long user_addr;
	size_t length;
};

struct atomik_fp_subsys {
	struct idr region_idr;
	struct mutex lock;
	unsigned int num_regions;
};

struct atomik_fp_subsys *atomik_fp_create(void)
{
	struct atomik_fp_subsys *fps;

	fps = kzalloc(sizeof(*fps), GFP_KERNEL);
	if (!fps)
		return NULL;
	idr_init(&fps->region_idr);
	mutex_init(&fps->lock);
	return fps;
}

static void atomik_fp_free_region(struct atomik_fp_region *r)
{
	if (r->pages) {
		unpin_user_pages(r->pages, r->num_pages);
		kvfree(r->pages);
	}
	kvfree(r->page_fps);
	kfree(r);
}

void atomik_fp_destroy(struct atomik_fp_subsys *fps)
{
	struct atomik_fp_region *r;
	int id;

	if (!fps)
		return;

	idr_for_each_entry(&fps->region_idr, r, id)
		atomik_fp_free_region(r);
	idr_destroy(&fps->region_idr);
	kfree(fps);
}

int atomik_fp_register(struct atomik_fp_subsys *fps,
		       unsigned long user_addr, size_t length, u32 *out_id)
{
	struct atomik_fp_region *r;
	unsigned long end_addr;
	unsigned int num_pages;
	u64 region_fp = 0;
	unsigned int i;
	int ret;

	if (!length || length > (1UL << 30))  /* max 1 GB */
		return -EINVAL;

	/* Check for integer overflow in user_addr + length */
	if (check_add_overflow(user_addr, length, &end_addr))
		return -EOVERFLOW;

	num_pages = DIV_ROUND_UP(end_addr, PAGE_SIZE) -
		    (user_addr / PAGE_SIZE);

	r = kzalloc(sizeof(*r), GFP_KERNEL);
	if (!r)
		return -ENOMEM;

	r->pages = kvmalloc_array(num_pages, sizeof(struct page *), GFP_KERNEL);
	r->page_fps = kvmalloc_array(num_pages, sizeof(u64),
				     GFP_KERNEL | __GFP_ZERO);
	if (!r->pages || !r->page_fps) {
		ret = -ENOMEM;
		goto err;
	}

	/* Pin user pages */
	ret = pin_user_pages_fast(user_addr & PAGE_MASK, num_pages,
				  0, r->pages);
	if (ret < 0)
		goto err;
	if ((unsigned int)ret != num_pages) {
		unpin_user_pages(r->pages, ret);
		ret = -EFAULT;
		goto err;
	}

	r->num_pages = num_pages;
	r->user_addr = user_addr;
	r->length = length;

	/* Compute initial fingerprints */
	for (i = 0; i < num_pages; i++) {
		u64 *data = kmap_local_page(r->pages[i]);
		r->page_fps[i] = atomik_fp_reduce_page(data);
		region_fp ^= r->page_fps[i];
		kunmap_local(data);
	}
	r->region_fp = region_fp;

	atomik_stat_add(fp_bytes_fingerprinted,
			(u64)num_pages * PAGE_SIZE);

	mutex_lock(&fps->lock);
	ret = idr_alloc(&fps->region_idr, r, 1, ATOMIK_MAX_FP_REGIONS + 1,
			GFP_KERNEL);
	if (ret < 0) {
		mutex_unlock(&fps->lock);
		goto err_unpin;
	}
	r->id = ret;
	fps->num_regions++;
	mutex_unlock(&fps->lock);

	*out_id = r->id;
	return 0;

err_unpin:
	unpin_user_pages(r->pages, num_pages);
err:
	kvfree(r->pages);
	kvfree(r->page_fps);
	kfree(r);
	return ret;
}

int atomik_fp_check(struct atomik_fp_subsys *fps, u32 fp_id,
		    u32 *changed, u64 *delta,
		    u64 *pages_checked, u64 *pages_changed)
{
	struct atomik_fp_region *r;
	u64 new_region_fp = 0;
	u64 pg_changed = 0;
	unsigned int i;

	/* Hold lock for full scan to prevent concurrent unregister */
	mutex_lock(&fps->lock);
	r = idr_find(&fps->region_idr, fp_id);
	if (!r) {
		mutex_unlock(&fps->lock);
		return -ENOENT;
	}

	for (i = 0; i < r->num_pages; i++) {
		u64 *data = kmap_local_page(r->pages[i]);
		u64 new_fp = atomik_fp_reduce_page(data);
		kunmap_local(data);

		if (new_fp != r->page_fps[i])
			pg_changed++;
		new_region_fp ^= new_fp;
	}
	mutex_unlock(&fps->lock);

	atomik_stat_inc(fp_checks_total);
	atomik_stat_add(fp_bytes_fingerprinted,
			(u64)r->num_pages * PAGE_SIZE);

	*delta = r->region_fp ^ new_region_fp;
	*changed = (*delta != 0) ? 1 : 0;
	*pages_checked = r->num_pages;
	*pages_changed = pg_changed;

	if (!*changed) {
		atomik_stat_inc(fp_checks_unchanged);
		atomik_stat_add(fp_bytes_skipped,
				(u64)r->num_pages * PAGE_SIZE);
	}

	return 0;
}

int atomik_fp_unregister(struct atomik_fp_subsys *fps, u32 fp_id)
{
	struct atomik_fp_region *r;

	mutex_lock(&fps->lock);
	r = idr_remove(&fps->region_idr, fp_id);
	if (r)
		fps->num_regions--;
	mutex_unlock(&fps->lock);

	if (!r)
		return -ENOENT;

	atomik_fp_free_region(r);
	return 0;
}
