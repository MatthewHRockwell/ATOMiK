// SPDX-License-Identifier: GPL-2.0
/*
 * atomik-status — Display ATOMiK module status from sysfs
 *
 * Shows real-time system optimization metrics including COW
 * redundancy detection with per-process breakdowns.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SYSFS_BASE "/sys/class/atomik/atomik0/"

static int read_sysfs(const char *attr, char *buf, size_t len)
{
	char path[256];
	FILE *f;
	size_t n;

	snprintf(path, sizeof(path), SYSFS_BASE "%s", attr);
	f = fopen(path, "r");
	if (!f)
		return -errno;
	n = fread(buf, 1, len - 1, f);
	buf[n] = '\0';
	if (n > 0 && buf[n - 1] == '\n')
		buf[n - 1] = '\0';
	fclose(f);
	return 0;
}

static unsigned long long read_sysfs_u64(const char *attr)
{
	char buf[64];

	if (read_sysfs(attr, buf, sizeof(buf)) < 0)
		return 0;
	return strtoull(buf, NULL, 10);
}

static void print_bytes(const char *label, unsigned long long bytes)
{
	if (bytes > (1ULL << 30))
		printf("  %-22s%.2f GB\n", label,
		       (double)bytes / (1ULL << 30));
	else if (bytes > (1ULL << 20))
		printf("  %-22s%.2f MB\n", label,
		       (double)bytes / (1ULL << 20));
	else if (bytes > (1ULL << 10))
		printf("  %-22s%.2f KB\n", label,
		       (double)bytes / (1ULL << 10));
	else
		printf("  %-22s%llu bytes\n", label, bytes);
}

/* Read multi-line sysfs attribute (like cow_top) */
static int read_sysfs_multi(const char *attr, char *buf, size_t len)
{
	char path[256];
	FILE *f;
	size_t n;

	snprintf(path, sizeof(path), SYSFS_BASE "%s", attr);
	f = fopen(path, "r");
	if (!f)
		return -errno;
	n = fread(buf, 1, len - 1, f);
	buf[n] = '\0';
	fclose(f);
	return 0;
}

int main(int argc, char *argv[])
{
	char version[32], backend[64], license[128];
	unsigned long long ops_total, ops_load, ops_accum, ops_read, ops_swap;
	unsigned long long fp_total, fp_unchanged, fp_hit, fp_saved;
	int quiet = 0;

	if (argc > 1 && strcmp(argv[1], "--quiet") == 0)
		quiet = 1;

	if (read_sysfs("version", version, sizeof(version)) < 0) {
		if (!quiet)
			fprintf(stderr, "ATOMiK module not loaded\n");
		return 1;
	}

	read_sysfs("backend", backend, sizeof(backend));
	read_sysfs("license_status", license, sizeof(license));

	ops_total = read_sysfs_u64("ops_total");
	ops_load = read_sysfs_u64("ops_load");
	ops_accum = read_sysfs_u64("ops_accum");
	ops_read = read_sysfs_u64("ops_read");
	ops_swap = read_sysfs_u64("ops_swap");
	fp_total = read_sysfs_u64("fp_checks_total");
	fp_unchanged = read_sysfs_u64("fp_checks_unchanged");
	fp_hit = read_sysfs_u64("fp_hit_rate");
	fp_saved = read_sysfs_u64("fp_bytes_saved");

	if (quiet) {
		printf("ATOMiK v%s OK\n", version);
		return 0;
	}

	printf("ATOMiK v%s — Delta-State Accelerator\n", version);
	printf("  Backend:     %s\n", backend);
	printf("  License:     %s\n", license);
	printf("  Status:      ACTIVE\n");
	printf("\n");

	/* Delta-state operations */
	if (ops_total > 0) {
		printf("Delta-State Operations: %llu total\n", ops_total);
		printf("  LOAD: %llu  ACCUM: %llu  READ: %llu  SWAP: %llu\n",
		       ops_load, ops_accum, ops_read, ops_swap);
		printf("\n");
	}

	/* Fingerprinting */
	if (fp_total > 0) {
		printf("Fingerprinting: %llu checks (%llu unchanged, %llu%% hit rate)\n",
		       fp_total, fp_unchanged, fp_hit);
		print_bytes("Bytes saved:", fp_saved);
		printf("\n");
	}

	/* COW optimization — the main product value */
	{
		char cow_enabled[8];
		unsigned long long cow_faults, cow_copies, cow_redundant;
		unsigned long long cow_bytes_copied, cow_bytes_saved;
		unsigned long long cow_rate;
		char cow_top_buf[4096];

		if (read_sysfs("cow_enabled", cow_enabled,
			       sizeof(cow_enabled)) < 0)
			goto done;

		cow_faults = read_sysfs_u64("cow_faults_total");
		cow_copies = read_sysfs_u64("cow_copies_performed");
		cow_redundant = read_sysfs_u64("cow_copies_redundant");
		cow_bytes_copied = read_sysfs_u64("cow_bytes_copied");
		cow_bytes_saved = read_sysfs_u64("cow_bytes_saved");
		cow_rate = read_sysfs_u64("cow_redundancy_rate");

		printf("COW Optimization: %s\n",
		       cow_enabled[0] == '1' ? "ACTIVE" : "DISABLED");
		printf("  Faults intercepted:   %llu\n", cow_faults);
		printf("  Page copies:          %llu total\n", cow_copies);
		printf("  Redundant copies:     %llu (%llu%% redundancy rate)\n",
		       cow_redundant, cow_rate);
		print_bytes("Memory copied:", cow_bytes_copied);
		print_bytes("Memory wasted:", cow_bytes_saved);

		if (cow_bytes_saved > 0) {
			printf("\n");
			printf("  Your system wasted ");
			if (cow_bytes_saved > (1ULL << 30))
				printf("%.2f GB",
				       (double)cow_bytes_saved / (1ULL << 30));
			else if (cow_bytes_saved > (1ULL << 20))
				printf("%.2f MB",
				       (double)cow_bytes_saved / (1ULL << 20));
			else
				printf("%llu KB", cow_bytes_saved >> 10);
			printf(" on unnecessary page copies.\n");
			printf("  ATOMiK detected %llu redundant copy-on-write operations.\n",
			       cow_redundant);
		}

		/* Per-process breakdown */
		if (read_sysfs_multi("cow_top", cow_top_buf,
				     sizeof(cow_top_buf)) == 0 &&
		    strstr(cow_top_buf, "(no redundant") == NULL) {
			printf("\n");
			printf("Top processes by redundant COW copies:\n");
			printf("  %-8s %-16s %-20s %s\n",
			       "PID", "COMMAND", "REDUNDANT/TOTAL", "WASTED");
			printf("  %-8s %-16s %-20s %s\n",
			       "---", "-------", "---------------", "------");

			/* Parse and reformat cow_top output */
			char *line = strtok(cow_top_buf, "\n");
			while (line) {
				printf("  %s\n", line);
				line = strtok(NULL, "\n");
			}
		}
	}

	/* Network monitoring (v0.4) */
	{
		char net_enabled[8];
		unsigned long long net_sends, net_redundant;
		unsigned long long net_bytes, net_bytes_red;
		unsigned long long net_rate;

		if (read_sysfs("net_enabled", net_enabled,
			       sizeof(net_enabled)) == 0) {
			net_sends = read_sysfs_u64("net_sends_total");
			net_redundant = read_sysfs_u64("net_sends_redundant");
			net_bytes = read_sysfs_u64("net_bytes_total");
			net_bytes_red = read_sysfs_u64("net_bytes_redundant");
			net_rate = read_sysfs_u64("net_redundancy_rate");

			printf("\nNetwork Monitoring: %s\n",
			       net_enabled[0] == '1' ? "ACTIVE" : "DISABLED");
			printf("  TCP sends tracked:    %llu\n", net_sends);
			printf("  Redundant sends:      %llu (%llu%% redundancy rate)\n",
			       net_redundant, net_rate);
			print_bytes("Traffic tracked:", net_bytes);
			print_bytes("Redundant traffic:", net_bytes_red);
		}
	}

	/* Container/cgroup breakdown */
	{
		char cg_buf[4096];

		if (read_sysfs_multi("cgroup_top", cg_buf,
				     sizeof(cg_buf)) == 0 &&
		    strstr(cg_buf, "(no container") == NULL) {
			printf("\nPer-Container/Cgroup Waste:\n");
			printf("  %-42s %s\n",
			       "CONTAINER/CGROUP", "COW + NETWORK WASTE");
			printf("  %-42s %s\n",
			       "------------------", "-------------------");

			char *line = strtok(cg_buf, "\n");
			while (line) {
				printf("  %s\n", line);
				line = strtok(NULL, "\n");
			}
		}
	}

done:
	printf("\n");
	return 0;
}
