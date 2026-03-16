// SPDX-License-Identifier: GPL-2.0
/*
 * atomik-dashboard — Real-time ATOMiK optimization monitor
 *
 * Terminal UI showing live COW detection rates, memory savings,
 * per-process breakdown, and KSM recovery stats. Updates every second.
 *
 * This is the "feel it working" tool — customers see ATOMiK actively
 * optimizing their system in real time. Essential for demos and trials.
 *
 * Usage:
 *   atomik-dashboard              # Full TUI, updates every 1s
 *   atomik-dashboard --compact    # Single-line updates
 *   atomik-dashboard --interval 5 # Update every 5 seconds
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>

#define SYSFS_ATOMIK  "/sys/class/atomik/atomik0/"
#define SYSFS_KSM     "/sys/kernel/mm/ksm/"

/* ANSI color codes */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_RED     "\033[31m"
#define C_CYAN    "\033[36m"
#define C_WHITE   "\033[37m"
#define C_BG_BLUE "\033[44m"
#define CLEAR     "\033[2J\033[H"
#define HIDE_CUR  "\033[?25l"
#define SHOW_CUR  "\033[?25h"

static volatile int running = 1;

static void sighandler(int sig)
{
	(void)sig;
	running = 0;
}

static long long read_ll(const char *base, const char *attr)
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

static int read_str(const char *base, const char *attr, char *buf, size_t len)
{
	char path[256];
	int fd;
	ssize_t n;

	snprintf(path, sizeof(path), "%s%s", base, attr);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;
	n = read(fd, buf, len - 1);
	close(fd);
	if (n <= 0)
		return -1;
	buf[n] = '\0';
	/* strip trailing newline */
	if (n > 0 && buf[n - 1] == '\n')
		buf[n - 1] = '\0';
	return 0;
}

static void format_bytes(char *buf, size_t len, long long bytes)
{
	if (bytes < 0)
		snprintf(buf, len, "N/A");
	else if (bytes > (1LL << 30))
		snprintf(buf, len, "%.2f GB", (double)bytes / (1LL << 30));
	else if (bytes > (1LL << 20))
		snprintf(buf, len, "%.2f MB", (double)bytes / (1LL << 20));
	else if (bytes > (1LL << 10))
		snprintf(buf, len, "%.1f KB", (double)bytes / (1LL << 10));
	else
		snprintf(buf, len, "%lld B", bytes);
}

static void format_rate(char *buf, size_t len, double per_sec)
{
	if (per_sec > 1e6)
		snprintf(buf, len, "%.1fM/s", per_sec / 1e6);
	else if (per_sec > 1e3)
		snprintf(buf, len, "%.1fK/s", per_sec / 1e3);
	else
		snprintf(buf, len, "%.0f/s", per_sec);
}

/* --- Progress bar --- */
static void print_bar(int width, int pct, const char *color)
{
	int filled = width * pct / 100;
	int i;

	printf("%s[", color);
	for (i = 0; i < width; i++) {
		if (i < filled)
			printf("#");
		else
			printf(" ");
	}
	printf("]%s %d%%", C_RESET, pct);
}

static int get_term_width(void)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
		return ws.ws_col;
	return 80;
}

/* --- Snapshot struct --- */
struct snapshot {
	long long cow_faults;
	long long cow_copies;
	long long cow_redundant;
	long long cow_bytes_copied;
	long long cow_bytes_saved;
	long long cow_rate;
	long long ksm_sharing;
	long long ksm_shared;
	long long ksm_scanned;
	long long ops_total;
	long long net_sends;
	long long net_redundant;
	long long net_bytes;
	long long net_bytes_red;
	long long net_rate;
	time_t timestamp;
};

static void take_snapshot(struct snapshot *s)
{
	s->cow_faults = read_ll(SYSFS_ATOMIK, "cow_faults_total");
	s->cow_copies = read_ll(SYSFS_ATOMIK, "cow_copies_performed");
	s->cow_redundant = read_ll(SYSFS_ATOMIK, "cow_copies_redundant");
	s->cow_bytes_copied = read_ll(SYSFS_ATOMIK, "cow_bytes_copied");
	s->cow_bytes_saved = read_ll(SYSFS_ATOMIK, "cow_bytes_saved");
	s->cow_rate = read_ll(SYSFS_ATOMIK, "cow_redundancy_rate");
	s->ksm_sharing = read_ll(SYSFS_KSM, "pages_sharing");
	s->ksm_shared = read_ll(SYSFS_KSM, "pages_shared");
	s->ksm_scanned = read_ll(SYSFS_KSM, "pages_scanned");
	s->ops_total = read_ll(SYSFS_ATOMIK, "ops_total");
	s->net_sends = read_ll(SYSFS_ATOMIK, "net_sends_total");
	s->net_redundant = read_ll(SYSFS_ATOMIK, "net_sends_redundant");
	s->net_bytes = read_ll(SYSFS_ATOMIK, "net_bytes_total");
	s->net_bytes_red = read_ll(SYSFS_ATOMIK, "net_bytes_redundant");
	s->net_rate = read_ll(SYSFS_ATOMIK, "net_redundancy_rate");
	s->timestamp = time(NULL);
}

/* --- Main dashboard render --- */

static void render_dashboard(struct snapshot *cur, struct snapshot *prev,
			     int interval)
{
	char version[32], backend[64], license[128];
	char cow_top[4096];
	char buf1[32], buf2[32], buf3[32];
	double dt = (double)(cur->timestamp - prev->timestamp);
	int width = get_term_width();
	int bar_width = width > 80 ? 40 : 20;
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);

	if (dt < 0.5) dt = 1.0;  /* avoid division by zero */

	/* Read static info */
	read_str(SYSFS_ATOMIK, "version", version, sizeof(version));
	read_str(SYSFS_ATOMIK, "backend", backend, sizeof(backend));
	read_str(SYSFS_ATOMIK, "license_status", license, sizeof(license));

	/* Clear and draw */
	printf(CLEAR);

	/* Header */
	printf(C_BOLD C_BG_BLUE C_WHITE);
	printf(" ATOMiK v%s Dashboard", version);
	{
		char timestr[16];

		strftime(timestr, sizeof(timestr), "%H:%M:%S", tm);
		int padding = width - 24 - (int)strlen(version);

		printf("%*s%s ", padding > 0 ? padding : 1, "", timestr);
	}
	printf(C_RESET "\n");

	printf(C_DIM "  Backend: %s  |  License: %s" C_RESET "\n\n",
	       backend, license);

	/* COW Detection */
	printf(C_BOLD " COW Optimization" C_RESET "\n");
	printf(" ================\n");

	/* Rates */
	double faults_per_sec = (cur->cow_faults - prev->cow_faults) / dt;
	double copies_per_sec = (cur->cow_copies - prev->cow_copies) / dt;
	double redundant_per_sec = (cur->cow_redundant - prev->cow_redundant) / dt;
	double bytes_per_sec = (cur->cow_bytes_saved - prev->cow_bytes_saved) / dt;

	format_rate(buf1, sizeof(buf1), faults_per_sec);
	printf("  Faults:     %lld total (%s%s%s)\n",
	       cur->cow_faults, C_CYAN, buf1, C_RESET);

	format_rate(buf1, sizeof(buf1), copies_per_sec);
	printf("  Copies:     %lld total (%s%s%s)\n",
	       cur->cow_copies, C_YELLOW, buf1, C_RESET);

	format_rate(buf1, sizeof(buf1), redundant_per_sec);
	printf("  Redundant:  %lld total (%s%s%s)\n",
	       cur->cow_redundant, C_GREEN, buf1, C_RESET);

	/* Redundancy rate bar */
	int rate = cur->cow_rate > 100 ? 100 : (int)cur->cow_rate;
	const char *rate_color = rate > 50 ? C_RED : rate > 20 ? C_YELLOW : C_GREEN;

	printf("  Redundancy: ");
	print_bar(bar_width, rate, rate_color);
	printf("\n");

	/* Memory stats */
	format_bytes(buf1, sizeof(buf1), cur->cow_bytes_copied);
	format_bytes(buf2, sizeof(buf2), cur->cow_bytes_saved);
	format_bytes(buf3, sizeof(buf3), (long long)bytes_per_sec);
	printf("\n  Memory copied:  %s\n", buf1);
	printf("  " C_RED "Memory wasted:  %s" C_RESET " (%s%s/s%s)\n",
	       buf2, C_RED, buf3, C_RESET);

	/* Hourly extrapolation */
	double hourly_waste = bytes_per_sec * 3600;
	format_bytes(buf1, sizeof(buf1), (long long)hourly_waste);
	printf("  Projected:    %s%s/hour wasted%s\n",
	       C_RED C_BOLD, buf1, C_RESET);

	/* KSM Recovery */
	printf("\n" C_BOLD " KSM Memory Recovery" C_RESET "\n");
	printf(" ====================\n");

	if (cur->ksm_sharing >= 0) {
		long long reclaimed = cur->ksm_sharing * 4096LL;

		format_bytes(buf1, sizeof(buf1), reclaimed);
		printf("  Status:      %s%s%s\n",
		       cur->ksm_sharing > 0 ? C_GREEN : C_DIM,
		       cur->ksm_sharing > 0 ? "ACTIVE" : "IDLE",
		       C_RESET);
		printf("  Pages merged: %lld sharing / %lld unique\n",
		       cur->ksm_sharing, cur->ksm_shared);
		printf("  " C_GREEN "Reclaimed:     %s" C_RESET "\n", buf1);
	} else {
		printf("  KSM: not available\n");
	}

	/* Network Monitoring */
	if (cur->net_sends >= 0) {
		double net_rate_sec = (cur->net_sends - prev->net_sends) / dt;
		double net_red_sec = (cur->net_redundant - prev->net_redundant) / dt;
		int nrate = cur->net_rate > 100 ? 100 : (int)cur->net_rate;

		printf("\n" C_BOLD " Network Monitoring" C_RESET "\n");
		printf(" ==================\n");

		format_rate(buf1, sizeof(buf1), net_rate_sec);
		printf("  TCP sends:    %lld total (%s%s%s)\n",
		       cur->net_sends, C_CYAN, buf1, C_RESET);

		format_rate(buf1, sizeof(buf1), net_red_sec);
		printf("  Redundant:    %lld total (%s%s%s)\n",
		       cur->net_redundant, C_GREEN, buf1, C_RESET);

		printf("  Network:      ");
		print_bar(bar_width, nrate,
			  nrate > 50 ? C_RED : nrate > 20 ? C_YELLOW : C_GREEN);
		printf("\n");

		format_bytes(buf1, sizeof(buf1), cur->net_bytes);
		format_bytes(buf2, sizeof(buf2), cur->net_bytes_red);
		printf("  Traffic:      %s total, %s%s redundant%s\n",
		       buf1, C_RED, buf2, C_RESET);
	}

	/* Per-process breakdown */
	if (read_str(SYSFS_ATOMIK, "cow_top", cow_top, sizeof(cow_top)) == 0 &&
	    strstr(cow_top, "(no redundant") == NULL) {
		printf("\n" C_BOLD " Top Processes (by wasted COW copies)" C_RESET "\n");
		printf(" =====================================\n");
		printf("  " C_DIM "%-8s %-16s %-20s %s" C_RESET "\n",
		       "PID", "COMMAND", "REDUNDANT/TOTAL", "WASTED");

		char *line = strtok(cow_top, "\n");
		int count = 0;

		while (line && count < 10) {
			printf("  %s\n", line);
			line = strtok(NULL, "\n");
			count++;
		}
	}

	/* Delta-state ops (if any) */
	if (cur->ops_total > 0) {
		printf("\n" C_BOLD " Delta-State Operations" C_RESET "\n");
		printf(" ======================\n");
		printf("  Total: %lld\n", cur->ops_total);
	}

	/* Footer */
	printf("\n" C_DIM " Press Ctrl-C to exit  |  "
	       "Refresh: %ds  |  atomik.tech" C_RESET "\n",
	       interval);
}

/* --- Compact mode --- */

static void render_compact(struct snapshot *cur, struct snapshot *prev)
{
	double dt = (double)(cur->timestamp - prev->timestamp);
	char buf1[32], buf2[32];
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	char timestr[16];

	if (dt < 0.5) dt = 1.0;

	strftime(timestr, sizeof(timestr), "%H:%M:%S", tm);

	double faults_rate = (cur->cow_faults - prev->cow_faults) / dt;
	double waste_rate = (cur->cow_bytes_saved - prev->cow_bytes_saved) / dt;

	format_bytes(buf1, sizeof(buf1), cur->cow_bytes_saved);
	format_rate(buf2, sizeof(buf2), faults_rate);

	printf("[%s] COW: %lld faults (%s), %lld%% redundant, "
	       "wasted: %s",
	       timestr, cur->cow_faults, buf2,
	       cur->cow_rate, buf1);

	if (cur->ksm_sharing > 0) {
		format_bytes(buf1, sizeof(buf1),
			     cur->ksm_sharing * 4096LL);
		printf(", KSM reclaimed: %s", buf1);
	}

	format_bytes(buf1, sizeof(buf1), (long long)(waste_rate * 3600));
	printf(" (~%s/hr)", buf1);

	if (cur->net_sends > 0)
		printf(", NET: %lld sends (%lld%% dup)",
		       cur->net_sends, cur->net_rate);

	printf("\n");
}

int main(int argc, char *argv[])
{
	int compact = 0, interval = 1;
	struct snapshot prev, cur;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--compact") == 0)
			compact = 1;
		else if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc)
			interval = atoi(argv[++i]);
		else if (strcmp(argv[i], "--help") == 0 ||
			 strcmp(argv[i], "-h") == 0) {
			printf("Usage: %s [--compact] [--interval N]\n\n"
			       "Real-time ATOMiK optimization dashboard.\n\n"
			       "  --compact     Single-line output per update\n"
			       "  --interval N  Update every N seconds (default: 1)\n",
			       argv[0]);
			return 0;
		}
	}

	/* Check ATOMiK module */
	if (read_ll(SYSFS_ATOMIK, "cow_faults_total") < 0) {
		fprintf(stderr, "ATOMiK module not loaded\n");
		return 1;
	}

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	if (!compact) {
		printf(HIDE_CUR);
		fflush(stdout);
	}

	take_snapshot(&prev);

	while (running) {
		sleep(interval);
		if (!running)
			break;

		take_snapshot(&cur);

		if (compact)
			render_compact(&cur, &prev);
		else
			render_dashboard(&cur, &prev, interval);

		fflush(stdout);
		prev = cur;
	}

	if (!compact) {
		printf(SHOW_CUR);
		printf(CLEAR);
	}

	printf("ATOMiK dashboard stopped.\n");
	return 0;
}
