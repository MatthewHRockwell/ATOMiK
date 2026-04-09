# Result Artifact Provenance

All results captured on HamGeek RK-ZYNQ7020-F (XC7Z020CLG484-2).
Bitstream: April 5th proven SoC (`zynq-adapter-v1` tag baseline).

## Build Environment

| Component | Version |
|-----------|---------|
| Cross-compiler | riscv32-buildroot-linux-gnu-gcc 11.4.0 (Buildroot 2023.02.5) |
| Host compiler | gcc 13.3.0 (Ubuntu 24.04) |
| Vivado | v2025.2 |
| LiteX | 498d1fe82 |
| Linux kernel | 6.9.0 (rv32ima) |
| OpenSBI | 1.3.1 (litex-hub fork) |
| libatomik | 1.0.0 |

## Artifacts

### workload_csr_20260408.txt
- **Source commit**: cc9307e (tag: zynq-adapter-v1)
- **Binary**: `workload_change_detect`
- **Source**: `hardware/zynq/test/workload_change_detect.c` + `software/libatomik/libatomik.c`
- **Build**: `riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=c99 -I. -static -o build/workload_change_detect ../../hardware/zynq/test/workload_change_detect.c libatomik.c -I.` (run from `software/libatomik/`)
- **Binary MD5**: not captured (binary was rebuilt before provenance tracking began)
- **Result MD5**: 74af9da64652cebf1794290924026dd4

### demo_state_monitor_20260408.txt
- **Source commit**: cc9307e (tag: zynq-adapter-v1)
- **Binary**: `demo_state_monitor`
- **Source**: `software/libatomik/demo_state_monitor.c` + `software/libatomik/libatomik.c`
- **Build**: `riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=c99 -I. -static -o build/demo_state_monitor demo_state_monitor.c libatomik.c -I.` (run from `software/libatomik/`)
- **Binary MD5**: not captured (binary was rebuilt before provenance tracking began)
- **Result MD5**: 948408cea1eb287a29d25af781959a38

### demo_20260408_204230.txt
- **Source commit**: c266809
- **Binary**: `demo_state_monitor` (same source as above, run via `demo_run.sh`)
- **Source**: `software/libatomik/demo_state_monitor.c` + `software/libatomik/libatomik.c`
- **Build**: `riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=c99 -I. -static -o build/demo_state_monitor demo_state_monitor.c libatomik.c -I.` (run from `software/libatomik/`)
- **Binary MD5**: not captured (binary was rebuilt before provenance tracking began)
- **Result MD5**: 94f6f98aaa6ad3d49a0ceea7fdd3a858

### watchd_20260409.txt
- **Source commit**: 3a9237d
- **Binary**: `atomik-watchd`
- **Source**: `software/atomik-watchd/atomik-watchd.c` + `software/libatomik/libatomik.c`
- **Build**: `riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=gnu99 -I../libatomik -static -o build/atomik-watchd atomik-watchd.c ../libatomik/libatomik.c` (run from `software/atomik-watchd/`)
- **Binary MD5**: 354250c42760bb162f4a1e08d9d82776
- **Result MD5**: b08e9acfe7cd7103f1b8e136a35b332d

### multiwedge_20260409.txt
- **Source commit**: 92d8fd0
- **Binaries**:
  - `atomik-watchd` — source: `software/atomik-watchd/atomik-watchd.c` + `libatomik.c` — MD5: 354250c42760bb162f4a1e08d9d82776
  - `atomik-sync` — source: `software/atomik-sync/atomik-sync.c` + `libatomik.c` — MD5: b7f092921d61efb59fdd67cda106f1d7
  - `atomik-agent-mem` — source: `software/atomik-agent-mem/atomik-agent-mem.c` + `libatomik.c` — MD5: 2fc3bb6c459f6b3f42ddbb71a592ba9d
- **Build**: all via `riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=gnu99 -I../libatomik -static -o <binary> <source> ../libatomik/libatomik.c`
- **Result MD5**: 0927a439f373087d58390738e57fbfd2

### docs/demo_session_20260408.txt
- **Source commit**: c266809
- **Binary**: `demo_state_monitor` (captured as part of `demo_run.sh` full session)
- **Source**: `software/libatomik/demo_state_monitor.c` + `software/libatomik/libatomik.c`
- **Build**: `riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=c99 -I. -static -o build/demo_state_monitor demo_state_monitor.c libatomik.c -I.` (run from `software/libatomik/`)
- **Binary MD5**: not captured (binary was rebuilt before provenance tracking began)
- **Result MD5**: dd0ab2245d7653df95197374cc9a3c50

## Provenance Gaps

The April 8 artifacts (workload, demo_state_monitor, demo_session) were built and run before provenance tracking was established. The exact binary MD5s for those runs are not recoverable — the binaries were rebuilt multiple times during that session. The source commits, build commands, and result MD5s are accurate.

The April 9 artifacts (watchd, multiwedge) have full provenance including binary MD5s verified against the workspace.

## Reproduction

```bash
git checkout <commit>
cd software/libatomik && make zynq CROSS=/path/to/riscv32-buildroot-linux-gnu-
cd ../atomik-watchd && make zynq CROSS=/path/to/riscv32-buildroot-linux-gnu-
cd ../atomik-sync && riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=gnu99 -I../libatomik -static -o atomik-sync atomik-sync.c ../libatomik/libatomik.c
cd ../atomik-agent-mem && riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=gnu99 -I../libatomik -static -o atomik-agent-mem atomik-agent-mem.c ../libatomik/libatomik.c
# Inject all into rootfs, cold boot, run
```
