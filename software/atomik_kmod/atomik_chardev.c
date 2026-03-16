// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_chardev.c — /dev/atomik character device
 *
 * Provides ioctl interface for ATOMiK operations.
 * Each open fd gets its own set of context tables.
 *
 * Thread safety: fctx->lock is held for the entire duration of any
 * operation that touches a table pointer, preventing use-after-free
 * from concurrent DESTROY_TABLE calls.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#include "atomik_core_kern.h"
#include "atomik_hw.h"
#include "atomik_stats.h"
#include "atomik_trace.h"
#include "include/uapi/atomik.h"

/* Forward declarations from atomik_fingerprint.c */
struct atomik_fp_subsys;
extern struct atomik_fp_subsys *atomik_fp_create(void);
extern void atomik_fp_destroy(struct atomik_fp_subsys *fps);
extern int atomik_fp_register(struct atomik_fp_subsys *fps,
			      unsigned long user_addr, size_t length,
			      u32 *out_id);
extern int atomik_fp_check(struct atomik_fp_subsys *fps, u32 fp_id,
			   u32 *changed, u64 *delta,
			   u64 *pages_checked, u64 *pages_changed);
extern int atomik_fp_unregister(struct atomik_fp_subsys *fps, u32 fp_id);

/* Forward declaration from atomik_license.c */
extern bool atomik_license_is_active(void);

/* Per-open-file state */
struct atomik_file_ctx {
	struct idr table_idr;
	struct mutex lock;
	struct atomik_fp_subsys *fps;
};

/* --- File operations --- */

static int atomik_open(struct inode *inode, struct file *filp)
{
	struct atomik_file_ctx *fctx;

	fctx = kzalloc(sizeof(*fctx), GFP_KERNEL);
	if (!fctx)
		return -ENOMEM;

	idr_init(&fctx->table_idr);
	mutex_init(&fctx->lock);
	fctx->fps = atomik_fp_create();
	if (!fctx->fps) {
		kfree(fctx);
		return -ENOMEM;
	}

	filp->private_data = fctx;
	return 0;
}

static int atomik_release(struct inode *inode, struct file *filp)
{
	struct atomik_file_ctx *fctx = filp->private_data;
	struct atomik_table *t;
	int id;

	/* Free all tables (release hw contexts first) */
	idr_for_each_entry(&fctx->table_idr, t, id) {
		if (t->hw_base >= 0)
			atomik_hw_free_contexts(t->hw_base, t->num_contexts);
		atomik_table_free(t);
		kfree(t);
	}
	idr_destroy(&fctx->table_idr);

	/* Free fingerprint subsystem */
	atomik_fp_destroy(fctx->fps);

	kfree(fctx);
	return 0;
}

/* --- ioctl handlers --- */

static long ioctl_create_table(struct atomik_file_ctx *fctx, unsigned long arg)
{
	struct atomik_create_table_args a;
	struct atomik_table *t;
	int ret;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;

	if (a.num_contexts == 0 || a.num_contexts > ATOMIK_MAX_CONTEXTS)
		return -EINVAL;

	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (!t)
		return -ENOMEM;

	ret = atomik_table_init(t, a.num_contexts);
	if (ret) {
		kfree(t);
		return ret;
	}

	/* Try hardware backing if available and table fits */
	if (atomik_hw_available() && a.num_contexts <= ATOMIK_HW_MAX_CONTEXTS) {
		int hw_base = atomik_hw_alloc_contexts(a.num_contexts);
		if (hw_base >= 0)
			t->hw_base = hw_base;
		/* Fall through to software if hw alloc fails */
	}

	mutex_lock(&fctx->lock);
	ret = idr_alloc(&fctx->table_idr, t, 1, ATOMIK_MAX_TABLES + 1,
			GFP_KERNEL);
	mutex_unlock(&fctx->lock);

	if (ret < 0) {
		atomik_table_free(t);
		kfree(t);
		return ret;
	}

	a.table_id = ret;
	if (copy_to_user((void __user *)arg, &a, sizeof(a))) {
		/* Rollback: remove from IDR and free */
		mutex_lock(&fctx->lock);
		idr_remove(&fctx->table_idr, a.table_id);
		mutex_unlock(&fctx->lock);
		atomik_table_free(t);
		kfree(t);
		return -EFAULT;
	}

	return 0;
}

static long ioctl_destroy_table(struct atomik_file_ctx *fctx, unsigned long arg)
{
	struct atomik_table *t;
	u32 table_id;

	if (get_user(table_id, (u32 __user *)arg))
		return -EFAULT;

	mutex_lock(&fctx->lock);
	t = idr_remove(&fctx->table_idr, table_id);
	mutex_unlock(&fctx->lock);

	if (!t)
		return -ENOENT;

	/* Release hardware context slots if hw-backed */
	if (t->hw_base >= 0)
		atomik_hw_free_contexts(t->hw_base, t->num_contexts);

	atomik_table_free(t);
	kfree(t);
	return 0;
}

/*
 * find_table_locked — Look up a table while holding fctx->lock.
 * Caller MUST hold fctx->lock and release it after finishing
 * with the returned pointer.
 */
static struct atomik_table *find_table_locked(struct atomik_file_ctx *fctx,
					      u32 id)
{
	return idr_find(&fctx->table_idr, id);
}

static long ioctl_load(struct atomik_file_ctx *fctx, unsigned long arg)
{
	struct atomik_load_args a;
	struct atomik_table *t;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;

	mutex_lock(&fctx->lock);
	t = find_table_locked(fctx, a.table_id);
	if (!t) {
		mutex_unlock(&fctx->lock);
		return -ENOENT;
	}

	if (a.addr >= t->num_contexts) {
		mutex_unlock(&fctx->lock);
		return -EINVAL;
	}

	if (t->hw_base >= 0)
		atomik_hw_load(t->hw_base + a.addr, a.initial_state);
	else
		atomik_table_load(t, a.addr, a.initial_state);
	mutex_unlock(&fctx->lock);

	atomik_stat_inc(ops_load);
	trace_atomik_op(ATOMIK_OP_LOAD, a.table_id, a.addr, a.initial_state);
	return 0;
}

static long ioctl_accum(struct atomik_file_ctx *fctx, unsigned long arg)
{
	struct atomik_accum_args a;
	struct atomik_table *t;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;

	mutex_lock(&fctx->lock);
	t = find_table_locked(fctx, a.table_id);
	if (!t) {
		mutex_unlock(&fctx->lock);
		return -ENOENT;
	}

	if (a.addr >= t->num_contexts) {
		mutex_unlock(&fctx->lock);
		return -EINVAL;
	}

	if (t->hw_base >= 0)
		atomik_hw_accum(t->hw_base + a.addr, a.delta);
	else
		atomik_table_accum(t, a.addr, a.delta);
	mutex_unlock(&fctx->lock);

	atomik_stat_inc(ops_accum);
	trace_atomik_op(ATOMIK_OP_ACCUM, a.table_id, a.addr, a.delta);
	return 0;
}

static long ioctl_read(struct atomik_file_ctx *fctx, unsigned long arg)
{
	struct atomik_read_args a;
	struct atomik_table *t;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;

	mutex_lock(&fctx->lock);
	t = find_table_locked(fctx, a.table_id);
	if (!t) {
		mutex_unlock(&fctx->lock);
		return -ENOENT;
	}

	if (a.addr >= t->num_contexts) {
		mutex_unlock(&fctx->lock);
		return -EINVAL;
	}

	if (t->hw_base >= 0)
		a.state = atomik_hw_read(t->hw_base + a.addr);
	else
		a.state = atomik_table_read(t, a.addr);
	mutex_unlock(&fctx->lock);

	atomik_stat_inc(ops_read);
	trace_atomik_op(ATOMIK_OP_READ, a.table_id, a.addr, a.state);

	if (copy_to_user((void __user *)arg, &a, sizeof(a)))
		return -EFAULT;
	return 0;
}

static long ioctl_swap(struct atomik_file_ctx *fctx, unsigned long arg)
{
	struct atomik_swap_args a;
	struct atomik_table *t;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;

	mutex_lock(&fctx->lock);
	t = find_table_locked(fctx, a.table_id);
	if (!t) {
		mutex_unlock(&fctx->lock);
		return -ENOENT;
	}

	if (a.addr >= t->num_contexts) {
		mutex_unlock(&fctx->lock);
		return -EINVAL;
	}

	if (t->hw_base >= 0)
		a.old_state = atomik_hw_swap(t->hw_base + a.addr);
	else
		a.old_state = atomik_table_swap(t, a.addr);
	mutex_unlock(&fctx->lock);

	atomik_stat_inc(ops_swap);
	trace_atomik_op(ATOMIK_OP_SWAP, a.table_id, a.addr, a.old_state);

	if (copy_to_user((void __user *)arg, &a, sizeof(a)))
		return -EFAULT;
	return 0;
}

static long ioctl_batch(struct atomik_file_ctx *fctx, unsigned long arg)
{
	struct atomik_batch_args ba;
	struct atomik_batch_op *ops;
	struct atomik_table *t;
	unsigned int i;
	long ret = 0;

	if (copy_from_user(&ba, (void __user *)arg, sizeof(ba)))
		return -EFAULT;

	if (ba.num_ops == 0 || ba.num_ops > ATOMIK_MAX_BATCH_OPS)
		return -EINVAL;

	ops = kvmalloc_array(ba.num_ops, sizeof(*ops), GFP_KERNEL);
	if (!ops)
		return -ENOMEM;

	if (copy_from_user(ops, (void __user *)ba.ops_ptr,
			   ba.num_ops * sizeof(*ops))) {
		ret = -EFAULT;
		goto out;
	}

	mutex_lock(&fctx->lock);
	t = find_table_locked(fctx, ba.table_id);
	if (!t) {
		mutex_unlock(&fctx->lock);
		ret = -ENOENT;
		goto out;
	}

	for (i = 0; i < ba.num_ops; i++) {
		if (ops[i].addr >= t->num_contexts) {
			mutex_unlock(&fctx->lock);
			ret = -EINVAL;
			goto out;
		}

		switch (ops[i].opcode) {
		case ATOMIK_OP_LOAD:
			if (t->hw_base >= 0)
				atomik_hw_load(t->hw_base + ops[i].addr,
					       ops[i].value);
			else
				atomik_table_load(t, ops[i].addr,
						  ops[i].value);
			atomik_stat_inc(ops_load);
			break;
		case ATOMIK_OP_ACCUM:
			if (t->hw_base >= 0)
				atomik_hw_accum(t->hw_base + ops[i].addr,
						ops[i].value);
			else
				atomik_table_accum(t, ops[i].addr,
						   ops[i].value);
			atomik_stat_inc(ops_accum);
			break;
		case ATOMIK_OP_READ:
			if (t->hw_base >= 0)
				ops[i].value = atomik_hw_read(
					t->hw_base + ops[i].addr);
			else
				ops[i].value = atomik_table_read(
					t, ops[i].addr);
			atomik_stat_inc(ops_read);
			break;
		case ATOMIK_OP_SWAP:
			if (t->hw_base >= 0)
				ops[i].value = atomik_hw_swap(
					t->hw_base + ops[i].addr);
			else
				ops[i].value = atomik_table_swap(
					t, ops[i].addr);
			atomik_stat_inc(ops_swap);
			break;
		default:
			mutex_unlock(&fctx->lock);
			ret = -EINVAL;
			goto out;
		}
	}
	mutex_unlock(&fctx->lock);

	/* Copy results back (READ/SWAP write into ops[].value) */
	if (copy_to_user((void __user *)ba.ops_ptr, ops,
			 ba.num_ops * sizeof(*ops)))
		ret = -EFAULT;

out:
	kvfree(ops);
	return ret;
}

static long ioctl_fp_register(struct atomik_file_ctx *fctx, unsigned long arg)
{
	struct atomik_fp_register_args a;
	u32 fp_id;
	int ret;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;

	ret = atomik_fp_register(fctx->fps, a.user_addr, a.length, &fp_id);
	if (ret)
		return ret;

	a.fp_id = fp_id;
	if (copy_to_user((void __user *)arg, &a, sizeof(a))) {
		/* Rollback: unregister the region we just created */
		atomik_fp_unregister(fctx->fps, fp_id);
		return -EFAULT;
	}
	return 0;
}

static long ioctl_fp_check(struct atomik_file_ctx *fctx, unsigned long arg)
{
	struct atomik_fp_check_args a;
	int ret;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;

	ret = atomik_fp_check(fctx->fps, a.fp_id, &a.changed, &a.delta,
			      &a.pages_checked, &a.pages_changed);
	if (ret)
		return ret;

	if (copy_to_user((void __user *)arg, &a, sizeof(a)))
		return -EFAULT;
	return 0;
}

static long ioctl_fp_unregister(struct atomik_file_ctx *fctx, unsigned long arg)
{
	u32 fp_id;

	if (get_user(fp_id, (u32 __user *)arg))
		return -EFAULT;

	return atomik_fp_unregister(fctx->fps, fp_id);
}

static long ioctl_get_info(unsigned long arg)
{
	struct atomik_info info;
	struct atomik_stats agg;

	memset(&info, 0, sizeof(info));
	atomik_stats_aggregate(&agg);

	info.version = (ATOMIK_VERSION_MAJOR << 16) |
		       (ATOMIK_VERSION_MINOR << 8) |
		       ATOMIK_VERSION_PATCH;
	info.backend = atomik_hw_available() ? 1 : 0;
	info.hw_banks = atomik_hw_dev ? atomik_hw_dev->n_banks : 0;
	info.fp_enabled = 1;
	info.ops_total = agg.ops_load + agg.ops_accum +
			 agg.ops_read + agg.ops_swap;
	info.fp_checks = agg.fp_checks_total;
	info.fp_bytes_saved = agg.fp_bytes_skipped;

	if (copy_to_user((void __user *)arg, &info, sizeof(info)))
		return -EFAULT;
	return 0;
}

/* --- Expired-license passthrough handlers --- */

/*
 * When the license expires, ATOMiK degrades gracefully instead of
 * returning errors. Operations silently become no-ops so the system
 * transitions seamlessly back to unaccelerated behavior:
 *
 *   LOAD/ACCUM:  silently dropped (return 0)
 *   READ:        returns state=0 (as if context is empty)
 *   SWAP:        returns old_state=0 (as if nothing accumulated)
 *   BATCH:       all ops become no-ops, READ/SWAP return 0
 *   FP_REGISTER: silently succeeds with fp_id=0 (dummy)
 *   FP_CHECK:    returns changed=0 (as if nothing changed)
 *   FP_UNREGISTER: silently succeeds
 *   CREATE/DESTROY: work normally (cleanup must still function)
 *   GET_INFO:    works normally (user can see "expired" status)
 */

static long expired_read(unsigned long arg)
{
	struct atomik_read_args a;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;
	a.state = 0;
	if (copy_to_user((void __user *)arg, &a, sizeof(a)))
		return -EFAULT;
	return 0;
}

static long expired_swap(unsigned long arg)
{
	struct atomik_swap_args a;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;
	a.old_state = 0;
	if (copy_to_user((void __user *)arg, &a, sizeof(a)))
		return -EFAULT;
	return 0;
}

static long expired_batch(unsigned long arg)
{
	struct atomik_batch_args ba;
	struct atomik_batch_op *ops;
	unsigned int i;

	if (copy_from_user(&ba, (void __user *)arg, sizeof(ba)))
		return -EFAULT;

	if (ba.num_ops == 0 || ba.num_ops > ATOMIK_MAX_BATCH_OPS)
		return -EINVAL;

	ops = kvmalloc_array(ba.num_ops, sizeof(*ops), GFP_KERNEL);
	if (!ops)
		return -ENOMEM;

	if (copy_from_user(ops, (void __user *)ba.ops_ptr,
			   ba.num_ops * sizeof(*ops))) {
		kvfree(ops);
		return -EFAULT;
	}

	/* Zero out all READ/SWAP results */
	for (i = 0; i < ba.num_ops; i++) {
		if (ops[i].opcode == ATOMIK_OP_READ ||
		    ops[i].opcode == ATOMIK_OP_SWAP)
			ops[i].value = 0;
	}

	if (copy_to_user((void __user *)ba.ops_ptr, ops,
			 ba.num_ops * sizeof(*ops))) {
		kvfree(ops);
		return -EFAULT;
	}

	kvfree(ops);
	return 0;
}

static long expired_fp_register(unsigned long arg)
{
	struct atomik_fp_register_args a;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;
	a.fp_id = 0; /* dummy ID */
	if (copy_to_user((void __user *)arg, &a, sizeof(a)))
		return -EFAULT;
	return 0;
}

static long expired_fp_check(unsigned long arg)
{
	struct atomik_fp_check_args a;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;
	a.changed = 0;
	a.delta = 0;
	a.pages_checked = 0;
	a.pages_changed = 0;
	if (copy_to_user((void __user *)arg, &a, sizeof(a)))
		return -EFAULT;
	return 0;
}

/* --- Main ioctl dispatch --- */

static bool expired_notified;

static long atomik_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct atomik_file_ctx *fctx = filp->private_data;

	if (!atomik_license_is_active()) {
		/* One-time notification on first expired call */
		if (!expired_notified) {
			expired_notified = true;
			pr_notice("atomik: trial expired — "
				  "ATOMiK is now disabled. "
				  "Visit https://atomik.tech to purchase a license.\n");
		}

		/*
		 * Graceful degradation: operations succeed but do nothing.
		 * The system seamlessly falls back to unaccelerated behavior.
		 */
		switch (cmd) {
		case ATOMIK_IOC_LOAD:
		case ATOMIK_IOC_ACCUM:
		case ATOMIK_IOC_FP_UNREGISTER:
			return 0; /* silently dropped */
		case ATOMIK_IOC_READ:
			return expired_read(arg);
		case ATOMIK_IOC_SWAP:
			return expired_swap(arg);
		case ATOMIK_IOC_BATCH:
			return expired_batch(arg);
		case ATOMIK_IOC_FP_REGISTER:
			return expired_fp_register(arg);
		case ATOMIK_IOC_FP_CHECK:
			return expired_fp_check(arg);
		case ATOMIK_IOC_CREATE_TABLE:
			return ioctl_create_table(fctx, arg);
		case ATOMIK_IOC_DESTROY_TABLE:
			return ioctl_destroy_table(fctx, arg);
		case ATOMIK_IOC_GET_INFO:
			return ioctl_get_info(arg);
		default:
			return -ENOTTY;
		}
	}

	switch (cmd) {
	case ATOMIK_IOC_CREATE_TABLE:
		return ioctl_create_table(fctx, arg);
	case ATOMIK_IOC_DESTROY_TABLE:
		return ioctl_destroy_table(fctx, arg);
	case ATOMIK_IOC_LOAD:
		return ioctl_load(fctx, arg);
	case ATOMIK_IOC_ACCUM:
		return ioctl_accum(fctx, arg);
	case ATOMIK_IOC_READ:
		return ioctl_read(fctx, arg);
	case ATOMIK_IOC_SWAP:
		return ioctl_swap(fctx, arg);
	case ATOMIK_IOC_BATCH:
		return ioctl_batch(fctx, arg);
	case ATOMIK_IOC_FP_REGISTER:
		return ioctl_fp_register(fctx, arg);
	case ATOMIK_IOC_FP_CHECK:
		return ioctl_fp_check(fctx, arg);
	case ATOMIK_IOC_FP_UNREGISTER:
		return ioctl_fp_unregister(fctx, arg);
	case ATOMIK_IOC_GET_INFO:
		return ioctl_get_info(arg);
	default:
		return -ENOTTY;
	}
}

const struct file_operations atomik_fops = {
	.owner          = THIS_MODULE,
	.open           = atomik_open,
	.release        = atomik_release,
	.unlocked_ioctl = atomik_ioctl,
	.compat_ioctl   = compat_ptr_ioctl,
};
