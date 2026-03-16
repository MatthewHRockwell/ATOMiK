// SPDX-License-Identifier: GPL-2.0
/*
 * atomik-optimizer — COW suppression daemon
 *
 * Bridges ATOMiK's COW redundancy detection with Linux KSM (Kernel
 * Same-page Merging) to actually reclaim wasted memory.
 *
 * How it works:
 *   1. Reads ATOMiK cow_top to find processes with redundant COW copies
 *   2. Enables KSM scanning on the system (if not already running)
 *   3. Uses process_madvise(MADV_MERGEABLE) on high-redundancy processes
 *      to hint KSM to scan their pages for duplicates
 *   4. Monitors KSM pages_sharing to report actual memory reclaimed
 *   5. Adjusts KSM scan rate based on ATOMiK's COW detection rate
 *
 * The result: ATOMiK detects redundant COW copies in O(1) per fault,
 * then KSM reclaims the wasted pages. ATOMiK is the "eyes" (fast
 * detection), KSM is the "hands" (safe page merging).
 *
 * This is what makes ATOMiK worth $99/mo — no other tool combines
 * real-time COW detection with automatic memory optimization.
 *
 * Usage:
 *   atomik-optimizer              # Run in foreground
 *   atomik-optimizer --daemon     # Daemonize
 *   atomik-optimizer --once       # Single optimization pass
 *   atomik-optimizer --status     # Show optimizer state
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/syscall.h>
#include <sys/uio.h>

#define SYSFS_ATOMIK  "/sys/class/atomik/atomik0/"
#define SYSFS_KSM     "/sys/kernel/mm/ksm/"
#define MAX_PIDS      64
#define SCAN_INTERVAL 5  /* seconds between optimization passes */

static volatile int running = 1;

static void sighandler(int sig)
{
	(void)sig;
	running = 0;
}

/* --- Sysfs helpers --- */

static int write_sysfs(const char *base, const char *attr, const char *val)
{
	char path[256];
	int fd, ret;

	snprintf(path, sizeof(path), "%s%s", base, attr);
	fd = open(path, O_WRONLY);
	if (fd < 0)
		return -errno;
	ret = write(fd, val, strlen(val));
	close(fd);
	return ret > 0 ? 0 : -errno;
}

static long long read_sysfs_ll(const char *base, const char *attr)
{
	char path[256], buf[64];
	int fd;
	ssize_t n;

	snprintf(path, sizeof(path), "%s%s", base, attr);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;
	n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (n <= 0)
		return -1;
	buf[n] = '\0';
	return strtoll(buf, NULL, 10);
}

static int read_sysfs_str(const char *base, const char *attr,
			   char *buf, size_t len)
{
	char path[256];
	int fd;
	ssize_t n;

	snprintf(path, sizeof(path), "%s%s", base, attr);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -errno;
	n = read(fd, buf, len - 1);
	close(fd);
	if (n <= 0)
		return -1;
	buf[n] = '\0';
	return 0;
}

/* --- KSM control --- */

static int ksm_is_running(void)
{
	return read_sysfs_ll(SYSFS_KSM, "run") >= 1;
}

static int ksm_enable(void)
{
	int ret;

	if (ksm_is_running()) {
		printf("  KSM already running\n");
		return 0;
	}

	ret = write_sysfs(SYSFS_KSM, "run", "1");
	if (ret < 0) {
		fprintf(stderr, "  Failed to enable KSM (need root?): %s\n",
			strerror(-ret));
		return ret;
	}

	/* Tune KSM for ATOMiK-guided scanning */
	write_sysfs(SYSFS_KSM, "pages_to_scan", "1000");
	write_sysfs(SYSFS_KSM, "sleep_millisecs", "20");
	write_sysfs(SYSFS_KSM, "use_zero_pages", "1");

	printf("  KSM enabled (pages_to_scan=1000, sleep=20ms, zero_pages=on)\n");
	return 0;
}

static void ksm_print_stats(void)
{
	long long sharing = read_sysfs_ll(SYSFS_KSM, "pages_sharing");
	long long shared = read_sysfs_ll(SYSFS_KSM, "pages_shared");
	long long scanned = read_sysfs_ll(SYSFS_KSM, "pages_scanned");
	long long profit = read_sysfs_ll(SYSFS_KSM, "general_profit");

	if (sharing < 0)
		return;

	printf("  KSM: %lld pages sharing, %lld unique shared, "
	       "%lld scanned",
	       sharing, shared, scanned);
	if (profit > 0) {
		if (profit > 1024 * 1024)
			printf(", profit: %lld MB", profit / (1024 * 1024));
		else if (profit > 1024)
			printf(", profit: %lld KB", profit / 1024);
	}
	printf("\n");

	if (sharing > 0) {
		long long saved_bytes = sharing * 4096LL;

		printf("  Memory reclaimed by KSM: ");
		if (saved_bytes > (1LL << 30))
			printf("%.2f GB", (double)saved_bytes / (1LL << 30));
		else if (saved_bytes > (1LL << 20))
			printf("%.2f MB", (double)saved_bytes / (1LL << 20));
		else
			printf("%lld KB", saved_bytes >> 10);
		printf(" (%lld shared pages)\n", sharing);
	}
}

/* --- process_madvise via syscall --- */

/*
 * process_madvise(2) — advise about a process's memory usage.
 * Available since Linux 5.10. We use MADV_MERGEABLE to tell KSM
 * to scan this process's pages for merge candidates.
 *
 * This is the key system call that turns ATOMiK's COW detection
 * into actual memory savings: we identify which processes waste
 * memory (via cow_top), then tell KSM to fix them.
 */
#ifndef __NR_process_madvise
#define __NR_process_madvise 440
#endif

#ifndef MADV_MERGEABLE
#define MADV_MERGEABLE 12
#endif

static int process_madvise_mergeable(pid_t pid)
{
	char maps_path[64];
	FILE *f;
	char line[512];
	int advised = 0;
	int pidfd;

	/* Open pidfd for process_madvise */
	pidfd = syscall(SYS_pidfd_open, pid, 0);
	if (pidfd < 0)
		return -errno;

	/* Read /proc/PID/maps to find heap and anonymous regions */
	snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
	f = fopen(maps_path, "r");
	if (!f) {
		close(pidfd);
		return -errno;
	}

	while (fgets(line, sizeof(line), f)) {
		unsigned long start, end;
		char perms[8];
		char *mapping;

		if (sscanf(line, "%lx-%lx %7s", &start, &end, perms) != 3)
			continue;

		/* Only advise on writable anonymous mappings (heap, stack, mmap) */
		if (perms[1] != 'w')
			continue;

		/* Check if anonymous (no file mapping) */
		mapping = strrchr(line, ' ');
		if (mapping && mapping[1] != '\n' && mapping[1] != '[' &&
		    strcmp(mapping + 1, "\n") != 0)
			continue;  /* file-backed, skip */

		struct iovec iov = {
			.iov_base = (void *)start,
			.iov_len = end - start,
		};

		long ret = syscall(__NR_process_madvise, pidfd,
				   &iov, 1UL, MADV_MERGEABLE, 0U);
		if (ret >= 0)
			advised++;
	}

	fclose(f);
	close(pidfd);
	return advised;
}

/* --- Parse cow_top from ATOMiK sysfs --- */

struct cow_process {
	pid_t pid;
	char comm[32];
	unsigned long long redundant;
	unsigned long long faults;
	unsigned long long bytes_saved;
};

static int parse_cow_top(struct cow_process *procs, int max)
{
	char buf[4096];
	int n = 0;
	char *line;

	if (read_sysfs_str(SYSFS_ATOMIK, "cow_top", buf, sizeof(buf)) < 0)
		return 0;

	line = strtok(buf, "\n");
	while (line && n < max) {
		int pid;
		char comm[32];
		unsigned long long redundant, faults;

		if (sscanf(line, "%d %31s %llu/%llu",
			   &pid, comm, &redundant, &faults) >= 4) {
			procs[n].pid = pid;
			snprintf(procs[n].comm, sizeof(procs[n].comm), "%s", comm);
			procs[n].redundant = redundant;
			procs[n].faults = faults;
			n++;
		}
		line = strtok(NULL, "\n");
	}
	return n;
}

/* --- Check if process is still alive --- */
static int pid_alive(pid_t pid)
{
	char path[32];

	snprintf(path, sizeof(path), "/proc/%d", pid);
	return access(path, F_OK) == 0;
}

/* --- Optimization pass --- */

static int optimize_pass(int verbose)
{
	struct cow_process procs[MAX_PIDS];
	int nprocs, i, optimized = 0;
	long long cow_copies;

	cow_copies = read_sysfs_ll(SYSFS_ATOMIK, "cow_copies_performed");

	if (cow_copies <= 0) {
		if (verbose)
			printf("  No COW activity detected yet\n");
		return 0;
	}

	nprocs = parse_cow_top(procs, MAX_PIDS);
	if (nprocs == 0) {
		if (verbose)
			printf("  No processes with redundant COW copies\n");
		return 0;
	}

	for (i = 0; i < nprocs; i++) {
		int ret;

		/* Skip dead processes */
		if (!pid_alive(procs[i].pid))
			continue;

		/* Skip PID 1 (systemd) and kernel threads */
		if (procs[i].pid <= 2)
			continue;

		/* Only optimize processes with significant redundancy */
		if (procs[i].redundant < 10)
			continue;

		ret = process_madvise_mergeable(procs[i].pid);
		if (ret > 0) {
			optimized++;
			if (verbose)
				printf("  Optimized PID %d (%s): "
				       "%d VMAs marked MERGEABLE "
				       "(%llu redundant COW copies)\n",
				       procs[i].pid, procs[i].comm,
				       ret, procs[i].redundant);
		} else if (ret < 0 && verbose) {
			printf("  Skipped PID %d (%s): %s\n",
			       procs[i].pid, procs[i].comm,
			       strerror(-ret));
		}
	}

	return optimized;
}

/* --- Status display --- */

static void show_status(void)
{
	long long cow_faults, cow_copies, cow_redundant, cow_rate;
	long long cow_bytes_saved;

	cow_faults = read_sysfs_ll(SYSFS_ATOMIK, "cow_faults_total");
	cow_copies = read_sysfs_ll(SYSFS_ATOMIK, "cow_copies_performed");
	cow_redundant = read_sysfs_ll(SYSFS_ATOMIK, "cow_copies_redundant");
	cow_rate = read_sysfs_ll(SYSFS_ATOMIK, "cow_redundancy_rate");
	cow_bytes_saved = read_sysfs_ll(SYSFS_ATOMIK, "cow_bytes_saved");

	printf("ATOMiK Optimizer Status\n");
	printf("=======================\n\n");

	printf("COW Detection:\n");
	printf("  Faults intercepted:  %lld\n", cow_faults);
	printf("  Copies performed:    %lld\n", cow_copies);
	printf("  Redundant copies:    %lld (%lld%% redundancy)\n",
	       cow_redundant, cow_rate);

	if (cow_bytes_saved > (1LL << 30))
		printf("  Memory wasted:       %.2f GB\n",
		       (double)cow_bytes_saved / (1LL << 30));
	else if (cow_bytes_saved > (1LL << 20))
		printf("  Memory wasted:       %.2f MB\n",
		       (double)cow_bytes_saved / (1LL << 20));
	else
		printf("  Memory wasted:       %lld KB\n",
		       cow_bytes_saved >> 10);

	printf("\nKSM Recovery:\n");
	printf("  Status: %s\n", ksm_is_running() ? "ACTIVE" : "INACTIVE");
	ksm_print_stats();
}

/* --- Main --- */

static void print_usage(const char *prog)
{
	printf("Usage: %s [OPTIONS]\n\n", prog);
	printf("ATOMiK COW Optimizer — Automatic memory deduplication\n\n");
	printf("Detects redundant copy-on-write pages and enables KSM\n");
	printf("merging to reclaim wasted memory.\n\n");
	printf("Options:\n");
	printf("  --once     Run a single optimization pass and exit\n");
	printf("  --daemon   Run in background\n");
	printf("  --status   Show current optimizer state\n");
	printf("  --help     Show this help\n");
}

int main(int argc, char *argv[])
{
	int daemon_mode = 0, once_mode = 0, status_mode = 0;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--daemon") == 0)
			daemon_mode = 1;
		else if (strcmp(argv[i], "--once") == 0)
			once_mode = 1;
		else if (strcmp(argv[i], "--status") == 0)
			status_mode = 1;
		else if (strcmp(argv[i], "--help") == 0 ||
			 strcmp(argv[i], "-h") == 0) {
			print_usage(argv[0]);
			return 0;
		}
	}

	/* Check ATOMiK module is loaded */
	if (read_sysfs_ll(SYSFS_ATOMIK, "cow_faults_total") < 0) {
		fprintf(stderr, "ATOMiK module not loaded or COW "
			"monitoring unavailable\n");
		return 1;
	}

	if (status_mode) {
		show_status();
		return 0;
	}

	printf("ATOMiK COW Optimizer v0.4.0\n");
	printf("===========================\n\n");

	/* Enable KSM */
	printf("Initializing KSM...\n");
	if (ksm_enable() < 0) {
		fprintf(stderr, "Cannot enable KSM. Run as root.\n");
		return 1;
	}

	if (once_mode) {
		printf("\nRunning optimization pass...\n");
		int n = optimize_pass(1);
		printf("\nOptimized %d processes\n", n);
		ksm_print_stats();
		return 0;
	}

	/* Continuous mode */
	if (daemon_mode) {
		if (daemon(0, 0) < 0) {
			perror("daemon");
			return 1;
		}
	}

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	printf("\nMonitoring COW activity (Ctrl-C to stop)...\n\n");

	long long prev_redundant = read_sysfs_ll(SYSFS_ATOMIK,
						 "cow_copies_redundant");
	long long prev_saved = read_sysfs_ll(SYSFS_ATOMIK,
					     "cow_bytes_saved");

	while (running) {
		long long cur_redundant, cur_saved, delta_redundant, delta_saved;
		time_t now;
		struct tm *tm;
		char timestr[32];
		int n;

		sleep(SCAN_INTERVAL);
		if (!running)
			break;

		cur_redundant = read_sysfs_ll(SYSFS_ATOMIK,
					      "cow_copies_redundant");
		cur_saved = read_sysfs_ll(SYSFS_ATOMIK, "cow_bytes_saved");
		delta_redundant = cur_redundant - prev_redundant;
		delta_saved = cur_saved - prev_saved;

		now = time(NULL);
		tm = localtime(&now);
		strftime(timestr, sizeof(timestr), "%H:%M:%S", tm);

		n = optimize_pass(0);

		long long sharing = read_sysfs_ll(SYSFS_KSM, "pages_sharing");
		long long reclaimed = sharing > 0 ? sharing * 4096LL : 0;

		printf("[%s] +%lld redundant copies ", timestr, delta_redundant);
		if (delta_saved > (1LL << 20))
			printf("(+%.1f MB wasted) ",
			       (double)delta_saved / (1LL << 20));
		else
			printf("(+%lld KB wasted) ", delta_saved >> 10);

		printf("| optimized %d procs | KSM reclaimed: ", n);
		if (reclaimed > (1LL << 20))
			printf("%.1f MB", (double)reclaimed / (1LL << 20));
		else
			printf("%lld KB", reclaimed >> 10);
		printf("\n");

		prev_redundant = cur_redundant;
		prev_saved = cur_saved;
	}

	printf("\nShutting down optimizer...\n");
	show_status();
	return 0;
}
