// SPDX-License-Identifier: GPL-2.0
/*
 * atomik-bench.c — ATOMiK vs naive benchmark with system metrics
 *
 * Runs side-by-side comparisons:
 *   1. Change detection: ATOMiK fingerprint vs memcmp
 *   2. State accumulation: ATOMiK XOR vs read-modify-write
 *   3. Batch throughput: ATOMiK batch ioctl vs individual ops
 *
 * Reports CPU temperature, frequency, and memory before/during/after.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>

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

struct atomik_batch_op {
	uint8_t  opcode;
	uint8_t  _pad[3];
	uint32_t addr;
	uint64_t value;
};

struct atomik_batch_args {
	uint32_t table_id;
	uint32_t num_ops;
	uint64_t ops_ptr;
};

struct atomik_fp_register_args {
	uint64_t user_addr;
	uint64_t length;
	uint32_t fp_id;
	uint32_t _pad;
};

struct atomik_fp_check_args {
	uint32_t fp_id;
	uint32_t changed;
	uint64_t delta;
	uint64_t pages_checked;
	uint64_t pages_changed;
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
#define ATOMIK_IOC_BATCH          _IOW(ATOMIK_MAGIC, 0x30, struct atomik_batch_args)
#define ATOMIK_IOC_FP_REGISTER    _IOWR(ATOMIK_MAGIC, 0x20, struct atomik_fp_register_args)
#define ATOMIK_IOC_FP_CHECK       _IOWR(ATOMIK_MAGIC, 0x21, struct atomik_fp_check_args)
#define ATOMIK_IOC_FP_UNREGISTER  _IOW(ATOMIK_MAGIC, 0x22, uint32_t)
#define ATOMIK_IOC_GET_INFO       _IOR(ATOMIK_MAGIC, 0x40, struct atomik_info)

/* --- System Metrics --- */

static int read_file_int(const char *path)
{
	FILE *f = fopen(path, "r");
	int val = -1;

	if (f) {
		if (fscanf(f, "%d", &val) != 1)
			val = -1;
		fclose(f);
	}
	return val;
}

static int get_cpu_temp(void)
{
	return read_file_int("/sys/class/hwmon/hwmon3/temp1_input");
}

static void print_metrics(const char *label)
{
	int cpu_temp = get_cpu_temp();
	int gpu_temp = read_file_int("/sys/class/hwmon/hwmon4/temp1_input");
	int cpu_freq = read_file_int(
		"/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");

	printf("  [%s] CPU: %d.%d°C  GPU: %d.%d°C  Freq: %d MHz\n",
	       label,
	       cpu_temp / 1000, (cpu_temp % 1000) / 100,
	       gpu_temp / 1000, (gpu_temp % 1000) / 100,
	       cpu_freq / 1000);
}

static void cooldown(const char *next_label, int target_temp)
{
	int temp, waited = 0;

	printf("\n  Cooling down");
	fflush(stdout);

	while (1) {
		temp = get_cpu_temp();
		if (temp <= target_temp && waited >= 5)
			break;
		if (waited >= 60) {
			printf(" (timeout)");
			break;
		}
		printf(".");
		fflush(stdout);
		sleep(1);
		waited++;
	}
	printf(" %d.%d°C\n", temp / 1000, (temp % 1000) / 100);

	if (next_label)
		printf("\n  Ready for: %s\n", next_label);
}

/* --- Timing --- */

static double time_sec(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* --- Benchmark 1: Change Detection --- */

static void bench_change_detection(int fd)
{
	const size_t region_size = 4 * 1024 * 1024; /* 4 MB */
	const int iterations = 500;
	unsigned char *buf;
	double t0, t1, naive_time, atomik_time;
	int i;

	printf("\n--- Benchmark 1: Change Detection (4 MB region, %d checks) ---\n",
	       iterations);

	buf = mmap(NULL, region_size, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (buf == MAP_FAILED) {
		perror("mmap");
		return;
	}
	memset(buf, 0xAA, region_size);

	/* Naive: full memcmp against a copy */
	unsigned char *copy = malloc(region_size);

	if (!copy) {
		munmap(buf, region_size);
		return;
	}
	memcpy(copy, buf, region_size);

	print_metrics("before");

	t0 = time_sec();
	for (i = 0; i < iterations; i++) {
		volatile int changed = (memcmp(buf, copy, region_size) != 0);
		(void)changed;
	}
	t1 = time_sec();
	naive_time = t1 - t0;

	print_metrics("after memcmp");

	/* ATOMiK: fingerprint register + check */
	struct atomik_fp_register_args reg = {
		.user_addr = (uint64_t)(uintptr_t)buf,
		.length = region_size,
	};
	int ret = ioctl(fd, ATOMIK_IOC_FP_REGISTER, &reg);

	if (ret < 0) {
		printf("  FP_REGISTER failed: %s\n", strerror(errno));
		free(copy);
		munmap(buf, region_size);
		return;
	}

	t0 = time_sec();
	for (i = 0; i < iterations; i++) {
		struct atomik_fp_check_args chk = { .fp_id = reg.fp_id };
		ioctl(fd, ATOMIK_IOC_FP_CHECK, &chk);
	}
	t1 = time_sec();
	atomik_time = t1 - t0;

	print_metrics("after fp_check");

	uint32_t dummy_id = reg.fp_id;
	ioctl(fd, ATOMIK_IOC_FP_UNREGISTER, &dummy_id);

	printf("\n  memcmp:     %.3f sec  (%.1f MB/s)\n",
	       naive_time,
	       (double)region_size * iterations / naive_time / 1e6);
	printf("  ATOMiK fp:  %.3f sec  (%.1f MB/s)\n",
	       atomik_time,
	       (double)region_size * iterations / atomik_time / 1e6);

	if (atomik_time < naive_time)
		printf("  ATOMiK is %.1fx faster\n",
		       naive_time / atomik_time);
	else
		printf("  memcmp is %.1fx faster (expected for sw backend)\n",
		       atomik_time / naive_time);

	free(copy);
	munmap(buf, region_size);
}

/* --- Benchmark 2: State Accumulation Throughput --- */

static void bench_accumulation(int fd)
{
	const int num_contexts = 4096;
	const int ops_per_ctx = 100;
	const int total_ops = num_contexts * ops_per_ctx;
	double t0, t1, naive_time, atomik_time;
	int i, j;

	printf("\n--- Benchmark 2: State Accumulation (%d ops) ---\n", total_ops);

	/* Naive: array of uint64_t with read-modify-write */
	uint64_t *states = calloc(num_contexts, sizeof(uint64_t));

	if (!states)
		return;

	print_metrics("before");

	t0 = time_sec();
	for (j = 0; j < ops_per_ctx; j++) {
		for (i = 0; i < num_contexts; i++) {
			states[i] ^= (uint64_t)(i * j + 1);
		}
	}
	t1 = time_sec();
	naive_time = t1 - t0;

	print_metrics("after naive");

	/* ATOMiK: create table + batch accum */
	struct atomik_create_table_args ct = { .num_contexts = num_contexts };

	if (ioctl(fd, ATOMIK_IOC_CREATE_TABLE, &ct) < 0) {
		printf("  CREATE_TABLE failed: %s\n", strerror(errno));
		free(states);
		return;
	}

	t0 = time_sec();
	for (j = 0; j < ops_per_ctx; j++) {
		for (i = 0; i < num_contexts; i++) {
			struct atomik_accum_args a = {
				.table_id = ct.table_id,
				.addr = i,
				.delta = (uint64_t)(i * j + 1),
			};
			ioctl(fd, ATOMIK_IOC_ACCUM, &a);
		}
	}
	t1 = time_sec();
	atomik_time = t1 - t0;

	print_metrics("after atomik");

	/* Verify correctness: read back a sample */
	struct atomik_read_args ra = {
		.table_id = ct.table_id,
		.addr = 0,
	};
	ioctl(fd, ATOMIK_IOC_READ, &ra);
	printf("\n  Verify ctx[0]: naive=%#lx atomik=%#lx %s\n",
	       states[0], (unsigned long)ra.state,
	       states[0] == ra.state ? "MATCH" : "MISMATCH");

	printf("  Naive (xor in userspace):  %.3f sec  (%.1f Mops/s)\n",
	       naive_time, total_ops / naive_time / 1e6);
	printf("  ATOMiK (ioctl per op):     %.3f sec  (%.1f Kops/s)\n",
	       atomik_time, total_ops / atomik_time / 1e3);
	printf("  Note: ioctl overhead dominates — batch mode is faster\n");

	uint32_t tid = ct.table_id;

	ioctl(fd, ATOMIK_IOC_DESTROY_TABLE, &tid);
	free(states);
}

/* --- Benchmark 3: Batch Throughput --- */

static void bench_batch(int fd)
{
	const int batch_size = 4096;
	const int num_batches = 100;
	const int total_ops = batch_size * num_batches;
	double t0, t1;
	int i, j;

	printf("\n--- Benchmark 3: Batch Throughput (%d ops in %d batches) ---\n",
	       total_ops, num_batches);

	struct atomik_create_table_args ct = { .num_contexts = batch_size };

	if (ioctl(fd, ATOMIK_IOC_CREATE_TABLE, &ct) < 0) {
		printf("  CREATE_TABLE failed: %s\n", strerror(errno));
		return;
	}

	struct atomik_batch_op *ops = malloc(batch_size * sizeof(*ops));

	if (!ops)
		return;

	/* Individual ioctls */
	print_metrics("before");

	t0 = time_sec();
	for (j = 0; j < num_batches; j++) {
		for (i = 0; i < batch_size; i++) {
			struct atomik_accum_args a = {
				.table_id = ct.table_id,
				.addr = i,
				.delta = j + 1,
			};
			ioctl(fd, ATOMIK_IOC_ACCUM, &a);
		}
	}
	t1 = time_sec();
	double individual_time = t1 - t0;

	print_metrics("after individual");

	/* Reset table */
	uint32_t tid = ct.table_id;

	ioctl(fd, ATOMIK_IOC_DESTROY_TABLE, &tid);
	ct.num_contexts = batch_size;
	ioctl(fd, ATOMIK_IOC_CREATE_TABLE, &ct);

	/* Batch ioctls */
	t0 = time_sec();
	for (j = 0; j < num_batches; j++) {
		for (i = 0; i < batch_size; i++) {
			ops[i].opcode = 1; /* ACCUM */
			memset(ops[i]._pad, 0, sizeof(ops[i]._pad));
			ops[i].addr = i;
			ops[i].value = j + 1;
		}
		struct atomik_batch_args ba = {
			.table_id = ct.table_id,
			.num_ops = batch_size,
			.ops_ptr = (uint64_t)(uintptr_t)ops,
		};
		ioctl(fd, ATOMIK_IOC_BATCH, &ba);
	}
	t1 = time_sec();
	double batch_time = t1 - t0;

	print_metrics("after batch");

	printf("\n  Individual ioctls: %.3f sec  (%.1f Kops/s)\n",
	       individual_time, total_ops / individual_time / 1e3);
	printf("  Batch ioctls:      %.3f sec  (%.1f Kops/s)\n",
	       batch_time, total_ops / batch_time / 1e3);
	printf("  Batch speedup:     %.1fx\n",
	       individual_time / batch_time);

	tid = ct.table_id;
	ioctl(fd, ATOMIK_IOC_DESTROY_TABLE, &tid);
	free(ops);
}

/* --- Benchmark 4: COW Monitoring Impact --- */

static int read_sysfs_ull(const char *attr, unsigned long long *val)
{
	char path[256], buf[64];
	FILE *f;

	snprintf(path, sizeof(path),
		 "/sys/class/atomik/atomik0/%s", attr);
	f = fopen(path, "r");
	if (!f)
		return -1;
	if (fgets(buf, sizeof(buf), f))
		*val = strtoull(buf, NULL, 10);
	fclose(f);
	return 0;
}

static void bench_cow_monitoring(void)
{
	unsigned long long faults_before, faults_after;
	unsigned long long copies_before, copies_after;
	unsigned long long bytes_before, bytes_after;
	double t0, t1, elapsed;
	int i;

	printf("\n--- Benchmark 4: COW Monitoring (200 fork+write cycles) ---\n");

	if (read_sysfs_ull("cow_faults_total", &faults_before) < 0) {
		printf("  COW monitoring not available (sysfs not found)\n");
		return;
	}
	read_sysfs_ull("cow_copies_performed", &copies_before);
	read_sysfs_ull("cow_bytes_copied", &bytes_before);

	print_metrics("before");

	t0 = time_sec();
	for (i = 0; i < 200; i++) {
		pid_t pid = fork();
		if (pid == 0) {
			/* Child: allocate + write 2MB to trigger COW */
			volatile char *data = malloc(2 * 1024 * 1024);
			if (data) {
				int j;
				for (j = 0; j < 2 * 1024 * 1024; j += 4096)
					data[j] = (char)(i & 0xff);
				free((void *)data);
			}
			_exit(0);
		} else if (pid > 0) {
			int status;
			waitpid(pid, &status, 0);
		}
	}
	t1 = time_sec();
	elapsed = t1 - t0;

	print_metrics("after");

	read_sysfs_ull("cow_faults_total", &faults_after);
	read_sysfs_ull("cow_copies_performed", &copies_after);
	read_sysfs_ull("cow_bytes_copied", &bytes_after);

	printf("\n  Duration:    %.3f sec (200 fork+write cycles)\n", elapsed);
	printf("  COW faults:  %llu (%.0f/sec)\n",
	       faults_after - faults_before,
	       (faults_after - faults_before) / elapsed);
	printf("  Page copies: %llu\n", copies_after - copies_before);

	{
		unsigned long long bytes_delta = bytes_after - bytes_before;
		if (bytes_delta > (1ULL << 20))
			printf("  Bytes copied: %.2f MB (%.1f MB/sec)\n",
			       (double)bytes_delta / (1ULL << 20),
			       (double)bytes_delta / (1ULL << 20) / elapsed);
		else
			printf("  Bytes copied: %llu bytes\n", bytes_delta);
	}

	/* Extrapolate to hourly rate */
	{
		double faults_per_sec = (faults_after - faults_before) / elapsed;
		double bytes_per_sec = (bytes_after - bytes_before) / elapsed;
		printf("\n  Extrapolated system COW overhead:\n");
		printf("    %.0f COW faults/hour\n", faults_per_sec * 3600);
		printf("    %.1f MB/hour of page copies\n",
		       bytes_per_sec * 3600 / (1024 * 1024));
	}
}

/* --- Main --- */

int main(void)
{
	int fd;

	printf("ATOMiK v0.4.0 — Performance Benchmark\n");
	printf("======================================\n");

	fd = open("/dev/atomik", O_RDWR);
	if (fd < 0) {
		perror("open /dev/atomik");
		printf("Module not loaded? Try: sudo insmod atomik.ko\n");
		return 1;
	}

	/* Module info */
	struct atomik_info info;

	if (ioctl(fd, ATOMIK_IOC_GET_INFO, &info) == 0) {
		printf("Module: v%d.%d.%d  Backend: %s\n",
		       (info.version >> 16) & 0xFF,
		       (info.version >> 8) & 0xFF,
		       info.version & 0xFF,
		       info.backend ? "hardware" : "software");
	}

	printf("\nSystem baseline:\n");
	print_metrics("idle");

	/* Get idle temperature as cooldown target */
	int idle_temp = get_cpu_temp();

	/* Allow system to settle before first benchmark */
	cooldown("Change Detection", idle_temp);

	bench_change_detection(fd);
	cooldown("State Accumulation", idle_temp);

	bench_accumulation(fd);
	cooldown("Batch Throughput", idle_temp);

	bench_batch(fd);
	cooldown("COW Monitoring", idle_temp);

	bench_cow_monitoring();

	printf("\n--- Final System State ---\n");
	print_metrics("final");

	/* Print module stats */
	if (ioctl(fd, ATOMIK_IOC_GET_INFO, &info) == 0) {
		printf("\n  Total ATOMiK ops this session: %llu\n",
		       (unsigned long long)info.ops_total);
		printf("  FP checks: %llu\n",
		       (unsigned long long)info.fp_checks);
		printf("  FP bytes saved: %llu\n",
		       (unsigned long long)info.fp_bytes_saved);
	}

	close(fd);
	printf("\nBenchmark complete.\n");
	return 0;
}
