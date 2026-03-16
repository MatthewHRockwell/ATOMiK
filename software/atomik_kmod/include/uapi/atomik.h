/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * ATOMiK Delta-State Accelerator — UAPI Header
 *
 * Shared between kernel module and userspace tools.
 * Include this header in userspace programs that talk to /dev/atomik.
 */

#ifndef _UAPI_ATOMIK_H
#define _UAPI_ATOMIK_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define ATOMIK_VERSION_MAJOR  0
#define ATOMIK_VERSION_MINOR  4
#define ATOMIK_VERSION_PATCH  0

#define ATOMIK_MAGIC 'A'

/* --- Argument structs --- */

struct atomik_create_table_args {
	__u32 num_contexts;     /* in: 1–65536 */
	__u32 table_id;         /* out: assigned ID */
};

struct atomik_load_args {
	__u32 table_id;
	__u32 addr;
	__u64 initial_state;
};

struct atomik_accum_args {
	__u32 table_id;
	__u32 addr;
	__u64 delta;
};

struct atomik_read_args {
	__u32 table_id;
	__u32 addr;
	__u64 state;            /* out: reconstructed state */
};

struct atomik_swap_args {
	__u32 table_id;
	__u32 addr;
	__u64 new_reference;    /* in: 0 = use current state as new ref */
	__u64 old_state;        /* out: state before swap */
};

struct atomik_fp_register_args {
	__u64 user_addr;        /* in: userspace virtual address */
	__u64 length;           /* in: bytes */
	__u32 fp_id;            /* out: assigned fingerprint ID */
	__u32 _pad;
};

struct atomik_fp_check_args {
	__u32 fp_id;            /* in */
	__u32 changed;          /* out: 0=unchanged, 1=changed */
	__u64 delta;            /* out: XOR delta fingerprint */
	__u64 pages_checked;    /* out: number of pages checked */
	__u64 pages_changed;    /* out: number of pages with changes */
};

struct atomik_info {
	__u32 version;          /* (major << 16) | (minor << 8) | patch */
	__u32 backend;          /* 0=software, 1=hardware */
	__u32 hw_banks;         /* hardware banks (0 if sw) */
	__u32 fp_enabled;       /* fingerprinting active */
	__u64 ops_total;        /* total operations since load */
	__u64 fp_checks;        /* total fingerprint checks */
	__u64 fp_bytes_saved;   /* bytes of redundant copies avoided */
};

struct atomik_batch_op {
	__u8  opcode;           /* ATOMIK_OP_* */
	__u8  _pad[3];
	__u32 addr;
	__u64 value;            /* delta for ACCUM, state for LOAD, result for READ */
};

#define ATOMIK_OP_LOAD   0
#define ATOMIK_OP_ACCUM  1
#define ATOMIK_OP_READ   2
#define ATOMIK_OP_SWAP   3

struct atomik_batch_args {
	__u32 table_id;
	__u32 num_ops;          /* max 4096 */
	__u64 ops_ptr;          /* pointer to struct atomik_batch_op[] */
};

/* --- ioctl commands --- */

/* Context table management */
#define ATOMIK_IOC_CREATE_TABLE   _IOWR(ATOMIK_MAGIC, 0x01, struct atomik_create_table_args)
#define ATOMIK_IOC_DESTROY_TABLE  _IOW(ATOMIK_MAGIC, 0x02, __u32)

/* Core operations */
#define ATOMIK_IOC_LOAD           _IOW(ATOMIK_MAGIC, 0x10, struct atomik_load_args)
#define ATOMIK_IOC_ACCUM          _IOW(ATOMIK_MAGIC, 0x11, struct atomik_accum_args)
#define ATOMIK_IOC_READ           _IOWR(ATOMIK_MAGIC, 0x12, struct atomik_read_args)
#define ATOMIK_IOC_SWAP           _IOWR(ATOMIK_MAGIC, 0x13, struct atomik_swap_args)

/* Fingerprinting */
#define ATOMIK_IOC_FP_REGISTER    _IOWR(ATOMIK_MAGIC, 0x20, struct atomik_fp_register_args)
#define ATOMIK_IOC_FP_CHECK       _IOWR(ATOMIK_MAGIC, 0x21, struct atomik_fp_check_args)
#define ATOMIK_IOC_FP_UNREGISTER  _IOW(ATOMIK_MAGIC, 0x22, __u32)

/* Batch */
#define ATOMIK_IOC_BATCH          _IOW(ATOMIK_MAGIC, 0x30, struct atomik_batch_args)

/* Info */
#define ATOMIK_IOC_GET_INFO       _IOR(ATOMIK_MAGIC, 0x40, struct atomik_info)

/* Max limits */
#define ATOMIK_MAX_CONTEXTS       65536
#define ATOMIK_MAX_TABLES         256
#define ATOMIK_MAX_FP_REGIONS     1024
#define ATOMIK_MAX_BATCH_OPS      4096

#endif /* _UAPI_ATOMIK_H */
