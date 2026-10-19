# Witnessed Live Run — 2026-06-28

A REAL capture from the AX7020, produced by `contract_bench --live-run` driving
`abench` over the board's serial console (live NaxRiscv RV64 Linux). This is the
first end-to-end validation that the `run_live()` harness works on hardware.

- `witnessed_run.md` / `.json` — the LiveRunRecord (measured ladder + result hash
  + verbatim abench transcript).
- `conditional_loi_witnessed.md` — a sample Conditional LOI with the **live**
  witnessed run embedded (Source: "live (on-call)").

Result: 0x17e9fe29b5dc0aad byte-identical across 1/2/4/8 banks, 800 M/s at 8 banks
— reproduces the recorded reference exactly, live. Reproduce: `abench 65537 0xDEADBEEF0BADF00D`.

To regenerate for a real prospect's spec (board up, console on ttyUSB1):
`python3 -m atomik_sdk.contract_bench.cli --required <rate> --agg <op> --name "<workload>" --loi --live-run --port /dev/ttyUSB1 --company "<Counterparty>" --drafted-on <date> --out <file>`
