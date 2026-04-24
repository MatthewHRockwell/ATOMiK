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

Every artifact below has the full chain: source commit → exact build command → binary MD5 → result MD5.

### workload_csr_20260409.txt
- **Source commit**: 63bc66e
- **Binary**: `workload_change_detect`
- **Source**: `hardware/zynq/test/workload_change_detect.c` + `software/libatomik/libatomik.c`
- **Build**: `riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=c99 -I. -static -o build/workload_change_detect ../../hardware/zynq/test/workload_change_detect.c libatomik.c -I.` (from `software/libatomik/`)
- **Binary MD5**: 32872774aef19ac2eb11aa7bb1a2b7eb
- **Result MD5**: 902df8e56a7992bf64d1ad101189d64d

### demo_state_monitor_20260409.txt
- **Source commit**: 63bc66e
- **Binary**: `demo_state_monitor`
- **Source**: `software/libatomik/demo_state_monitor.c` + `software/libatomik/libatomik.c`
- **Build**: `riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=c99 -I. -static -o build/demo_state_monitor demo_state_monitor.c libatomik.c -I.` (from `software/libatomik/`)
- **Binary MD5**: 2f8fdd3fa06e1d360efe38c6a9d44ff8
- **Result MD5**: ab5d666579628bff6c32f58ed9317795

### watchd_20260409.txt
- **Source commit**: 3a9237d
- **Binary**: `atomik-watchd`
- **Source**: `software/atomik-watchd/atomik-watchd.c` + `software/libatomik/libatomik.c`
- **Build**: `riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=gnu99 -I../libatomik -static -o build/atomik-watchd atomik-watchd.c ../libatomik/libatomik.c` (from `software/atomik-watchd/`)
- **Binary MD5**: 354250c42760bb162f4a1e08d9d82776
- **Result MD5**: b08e9acfe7cd7103f1b8e136a35b332d

### multiwedge_20260409.txt
- **Source commit**: 92d8fd0
- **Binaries**:
  - `atomik-watchd` — source: `software/atomik-watchd/atomik-watchd.c` — MD5: 354250c42760bb162f4a1e08d9d82776
  - `atomik-sync` — source: `software/atomik-sync/atomik-sync.c` — MD5: b7f092921d61efb59fdd67cda106f1d7
  - `atomik-agent-mem` — source: `software/atomik-agent-mem/atomik-agent-mem.c` — MD5: 2fc3bb6c459f6b3f42ddbb71a592ba9d
- **Build**: all via `riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=gnu99 -I../libatomik -static -o <binary> <source> ../libatomik/libatomik.c`
- **Result MD5**: 0927a439f373087d58390738e57fbfd2

### docs/demo_session_20260408.txt
- **Source commit**: c266809
- **Binary**: `demo_state_monitor` — same source as above, run via `demo_run.sh`
- **Build**: `riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=c99 -I. -static -o build/demo_state_monitor demo_state_monitor.c libatomik.c -I.` (from `software/libatomik/`)
- **Binary MD5**: not recoverable (binary rebuilt before provenance tracking)
- **Result MD5**: dd0ab2245d7653df95197374cc9a3c50
- **Note**: this is the only artifact without a binary hash. It was captured before provenance tracking began and cannot be rerun (it's a full script session capture including boot output, not just a demo result).

## Reproduction

```bash
git checkout <commit>
cd software/libatomik && make zynq CROSS=/path/to/riscv32-buildroot-linux-gnu-
cd ../atomik-watchd && make zynq CROSS=/path/to/riscv32-buildroot-linux-gnu-
cd ../atomik-sync && riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=gnu99 -I../libatomik -static -o atomik-sync atomik-sync.c ../libatomik/libatomik.c
cd ../atomik-agent-mem && riscv32-buildroot-linux-gnu-gcc -Wall -Wextra -Werror -O2 -std=gnu99 -I../libatomik -static -o atomik-agent-mem atomik-agent-mem.c ../libatomik/libatomik.c
# Inject all into rootfs, cold boot, run
```

### adapter_workload_20260409.txt
- **Source commit**: 071a358
- **Bitstream**: adapter SoC (`litex-build-adapter/gateware/hamgeek_rk7020f.bit`, MD5: e6e8436b031d8f64fab24bc4a5fa9621)
- **Binaries**:
  - `workload_change_detect` MD5: 32872774aef19ac2eb11aa7bb1a2b7eb
  - `demo_state_monitor` MD5: 2f8fdd3fa06e1d360efe38c6a9d44ff8
- **Build**: same as workload_csr_20260409.txt and demo_state_monitor_20260409.txt entries above
- **Result MD5**: 0544d2c1c24cd12b38294a28ff5fcd6f
- **Content**: three-column workload (CSR + adapter + SW) + demo_state_monitor on adapter path (7/7 YES)

---

## Phase 9 Display Artifacts (April 2026)

Phase 9 upgraded from VexRiscv SMP (RV32) to NaxRiscv (RV64GC) and added dual-display output.
Full proof note: [`PHASE_9_PROOF.md`](PHASE_9_PROOF.md)

### Build Environment (Phase 9)

| Component | Version |
|-----------|---------|
| Cross-compiler | riscv64-linux-gnu-gcc 13.3.0 (Ubuntu 24.04) |
| Vivado | v2025.2 |
| NaxRiscv netlist | NaxRiscvLitex_3c064e59e555b5c0eeae3421918289b5 |
| Linux kernel | 6.9.0 (rv64imafdc) |
| OpenSBI | fw_jump (rv64, litex-hub fork) |
| Rootfs | Ubuntu 24.04 (riscv64, LP64D) |

### Bitstream

- **Source commit**: 9f4eff2
- **SoC definition**: `hardware/zynq/litex/soc_nax64_atomik.py` — MD5: 5e35f9aaba52285a08f22b2575ef8b97
- **Build**: `python3 soc_nax64_atomik.py --build --uart-baudrate=921600 --output-dir=../litex-build-nax64 --with-video-framebuffer-hp`
- **Bitstream MD5**: ea016e0fa510c07009ebfbdab83c755e
- **Timing**: WNS +0.196 ns, TNS 0.000

### Boot Files

| File | MD5 | Size | Origin |
|------|-----|------|--------|
| Image_nax64 | 8b2ee067d9230b4b8707bc7541c8969c | 8,231,096 | buildroot (rv64imafdc) |
| linux_nax64.dtb | e5f44e2b8e5aa545bb43099aa97ed2f2 | 2,759 | dtc from linux_nax64.dts |
| fw_jump_nax64.bin | 6f3a60d78a03cc82574805e440c1a21d | 133,632 | buildroot OpenSBI |
| ubuntu_rv64.cpio.gz | d0329cabba7ec64ccfe0dcd184dcb6d6 | 33,932,917 | debootstrap riscv64 |
| trampoline.bin | d8ecb4cef45f784942321cd5226b10fa | 36 | make -C ps_loader trampoline |

### Display Programs

All built from source at commit 9f4eff2. Cross-compiler: `riscv64-linux-gnu-gcc`.

#### fb_test (Phase 9.2 — HDMI framebuffer)
- **Source**: `ps_loader/fb_test.c` — MD5: 2016f301b8a9780ec8ac763f02ce2547
- **Build**: `riscv64-linux-gnu-gcc -O2 -static fb_test.c -o fb_test`
- **Result**: 1920x1080@30Hz solid color fills on Dell 3440x1440 ultrawide

#### atomik_hdmi_viz (Phase 9.4 — ATOMiK visualization)
- **Source**: `ps_loader/atomik_hdmi_viz.c` — MD5: af27c535991d6c418f57866c2cf037c5
- **Build**: `riscv64-linux-gnu-gcc -O2 -static atomik_hdmi_viz.c -o atomik_hdmi_viz`
- **Result**: live delta-state operations rendered on HDMI framebuffer

#### atomik_splash (Phase 9.5 — HDMI splash)
- **Source**: `ps_loader/atomik_splash.c` — MD5: 8077496788f4ab1c151735630cd6ba61
- **Build**: `riscv64-linux-gnu-gcc -O2 -static atomik_splash.c -o atomik_splash`
- **Result**: ATOMiK branded splash on 1080p HDMI

#### lcd_tiny (Phase 9.6 — SPI LCD splash)
- **Source**: `ps_loader/lcd_tiny.c` — MD5: b363efb00aa57633adfd0d9c04d43dd6
- **Build**: `riscv64-linux-gnu-gcc -Os -nostdlib -static -fno-builtin -o lcd_tiny lcd_tiny.c && riscv64-linux-gnu-strip lcd_tiny`
- **Binary MD5**: 7d07c5c2e243b90157ab0e8f41f4e214
- **Binary size**: 2,400 bytes
- **Result**: ATOMiK splash on ST7789V 320x172 LCD (dark background + blue accent bars)
- **LCD pins**: SDA=U19, SCL=V18, DC=W13, CS=AA13, RST=AA18, LED=Y13 (Bank 33, from RK schematic)

### JTAG Boot (Phase 9.1)

- **Source**: `ps_loader/jtag_boot.py` (committed at 58c3bb7)
- **Trampoline**: `ps_loader/trampoline.S` (committed at 58c3bb7)
- **Result**: 95-102s from xsdb connect to root shell (measured across 4+ runs)
- **Reproduction**: `BITSTREAM=../litex-build-nax64/gateware/hamgeek_rk7020f.bit python3 jtag_boot.py`
