// SPDX-License-Identifier: GPL-2.0
/*
 * atomik-report — Generate formatted ATOMiK waste analysis report
 *
 * Reads metrics from /proc/atomik, /sys/class/atomik/atomik0/,
 * and system files to produce a comprehensive waste analysis
 * suitable for sales demos and customer reporting.
 *
 * Compile: cc -Wall -O2 -o atomik-report atomik-report.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#define PROC_ATOMIK      "/proc/atomik"
#define PROC_ATOMIK_COW  "/proc/atomik/cow"
#define PROC_ATOMIK_NET  "/proc/atomik/net"
#define PROC_UPTIME      "/proc/uptime"
#define SYSFS_BASE       "/sys/class/atomik/atomik0/"
#define PROC_VERSION     "/proc/version"

#define MAX_PROCS   16
#define MAX_CGROUPS 16

/* ── Output format flags ─────────────────────────────────────── */

enum output_fmt {
	FMT_TEXT,
	FMT_JSON,
	FMT_CSV,
	FMT_BRIEF,
};

/* ── Data structures ─────────────────────────────────────────── */

struct proc_entry {
	int    pid;
	char   comm[64];
	unsigned long long redundant;
	unsigned long long faults;
	unsigned long long pct;
	double bytes_wasted;   /* in bytes */
	char   wasted_str[32]; /* human-readable from sysfs */
};

struct cgroup_entry {
	char   name[128];
	unsigned long long cow_faults;
	double cow_waste;      /* bytes */
	char   cow_str[32];
	unsigned long long net_sends;
	double net_waste;      /* bytes */
	char   net_str[32];
	double total;
};

struct report_data {
	/* Version / backend */
	char version[32];
	char backend[64];
	char license[128];
	int  license_days;

	/* System info */
	char kernel[128];
	double uptime_sec;

	/* Core ops */
	unsigned long long ops_total;
	unsigned long long ops_load;
	unsigned long long ops_accum;
	unsigned long long ops_read;
	unsigned long long ops_swap;

	/* Fingerprinting */
	unsigned long long fp_checks;
	unsigned long long fp_unchanged;
	unsigned long long fp_bytes;

	/* COW */
	unsigned long long cow_faults;
	unsigned long long cow_copies;
	unsigned long long cow_redundant;
	unsigned long long cow_bytes_copied;
	unsigned long long cow_bytes_saved;
	char cow_redundancy_rate[16];

	/* Network */
	unsigned long long net_sends;
	unsigned long long net_redundant;
	unsigned long long net_bytes;
	unsigned long long net_bytes_redundant;
	char net_redundancy_rate[16];

	/* Top processes */
	struct proc_entry procs[MAX_PROCS];
	int nprocs;

	/* Cgroups */
	struct cgroup_entry cgroups[MAX_CGROUPS];
	int ncgroups;

	/* Timestamp */
	char timestamp[64];
};

/* ── File reading helpers ────────────────────────────────────── */

static int read_file(const char *path, char *buf, size_t len)
{
	FILE *f;
	size_t n;

	f = fopen(path, "r");
	if (!f)
		return -errno;
	n = fread(buf, 1, len - 1, f);
	buf[n] = '\0';
	fclose(f);
	return 0;
}

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

/* Extract key=value from /proc/atomik output */
static unsigned long long kv_get_u64(const char *data, const char *key)
{
	char needle[64];
	const char *p;

	snprintf(needle, sizeof(needle), "%s=", key);
	p = strstr(data, needle);
	if (!p)
		return 0;
	p += strlen(needle);
	return strtoull(p, NULL, 10);
}

static void kv_get_str(const char *data, const char *key,
		       char *out, size_t outlen)
{
	char needle[64];
	const char *p, *end;
	size_t len;

	snprintf(needle, sizeof(needle), "%s=", key);
	p = strstr(data, needle);
	if (!p) {
		out[0] = '\0';
		return;
	}
	p += strlen(needle);
	end = strchr(p, '\n');
	len = end ? (size_t)(end - p) : strlen(p);
	if (len >= outlen)
		len = outlen - 1;
	memcpy(out, p, len);
	out[len] = '\0';
}

/* ── Human-readable byte formatting ─────────────────────────── */

static void format_bytes(unsigned long long bytes, char *buf, size_t len)
{
	if (bytes >= (1ULL << 30))
		snprintf(buf, len, "%.1f GB",
			 (double)bytes / (1ULL << 30));
	else if (bytes >= (1ULL << 20))
		snprintf(buf, len, "%.1f MB",
			 (double)bytes / (1ULL << 20));
	else if (bytes >= (1ULL << 10))
		snprintf(buf, len, "%.1f KB",
			 (double)bytes / (1ULL << 10));
	else
		snprintf(buf, len, "%llu bytes", bytes);
}

static void format_number(unsigned long long n, char *buf, size_t len)
{
	char raw[32];
	int rawlen, groups, i, j;

	snprintf(raw, sizeof(raw), "%llu", n);
	rawlen = (int)strlen(raw);
	groups = (rawlen - 1) / 3;
	if ((int)len < rawlen + groups + 1) {
		snprintf(buf, len, "%s", raw);
		return;
	}
	j = 0;
	for (i = 0; i < rawlen; i++) {
		int remain = rawlen - i;
		if (i > 0 && remain % 3 == 0)
			buf[j++] = ',';
		buf[j++] = raw[i];
	}
	buf[j] = '\0';
}

static void format_uptime(double seconds, char *buf, size_t len)
{
	int days, hours, mins;

	days = (int)(seconds / 86400);
	hours = (int)((seconds - days * 86400) / 3600);
	mins = (int)((seconds - days * 86400 - hours * 3600) / 60);

	if (days > 0)
		snprintf(buf, len, "%dd %dh %dm since module load",
			 days, hours, mins);
	else if (hours > 0)
		snprintf(buf, len, "%dh %dm since module load", hours, mins);
	else
		snprintf(buf, len, "%dm since module load", mins);
}

/* ── Parse cow_top sysfs output ──────────────────────────────── */
/*
 * Format from kernel:
 *   <pid> <comm>            <redundant>/<faults> (<pct>%) <size> MB
 *   <pid> <comm>            <redundant>/<faults> (<pct>%) <size> KB
 */
static int parse_cow_top(const char *raw, struct proc_entry *out, int max)
{
	char buf[4096];
	char *line;
	int n = 0;

	if (!raw || !raw[0] || strstr(raw, "(no redundant"))
		return 0;

	strncpy(buf, raw, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	line = strtok(buf, "\n");
	while (line && n < max) {
		struct proc_entry *e = &out[n];
		char unit[8] = {0};
		double size = 0;

		/*
		 * Parse: "<pid> <comm> <redundant>/<faults> (<pct>%) <size> <unit>"
		 * The comm field is left-justified with trailing spaces.
		 */
		if (sscanf(line, "%d %63s %llu/%llu (%llu%%) %lf %7s",
			   &e->pid, e->comm, &e->redundant,
			   &e->faults, &e->pct, &size, unit) >= 5) {
			if (strstr(unit, "GB"))
				e->bytes_wasted = size * (1ULL << 30);
			else if (strstr(unit, "MB"))
				e->bytes_wasted = size * (1ULL << 20);
			else if (strstr(unit, "KB"))
				e->bytes_wasted = size * (1ULL << 10);
			else
				e->bytes_wasted = size;

			format_bytes((unsigned long long)e->bytes_wasted,
				     e->wasted_str, sizeof(e->wasted_str));
			n++;
		}
		line = strtok(NULL, "\n");
	}
	return n;
}

/* ── Parse cgroup_top sysfs output ───────────────────────────── */
/*
 * Format from kernel:
 *   <name>                                  COW: <n> faults (<size> MB)  NET: <n> sends (<size> MB)
 */
static int parse_cgroup_top(const char *raw, struct cgroup_entry *out, int max)
{
	char buf[4096];
	char *line;
	int n = 0;

	if (!raw || !raw[0] || strstr(raw, "(no container"))
		return 0;

	strncpy(buf, raw, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	line = strtok(buf, "\n");
	while (line && n < max) {
		struct cgroup_entry *e = &out[n];
		char *cow_p, *net_p;

		cow_p = strstr(line, "COW:");
		net_p = strstr(line, "NET:");
		if (!cow_p)
			goto next;

		/* Extract name (everything before COW:) */
		{
			int namelen = (int)(cow_p - line);
			while (namelen > 0 && line[namelen - 1] == ' ')
				namelen--;
			if (namelen >= (int)sizeof(e->name))
				namelen = (int)sizeof(e->name) - 1;
			memcpy(e->name, line, namelen);
			e->name[namelen] = '\0';
		}

		/* Parse COW portion */
		{
			double cow_size = 0;
			char cow_unit[8] = {0};

			sscanf(cow_p, "COW: %llu faults (%lf %7[^)])",
			       &e->cow_faults, &cow_size, cow_unit);
			if (strstr(cow_unit, "GB"))
				e->cow_waste = cow_size * (1ULL << 30);
			else if (strstr(cow_unit, "MB"))
				e->cow_waste = cow_size * (1ULL << 20);
			else
				e->cow_waste = cow_size * (1ULL << 10);
			format_bytes((unsigned long long)e->cow_waste,
				     e->cow_str, sizeof(e->cow_str));
		}

		/* Parse NET portion */
		if (net_p) {
			double net_size = 0;
			char net_unit[8] = {0};

			sscanf(net_p, "NET: %llu sends (%lf %7[^)])",
			       &e->net_sends, &net_size, net_unit);
			if (strstr(net_unit, "GB"))
				e->net_waste = net_size * (1ULL << 30);
			else if (strstr(net_unit, "MB"))
				e->net_waste = net_size * (1ULL << 20);
			else
				e->net_waste = net_size * (1ULL << 10);
			format_bytes((unsigned long long)e->net_waste,
				     e->net_str, sizeof(e->net_str));
		}

		e->total = e->cow_waste + e->net_waste;
		n++;
next:
		line = strtok(NULL, "\n");
	}
	return n;
}

/* ── Read kernel version ─────────────────────────────────────── */

static void read_kernel_version(char *out, size_t len)
{
	char buf[512];

	if (read_file(PROC_VERSION, buf, sizeof(buf)) == 0) {
		/* "Linux version X.Y.Z-... (gcc ...) ..." */
		char *p = strstr(buf, "version ");
		if (p) {
			char *end;

			p += 8;
			end = strchr(p, ' ');
			if (end) {
				size_t n = (size_t)(end - p);
				if (n >= len)
					n = len - 1;
				memcpy(out, p, n);
				out[n] = '\0';
				return;
			}
		}
	}
	snprintf(out, len, "unknown");
}

/* ── Gather all report data ──────────────────────────────────── */

static int gather_data(struct report_data *d)
{
	char proc_buf[4096];
	char cow_buf[4096];
	char net_buf[4096];
	char cow_top_buf[4096];
	char cg_buf[4096];
	char uptime_buf[64];
	time_t now;
	struct tm *tm;

	memset(d, 0, sizeof(*d));

	/* Timestamp */
	now = time(NULL);
	tm = gmtime(&now);
	strftime(d->timestamp, sizeof(d->timestamp),
		 "%Y-%m-%d %H:%M:%S UTC", tm);

	/* Check if module is loaded */
	if (read_file(PROC_ATOMIK, proc_buf, sizeof(proc_buf)) < 0)
		return -1;

	/* Parse /proc/atomik key=value pairs */
	kv_get_str(proc_buf, "version", d->version, sizeof(d->version));
	kv_get_str(proc_buf, "backend", d->backend, sizeof(d->backend));
	kv_get_str(proc_buf, "license", d->license, sizeof(d->license));

	d->ops_load  = kv_get_u64(proc_buf, "ops_load");
	d->ops_accum = kv_get_u64(proc_buf, "ops_accum");
	d->ops_read  = kv_get_u64(proc_buf, "ops_read");
	d->ops_swap  = kv_get_u64(proc_buf, "ops_swap");
	d->ops_total = kv_get_u64(proc_buf, "ops_total");

	d->fp_checks   = kv_get_u64(proc_buf, "fp_checks");
	d->fp_unchanged = kv_get_u64(proc_buf, "fp_unchanged");
	d->fp_bytes    = kv_get_u64(proc_buf, "fp_bytes");

	d->cow_faults      = kv_get_u64(proc_buf, "cow_faults");
	d->cow_redundant   = kv_get_u64(proc_buf, "cow_redundant");
	d->cow_bytes_copied = kv_get_u64(proc_buf, "cow_bytes_copied");
	d->cow_bytes_saved = kv_get_u64(proc_buf, "cow_bytes_saved");

	d->net_sends          = kv_get_u64(proc_buf, "net_sends");
	d->net_redundant      = kv_get_u64(proc_buf, "net_redundant");
	d->net_bytes          = kv_get_u64(proc_buf, "net_bytes");
	d->net_bytes_redundant = kv_get_u64(proc_buf, "net_bytes_redundant");

	/* Read /proc/atomik/cow for redundancy rate */
	if (read_file(PROC_ATOMIK_COW, cow_buf, sizeof(cow_buf)) == 0) {
		kv_get_str(cow_buf, "redundancy_rate",
			   d->cow_redundancy_rate,
			   sizeof(d->cow_redundancy_rate));
		d->cow_copies = kv_get_u64(cow_buf, "copies");
	}

	/* Read /proc/atomik/net for redundancy rate */
	if (read_file(PROC_ATOMIK_NET, net_buf, sizeof(net_buf)) == 0) {
		kv_get_str(net_buf, "redundancy_rate",
			   d->net_redundancy_rate,
			   sizeof(d->net_redundancy_rate));
	}

	/* Top processes from sysfs */
	if (read_sysfs("cow_top", cow_top_buf,
			sizeof(cow_top_buf)) == 0) {
		d->nprocs = parse_cow_top(cow_top_buf, d->procs, MAX_PROCS);
	}

	/* Cgroup breakdown from sysfs */
	if (read_sysfs("cgroup_top", cg_buf, sizeof(cg_buf)) == 0) {
		d->ncgroups = parse_cgroup_top(cg_buf, d->cgroups,
					       MAX_CGROUPS);
	}

	/* License details from sysfs */
	if (d->license[0] == '\0')
		read_sysfs("license_status", d->license, sizeof(d->license));
	d->license_days = (int)read_sysfs_u64("license_days_remaining");

	/* System info */
	read_kernel_version(d->kernel, sizeof(d->kernel));

	/* Uptime */
	if (read_file(PROC_UPTIME, uptime_buf, sizeof(uptime_buf)) == 0)
		d->uptime_sec = strtod(uptime_buf, NULL);

	return 0;
}

/* ── Text report output ──────────────────────────────────────── */

#define BOX_W 62

static void print_hline(void)
{
	int i;

	printf("  ");
	for (i = 0; i < BOX_W; i++)
		putchar('=');
	putchar('\n');
}

static void print_section(const char *title)
{
	int tlen = (int)strlen(title);
	int pad = BOX_W - tlen - 3;
	int i;

	printf("\n");
	printf("  ");
	putchar(0xE2); putchar(0x94); putchar(0x80);   /* ─ */
	putchar(0xE2); putchar(0x94); putchar(0x80);   /* ─ */
	printf(" %s ", title);
	for (i = 0; i < pad; i++) {
		putchar(0xE2); putchar(0x94); putchar(0x80); /* ─ */
	}
	putchar('\n');
}

static void output_text(const struct report_data *d)
{
	char nbuf[32], bytebuf[32];
	char uptimebuf[64];
	int i;

	/* Header box */
	printf("\n");
	printf("  ");
	putchar(0xE2); putchar(0x95); putchar(0x94); /* ╔ */
	for (i = 0; i < BOX_W; i++) {
		putchar(0xE2); putchar(0x95); putchar(0x90); /* ═ */
	}
	putchar(0xE2); putchar(0x95); putchar(0x97); /* ╗ */
	putchar('\n');

	/* Title line */
	{
		const char *title = "ATOMiK System Waste Analysis Report";
		int tlen = (int)strlen(title);
		int lpad = (BOX_W - tlen) / 2;
		int rpad = BOX_W - tlen - lpad;

		printf("  ");
		putchar(0xE2); putchar(0x95); putchar(0x91); /* ║ */
		printf("%*s%s%*s", lpad, "", title, rpad, "");
		putchar(0xE2); putchar(0x95); putchar(0x91); /* ║ */
		putchar('\n');
	}

	/* Timestamp line */
	{
		char tsbuf[80];
		int tlen, lpad, rpad;

		snprintf(tsbuf, sizeof(tsbuf), "Generated: %s", d->timestamp);
		tlen = (int)strlen(tsbuf);
		lpad = (BOX_W - tlen) / 2;
		rpad = BOX_W - tlen - lpad;

		printf("  ");
		putchar(0xE2); putchar(0x95); putchar(0x91); /* ║ */
		printf("%*s%s%*s", lpad, "", tsbuf, rpad, "");
		putchar(0xE2); putchar(0x95); putchar(0x91); /* ║ */
		putchar('\n');
	}

	/* Bottom of box */
	printf("  ");
	putchar(0xE2); putchar(0x95); putchar(0x9A); /* ╚ */
	for (i = 0; i < BOX_W; i++) {
		putchar(0xE2); putchar(0x95); putchar(0x90); /* ═ */
	}
	putchar(0xE2); putchar(0x95); putchar(0x9D); /* ╝ */
	putchar('\n');

	/* System Information */
	printf("\n  System Information\n");
	printf("    Kernel:     %s\n", d->kernel);
	printf("    ATOMiK:     v%s (%s backend)\n",
	       d->version, d->backend);

	format_uptime(d->uptime_sec, uptimebuf, sizeof(uptimebuf));
	printf("    Uptime:     %s\n", uptimebuf);
	printf("    License:    %s\n", d->license);

	/* COW Analysis */
	print_section("Copy-on-Write Analysis");

	if (d->cow_faults > 0) {
		format_number(d->cow_faults, nbuf, sizeof(nbuf));
		printf("    Total COW faults:     %s\n", nbuf);

		format_number(d->cow_redundant, nbuf, sizeof(nbuf));
		if (d->cow_redundancy_rate[0])
			printf("    Redundant copies:     %s (%s)\n",
			       nbuf, d->cow_redundancy_rate);
		else
			printf("    Redundant copies:     %s\n", nbuf);

		format_bytes(d->cow_bytes_saved, bytebuf, sizeof(bytebuf));
		printf("    Bytes wasted:         %s\n", bytebuf);

		if (d->nprocs > 0) {
			printf("\n    Top processes by waste:\n");
			for (i = 0; i < d->nprocs; i++) {
				format_number(d->procs[i].faults, nbuf,
					      sizeof(nbuf));
				printf("      PID %-6d %-16s %6s faults  "
				       "(%s wasted)\n",
				       d->procs[i].pid,
				       d->procs[i].comm,
				       nbuf,
				       d->procs[i].wasted_str);
			}
		}
	} else {
		printf("    No COW faults detected yet.\n");
	}

	/* Network Analysis */
	print_section("Network Send Analysis");

	if (d->net_sends > 0) {
		format_number(d->net_sends, nbuf, sizeof(nbuf));
		printf("    Total TCP sends:      %s\n", nbuf);

		format_number(d->net_redundant, nbuf, sizeof(nbuf));
		if (d->net_redundancy_rate[0])
			printf("    Redundant sends:      %s (%s)\n",
			       nbuf, d->net_redundancy_rate);
		else
			printf("    Redundant sends:      %s\n", nbuf);

		format_bytes(d->net_bytes_redundant, bytebuf,
			     sizeof(bytebuf));
		printf("    Bytes redundant:      %s\n", bytebuf);
	} else {
		printf("    No TCP sends tracked yet.\n");
	}

	/* Container Attribution */
	if (d->ncgroups > 0) {
		char total_buf[32];

		print_section("Container Attribution");
		printf("    %-24s %-12s %-12s %s\n",
		       "Container", "COW Waste", "Net Waste", "Total");
		printf("    %-24s %-12s %-12s %s\n",
		       "------------------------",
		       "----------", "----------", "----------");

		for (i = 0; i < d->ncgroups; i++) {
			format_bytes((unsigned long long)d->cgroups[i].total,
				     total_buf, sizeof(total_buf));
			printf("    %-24s %-12s %-12s %s\n",
			       d->cgroups[i].name,
			       d->cgroups[i].cow_str,
			       d->cgroups[i].net_str,
			       total_buf);
		}
	}

	/* Optimization Opportunities */
	{
		double total_waste = (double)(d->cow_bytes_saved +
					     d->net_bytes_redundant);
		double monthly;

		print_section("Optimization Opportunities");

		if (d->uptime_sec > 60 && total_waste > 0) {
			monthly = total_waste / d->uptime_sec *
				  86400.0 * 30.0;
			format_bytes((unsigned long long)monthly, bytebuf,
				     sizeof(bytebuf));
			printf("    Monthly waste projection:  ~%s "
			       "(at current rate)\n", bytebuf);
		}

		if (strcmp(d->license, "active") != 0) {
			printf("    Potential savings with Pro: "
			       "Up to 90%% reduction\n");
			printf("\n");
			printf("    %c Learn more at "
			       "https://atomik.tech/pricing\n",
			       0x2D);  /* dash */
		} else {
			printf("    ATOMiK Pro is actively optimizing "
			       "your system.\n");
		}
	}

	/* Footer */
	printf("\n");
	print_hline();
	printf("\n");
}

/* ── JSON output ─────────────────────────────────────────────── */

static void json_str(const char *key, const char *val, int comma)
{
	/* Escape quotes in val */
	printf("    \"%s\": \"", key);
	while (*val) {
		if (*val == '"' || *val == '\\')
			putchar('\\');
		putchar(*val);
		val++;
	}
	printf("\"%s\n", comma ? "," : "");
}

static void json_dbl(const char *key, double val, int comma)
{
	printf("    \"%s\": %.2f%s\n", key, val, comma ? "," : "");
}

static void output_json(const struct report_data *d)
{
	int i;

	printf("{\n");
	json_str("timestamp", d->timestamp, 1);
	json_str("version", d->version, 1);
	json_str("backend", d->backend, 1);
	json_str("license", d->license, 1);
	json_str("kernel", d->kernel, 1);
	json_dbl("uptime_seconds", d->uptime_sec, 1);

	/* Operations */
	printf("    \"operations\": {\n");
	printf("        \"total\": %llu,\n", d->ops_total);
	printf("        \"load\": %llu,\n", d->ops_load);
	printf("        \"accum\": %llu,\n", d->ops_accum);
	printf("        \"read\": %llu,\n", d->ops_read);
	printf("        \"swap\": %llu\n", d->ops_swap);
	printf("    },\n");

	/* COW */
	printf("    \"cow\": {\n");
	printf("        \"faults\": %llu,\n", d->cow_faults);
	printf("        \"copies\": %llu,\n", d->cow_copies);
	printf("        \"redundant\": %llu,\n", d->cow_redundant);
	printf("        \"bytes_copied\": %llu,\n", d->cow_bytes_copied);
	printf("        \"bytes_saved\": %llu,\n", d->cow_bytes_saved);
	printf("        \"redundancy_rate\": \"%s\",\n",
	       d->cow_redundancy_rate);

	/* Top processes */
	printf("        \"top_processes\": [\n");
	for (i = 0; i < d->nprocs; i++) {
		printf("            {\"pid\": %d, \"comm\": \"%s\", "
		       "\"redundant\": %llu, \"faults\": %llu, "
		       "\"pct\": %llu, \"bytes_wasted\": %.0f}%s\n",
		       d->procs[i].pid, d->procs[i].comm,
		       d->procs[i].redundant, d->procs[i].faults,
		       d->procs[i].pct, d->procs[i].bytes_wasted,
		       i < d->nprocs - 1 ? "," : "");
	}
	printf("        ]\n");
	printf("    },\n");

	/* Network */
	printf("    \"network\": {\n");
	printf("        \"sends\": %llu,\n", d->net_sends);
	printf("        \"redundant\": %llu,\n", d->net_redundant);
	printf("        \"bytes_total\": %llu,\n", d->net_bytes);
	printf("        \"bytes_redundant\": %llu,\n",
	       d->net_bytes_redundant);
	printf("        \"redundancy_rate\": \"%s\"\n",
	       d->net_redundancy_rate);
	printf("    },\n");

	/* Cgroups */
	printf("    \"containers\": [\n");
	for (i = 0; i < d->ncgroups; i++) {
		printf("        {\"name\": \"%s\", "
		       "\"cow_faults\": %llu, \"cow_waste\": %.0f, "
		       "\"net_sends\": %llu, \"net_waste\": %.0f, "
		       "\"total_waste\": %.0f}%s\n",
		       d->cgroups[i].name,
		       d->cgroups[i].cow_faults, d->cgroups[i].cow_waste,
		       d->cgroups[i].net_sends, d->cgroups[i].net_waste,
		       d->cgroups[i].total,
		       i < d->ncgroups - 1 ? "," : "");
	}
	printf("    ],\n");

	/* Projections */
	{
		double total_waste = (double)(d->cow_bytes_saved +
					     d->net_bytes_redundant);
		double monthly = 0;

		if (d->uptime_sec > 60)
			monthly = total_waste / d->uptime_sec *
				  86400.0 * 30.0;
		json_dbl("total_waste_bytes", total_waste, 1);
		json_dbl("monthly_waste_projection_bytes", monthly, 0);
	}

	printf("}\n");
}

/* ── CSV output ──────────────────────────────────────────────── */

static void output_csv(const struct report_data *d)
{
	int i;

	/* Summary row */
	printf("section,key,value\n");
	printf("system,timestamp,%s\n", d->timestamp);
	printf("system,kernel,%s\n", d->kernel);
	printf("system,version,%s\n", d->version);
	printf("system,backend,%s\n", d->backend);
	printf("system,license,%s\n", d->license);
	printf("system,uptime_seconds,%.0f\n", d->uptime_sec);

	printf("cow,faults,%llu\n", d->cow_faults);
	printf("cow,copies,%llu\n", d->cow_copies);
	printf("cow,redundant,%llu\n", d->cow_redundant);
	printf("cow,bytes_copied,%llu\n", d->cow_bytes_copied);
	printf("cow,bytes_saved,%llu\n", d->cow_bytes_saved);
	printf("cow,redundancy_rate,%s\n", d->cow_redundancy_rate);

	printf("net,sends,%llu\n", d->net_sends);
	printf("net,redundant,%llu\n", d->net_redundant);
	printf("net,bytes_total,%llu\n", d->net_bytes);
	printf("net,bytes_redundant,%llu\n", d->net_bytes_redundant);
	printf("net,redundancy_rate,%s\n", d->net_redundancy_rate);

	/* Per-process rows */
	for (i = 0; i < d->nprocs; i++)
		printf("cow_process,%d %s,%llu/%llu (%.0f bytes wasted)\n",
		       d->procs[i].pid, d->procs[i].comm,
		       d->procs[i].redundant, d->procs[i].faults,
		       d->procs[i].bytes_wasted);

	/* Per-cgroup rows */
	for (i = 0; i < d->ncgroups; i++)
		printf("container,%s,cow=%.0f net=%.0f total=%.0f\n",
		       d->cgroups[i].name,
		       d->cgroups[i].cow_waste,
		       d->cgroups[i].net_waste,
		       d->cgroups[i].total);
}

/* ── Brief output ────────────────────────────────────────────── */

static void output_brief(const struct report_data *d)
{
	char cow_str[32], net_str[32], total_str[32];
	unsigned long long total;

	total = d->cow_bytes_saved + d->net_bytes_redundant;
	format_bytes(d->cow_bytes_saved, cow_str, sizeof(cow_str));
	format_bytes(d->net_bytes_redundant, net_str, sizeof(net_str));
	format_bytes(total, total_str, sizeof(total_str));

	printf("ATOMiK v%s | COW waste: %s (%llu redundant/%llu faults) | "
	       "Net waste: %s (%llu redundant/%llu sends) | "
	       "Total: %s\n",
	       d->version,
	       cow_str, d->cow_redundant, d->cow_faults,
	       net_str, d->net_redundant, d->net_sends,
	       total_str);
}

/* ── Usage ───────────────────────────────────────────────────── */

static void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s [OPTIONS]\n", prog);
	fprintf(stderr, "\n");
	fprintf(stderr, "Generate ATOMiK system waste analysis report.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  --json     Output as JSON\n");
	fprintf(stderr, "  --csv      Output as CSV\n");
	fprintf(stderr, "  --brief    One-line summary\n");
	fprintf(stderr, "  --help     Show this help\n");
}

/* ── Main ────────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
	struct report_data data;
	enum output_fmt fmt = FMT_TEXT;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--json") == 0)
			fmt = FMT_JSON;
		else if (strcmp(argv[i], "--csv") == 0)
			fmt = FMT_CSV;
		else if (strcmp(argv[i], "--brief") == 0)
			fmt = FMT_BRIEF;
		else if (strcmp(argv[i], "--help") == 0 ||
			 strcmp(argv[i], "-h") == 0) {
			usage(argv[0]);
			return 0;
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			usage(argv[0]);
			return 2;
		}
	}

	if (gather_data(&data) < 0) {
		fprintf(stderr,
			"ATOMiK kernel module not loaded\n");
		return 1;
	}

	switch (fmt) {
	case FMT_JSON:
		output_json(&data);
		break;
	case FMT_CSV:
		output_csv(&data);
		break;
	case FMT_BRIEF:
		output_brief(&data);
		break;
	case FMT_TEXT:
	default:
		output_text(&data);
		break;
	}

	return 0;
}
