/* SPDX-License-Identifier: GPL-2.0 */
/*
 * atomik_cow.h — Transparent COW optimization interface
 *
 * Intercepts copy-on-write page faults via kretprobe on wp_page_copy().
 * When a page is about to be copied, fingerprints it first. If the page
 * content hasn't actually changed (written with identical data), the
 * copy is unnecessary — ATOMiK detects this and tracks the savings.
 *
 * v0.3 is observation-only: we fingerprint and count, but don't yet
 * suppress the copy (that requires deeper mm integration). The stats
 * prove the value proposition — customers see exactly how much memory
 * and CPU their system wastes on redundant copies.
 *
 * v0.3.1+ will add actual copy suppression via PTE manipulation.
 */

#ifndef ATOMIK_COW_H
#define ATOMIK_COW_H

#include <linux/types.h>

/* COW subsystem lifecycle */
int atomik_cow_init(void);
void atomik_cow_exit(void);

/* Runtime enable/disable (safe to call from sysfs) */
void atomik_cow_set_enabled(bool enabled);
bool atomik_cow_is_enabled(void);

/* Per-process stats export (for sysfs cow_top attribute) */
int atomik_cow_top(char *buf, int buflen, int max_entries);

#endif /* ATOMIK_COW_H */
