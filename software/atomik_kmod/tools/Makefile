CC ?= gcc
CFLAGS := -Wall -O2

TOOLS := atomik-status atomik-test atomik-bench atomik-optimizer atomik-dashboard atomik-report

all: $(TOOLS)

atomik-status: atomik-status.c
	$(CC) $(CFLAGS) -o $@ $< -lm

atomik-test: atomik-test.c
	$(CC) $(CFLAGS) -o $@ $<

atomik-bench: atomik-bench.c
	$(CC) $(CFLAGS) -o $@ $<

atomik-optimizer: atomik-optimizer.c
	$(CC) $(CFLAGS) -o $@ $<

atomik-dashboard: atomik-dashboard.c
	$(CC) $(CFLAGS) -o $@ $<

atomik-report: atomik-report.c
	$(CC) $(CFLAGS) -o $@ $< -lm

install: $(TOOLS)
	install -m 755 atomik-status /usr/local/bin/
	install -m 755 atomik-test /usr/local/bin/
	install -m 755 atomik-bench /usr/local/bin/
	install -m 755 atomik-optimizer /usr/local/bin/
	install -m 755 atomik-dashboard /usr/local/bin/
	install -m 755 atomik-report /usr/local/bin/
	install -m 755 atomik-full-test.sh /usr/local/bin/atomik-full-test
	install -m 755 atomik-workload-bench.sh /usr/local/bin/atomik-workload-bench

clean:
	rm -f $(TOOLS)

.PHONY: all install clean
