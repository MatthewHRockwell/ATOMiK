// ATOMiK Performance Benchmark Suite
// Machine-parseable output for automated capture and analysis.
// Output format: ##PERF:test=name,key=val,...
#ifndef PERF_BENCH_H
#define PERF_BENCH_H

// Run full performance benchmark suite.
// Outputs ##PERF_START, ##META, ##PERF lines, ##PERF_END to UART.
void cmd_perf_suite(void);

#endif
