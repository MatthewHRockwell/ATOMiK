# SD Boot Validation Note Template

Status: template only. Do not treat this as validation evidence until completed from a real board run.

## Required current status language
We are working on booting from SD card instead of relying on JTAG so workload updates can be made and rerun faster. After SD boot is stable, the first validation workloads will be integrated and measured.

## Board and build details
- Board model:
- Serial / board identifier, if safe to record:
- BOOT.BIN source:
- Bitstream used:
- Linux image used:
- Root filesystem source:
- Kernel version:
- Device tree source:
- ATOMiK hardware core version:
- Git commit hash:
- JTAG fallback available: yes/no

## Boot reproduction steps
1. Prepare SD card:
2. Copy boot artifacts:
3. Insert SD card:
4. Set boot mode:
5. Capture serial/log output:
6. Confirm userspace shell:
7. Confirm ATOMiK hardware register or test-interface reachability:
8. Run LOAD / ACCUM / READ / SWAP smoke test or current equivalent validation command:

## Minimum boot success criteria
- Board boots from SD card without JTAG.
- Userspace shell is available.
- ATOMiK hardware registers or test interface are reachable.
- Minimal LOAD / ACCUM / READ / SWAP smoke test can run, or the current equivalent hardware validation command can run.
- Logs are captured and linked.

## Logs and artifacts
- Serial log:
- Boot log:
- Build manifest:
- Smoke-test output:
- Screenshot/photo, if used:

## Caveats
- This note validates boot and reachability only.
- This note does not prove workload performance, customer value, battery improvement, heat reduction, cooling reduction, water savings, smaller hardware, production readiness, or universal speedup.
