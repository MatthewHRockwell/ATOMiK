#!/bin/sh
# Re-runs the validated parallel-bank sweep every 2s, feeding the Workloads
# "Parallel Aggregate" scenario its LIVE_MEASURED throughput numbers.
[ -e /dev/mem ] || mknod /dev/mem c 1 1
while true; do
  /tmp/abench 65537 0xDEADBEEF0BADF00D > /tmp/atomik_bench_live.txt 2>&1
  sleep 2
done
