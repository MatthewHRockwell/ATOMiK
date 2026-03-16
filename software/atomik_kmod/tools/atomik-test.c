// SPDX-License-Identifier: GPL-2.0
/*
 * atomik-test — Smoke test for /dev/atomik ioctl interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>

/* Include the UAPI header types inline (avoids kernel header dependency) */
#define ATOMIK_MAGIC 'A'

struct atomik_create_table_args {
	uint32_t num_contexts;
	uint32_t table_id;
};

struct atomik_load_args {
	uint32_t table_id;
	uint32_t addr;
	uint64_t initial_state;
};

struct atomik_accum_args {
	uint32_t table_id;
	uint32_t addr;
	uint64_t delta;
};

struct atomik_read_args {
	uint32_t table_id;
	uint32_t addr;
	uint64_t state;
};

struct atomik_swap_args {
	uint32_t table_id;
	uint32_t addr;
	uint64_t new_reference;
	uint64_t old_state;
};

struct atomik_info {
	uint32_t version;
	uint32_t backend;
	uint32_t hw_banks;
	uint32_t fp_enabled;
	uint64_t ops_total;
	uint64_t fp_checks;
	uint64_t fp_bytes_saved;
};

#define ATOMIK_IOC_CREATE_TABLE   _IOWR(ATOMIK_MAGIC, 0x01, struct atomik_create_table_args)
#define ATOMIK_IOC_DESTROY_TABLE  _IOW(ATOMIK_MAGIC, 0x02, uint32_t)
#define ATOMIK_IOC_LOAD           _IOW(ATOMIK_MAGIC, 0x10, struct atomik_load_args)
#define ATOMIK_IOC_ACCUM          _IOW(ATOMIK_MAGIC, 0x11, struct atomik_accum_args)
#define ATOMIK_IOC_READ           _IOWR(ATOMIK_MAGIC, 0x12, struct atomik_read_args)
#define ATOMIK_IOC_SWAP           _IOWR(ATOMIK_MAGIC, 0x13, struct atomik_swap_args)
#define ATOMIK_IOC_GET_INFO       _IOR(ATOMIK_MAGIC, 0x40, struct atomik_info)

#define TEST(name, cond) do { \
	tests++; \
	if (cond) { passed++; printf("  PASS: %s\n", name); } \
	else { printf("  FAIL: %s\n", name); } \
} while (0)

int main(void)
{
	int fd, ret;
	int tests = 0, passed = 0;

	printf("ATOMiK Kernel Module Smoke Test\n");
	printf("===============================\n\n");

	fd = open("/dev/atomik", O_RDWR);
	if (fd < 0) {
		perror("open /dev/atomik");
		printf("\nModule not loaded? Try: sudo insmod atomik.ko\n");
		return 1;
	}
	TEST("open /dev/atomik", fd >= 0);

	/* T1: GET_INFO */
	{
		struct atomik_info info = {0};
		ret = ioctl(fd, ATOMIK_IOC_GET_INFO, &info);
		TEST("GET_INFO ioctl", ret == 0);
		TEST("version is 0.4.0", info.version == ((0 << 16) | (4 << 8) | 0));
		TEST("backend is software", info.backend == 0);
		printf("  Info: v%d.%d.%d, backend=%s, fp=%s\n",
		       (info.version >> 16) & 0xFF,
		       (info.version >> 8) & 0xFF,
		       info.version & 0xFF,
		       info.backend ? "hardware" : "software",
		       info.fp_enabled ? "enabled" : "disabled");
	}

	/* T2: CREATE_TABLE */
	struct atomik_create_table_args ct = { .num_contexts = 256 };
	ret = ioctl(fd, ATOMIK_IOC_CREATE_TABLE, &ct);
	TEST("CREATE_TABLE (256 contexts)", ret == 0);
	TEST("table_id assigned", ct.table_id > 0);
	printf("  Allocated table_id=%u\n", ct.table_id);

	/* T3: LOAD */
	{
		struct atomik_load_args la = {
			.table_id = ct.table_id,
			.addr = 0,
			.initial_state = 0xDEADBEEF,
		};
		ret = ioctl(fd, ATOMIK_IOC_LOAD, &la);
		TEST("LOAD addr=0 state=0xDEADBEEF", ret == 0);
	}

	/* T4: READ (should return initial state) */
	{
		struct atomik_read_args ra = {
			.table_id = ct.table_id,
			.addr = 0,
		};
		ret = ioctl(fd, ATOMIK_IOC_READ, &ra);
		TEST("READ addr=0", ret == 0);
		TEST("READ returns 0xDEADBEEF", ra.state == 0xDEADBEEF);
		printf("  Read state=0x%llx\n", (unsigned long long)ra.state);
	}

	/* T5: ACCUM (XOR a delta) */
	{
		struct atomik_accum_args aa = {
			.table_id = ct.table_id,
			.addr = 0,
			.delta = 0xFF,
		};
		ret = ioctl(fd, ATOMIK_IOC_ACCUM, &aa);
		TEST("ACCUM addr=0 delta=0xFF", ret == 0);
	}

	/* T6: READ after ACCUM */
	{
		struct atomik_read_args ra = {
			.table_id = ct.table_id,
			.addr = 0,
		};
		ret = ioctl(fd, ATOMIK_IOC_READ, &ra);
		TEST("READ after ACCUM", ret == 0);
		TEST("state = 0xDEADBEEF ^ 0xFF = 0xDEADBE10",
		     ra.state == (0xDEADBEEF ^ 0xFF));
		printf("  Read state=0x%llx (expected 0x%llx)\n",
		       (unsigned long long)ra.state,
		       (unsigned long long)(0xDEADBEEF ^ 0xFF));
	}

	/* T7: SWAP */
	{
		struct atomik_swap_args sa = {
			.table_id = ct.table_id,
			.addr = 0,
		};
		ret = ioctl(fd, ATOMIK_IOC_SWAP, &sa);
		TEST("SWAP addr=0", ret == 0);
		TEST("SWAP returns old state",
		     sa.old_state == (0xDEADBEEF ^ 0xFF));
		printf("  Swap old_state=0x%llx\n",
		       (unsigned long long)sa.old_state);
	}

	/* T8: READ after SWAP (should equal swapped state) */
	{
		struct atomik_read_args ra = {
			.table_id = ct.table_id,
			.addr = 0,
		};
		ret = ioctl(fd, ATOMIK_IOC_READ, &ra);
		TEST("READ after SWAP", ret == 0);
		TEST("state after swap = old_state (acc reset)",
		     ra.state == (0xDEADBEEF ^ 0xFF));
	}

	/* T9: DESTROY_TABLE */
	{
		uint32_t tid = ct.table_id;
		ret = ioctl(fd, ATOMIK_IOC_DESTROY_TABLE, &tid);
		TEST("DESTROY_TABLE", ret == 0);
	}

	/* T10: READ from destroyed table should fail */
	{
		struct atomik_read_args ra = {
			.table_id = ct.table_id,
			.addr = 0,
		};
		ret = ioctl(fd, ATOMIK_IOC_READ, &ra);
		TEST("READ from destroyed table fails", ret != 0);
	}

	close(fd);

	printf("\n%d/%d tests passed\n", passed, tests);
	return (passed == tests) ? 0 : 1;
}
