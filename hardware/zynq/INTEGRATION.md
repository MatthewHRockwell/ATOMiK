# ATOMiK Zynq Integration Status

## Hardware

- **Board**: HamGeek RK-ZYNQ7020-F (XC7Z020-2CLG484I)
- **CPU**: VexRiscv SMP rv32ima @ 100MHz (Sv32 MMU, 4KB D-cache, 4KB I-cache)
- **DDR3**: 512MB via S_AXI_GP0 (central interconnect), 0 memtest errors
- **UART**: LiteUART on V12/W12 (bank 13) → USB-UART adapter → 115200 baud

## ATOMiK Accelerator

- **Implementation**: LiteX Migen CSR module (256×64-bit state table, single bank)
- **CSR base**: 0xf0000000 (LiteX Wishbone bus)
- **Register map**:

| Offset | Name | Access | Description |
|--------|------|--------|-------------|
| 0x00 | load_addr | W | Address for LOAD (8-bit) |
| 0x04 | load_data_lo | W | Initial state [31:0] |
| 0x08 | load_data_hi | W | Initial state [63:32], triggers LOAD |
| 0x0C | accum_lo | W | Delta [31:0] |
| 0x10 | accum_hi | W | Delta [63:32], triggers ACCUM |
| 0x14 | swap_addr | W | Address, triggers SWAP |
| 0x18 | config | W | Core enable (bit 0, default=1) |
| 0x1C | state_lo | R | Current state [31:0] |
| 0x20 | state_hi | R | Current state [63:32] |
| 0x24 | status | R | version[23:16], n_banks[15:8], acc_zero[0] |

## Software Stack

- **OpenSBI**: v1.3, platform=litex/vexriscv (litex-hub fork, readl/writel for LiteUART)
- **Linux**: 4.20.17, rv32ima (pre-built from VexRiscv SMP regression data)
- **Rootfs**: BusyBox 1.30.1, glibc 2.29, modified with /dev/hvc0 + /dev/mem nodes
- **Console**: earlycon=sbi + hvc0 (SBI relay via OpenSBI → LiteUART)

## Boot Flow

1. JTAG: system reset → ps7_init → bitstream → ps7_post_config
2. Wait for LiteX BIOS prompt (~10s, includes DDR memtest)
3. JTAG: load images to PS DDR (ARM debug port, ~10s):
   - Image32 → 0x00100000 (VexRiscv 0x40000000)
   - rootfs.cpio → 0x01100000 (VexRiscv 0x41000000)
   - linux32.dtb → 0x00ff0000 (VexRiscv 0x40ef0000)
   - fw_jump.bin → 0x01000000 (VexRiscv 0x40f00000)
   - Trampoline → 0x00300000 (VexRiscv 0x40200000)
4. Flush ARM PL310 L2 cache
5. BIOS: `boot 0x40200000` → D-cache flush → trampoline → OpenSBI → Linux
6. Login prompt in ~34s from boot command

**GP0 address mapping**: VexRiscv address X → PS DDR = X - 0x40000000 + 0x00100000

## Verification Results

### ATOMiK algebra (tested from RISC-V BIOS, M-mode, 6/6 PASS)

| Test | Operation | Expected | Got | Status |
|------|-----------|----------|-----|--------|
| T1 | Status register | version=2, banks=1 | 0x00020101 | PASS |
| T2 | Initial state | 0x0000000000000000 | all zeros | PASS |
| T3 | LOAD(0, 0xCAFEBABE_DEADBEEF) | state=0xCAFEBABE_DEADBEEF | match | PASS |
| T4 | ACCUM(0x00000000_11111111) | state=0xCAFEBABE_CFBCAFFE | match | PASS |
| T5 | ACCUM(0x00000000_11111111) again | state=0xCAFEBABE_DEADBEEF | match | PASS |
| T6 | Status acc_zero | 0 (accumulator non-zero) | 0 | PASS |

The XOR self-inverse property (T5) confirms the delta-state algebra: A ⊕ B ⊕ B = A.

### Linux boot (verified)

- OpenSBI v1.3 banner ✓
- Linux 4.20.17 kernel boot ✓
- initramfs unpack (4MB BusyBox rootfs) ✓
- syslogd, klogd started ✓
- `buildroot login:` prompt reached ✓
- Root shell access ✓

## Known Limitations

Two separate issues prevent Linux userspace ATOMiK validation on the current stack:

1. **Interactive observability blocked by SBI console path**: Kernel printk works (~150 bytes/s), but user process stdout via hvc0 is extremely slow (<1 byte/s). Cause: character-at-a-time SBI ecalls on VexRiscv SMP. This prevents observing test program output interactively.

2. **Userspace MMIO validation remains inconclusive under the current kernel/debug path**: /dev/mem + mmap writes from userspace are not yet giving trustworthy physical DDR readback via JTAG. Potential contributing factors include mapping attributes, reserved-memory treatment, cacheability semantics on the GP0 path, and /dev/mem restrictions on kernel 4.20. The root cause has not been isolated.

Note: BIOS-level CSR access (M-mode, no MMU, no caching issues) works perfectly — confirming the hardware path is sound. The gap is specifically in the Linux userspace → physical memory observability chain.

## Next Steps (in priority order)

### Step 1: Build linux-on-litex Buildroot Image (kernel 6.9 + patches)

**Status**: Build attempted 2026-04-06, failed with "No space left on device" on /tmp.

**Root cause**: `/tmp` partition filled during linux-headers-6.9 tar extraction. Buildroot
at `/tmp/buildroot/` got through host toolchain (gcc, binutils, m4, bison, etc.) but died
at `linux-headers-6.9/.stamp_extracted`.

**Fix**: Move build directory from `/tmp/buildroot/` to `/home/mattrock/buildroot/` where
there is more disk space. Then rebuild at low parallelism (-j2) to avoid the 8GB RAM OOM
that killed the first attempt.

**Commands to resume**:
```bash
# 1. Free /tmp space first
rm -rf /tmp/buildroot/output/build/linux-headers-6.9/
# Or nuke and start fresh under /home:
rm -rf /tmp/buildroot

# 2. Clone buildroot under /home (more disk space)
cd /home/mattrock
git clone https://github.com/buildroot/buildroot.git --branch 2023.02.5 buildroot-litex
cd buildroot-litex

# 3. Configure with linux-on-litex external
make BR2_EXTERNAL=/tmp/linux-on-litex/buildroot litex_vexriscv_defconfig

# 4. Build at -j2 (avoid OOM on 8GB host)
make -j2

# 5. Output will be in output/images/:
#    Image      — Linux 6.9 kernel (rv32ima)
#    rootfs.cpio — BusyBox rootfs
#    fw_jump.bin — OpenSBI 1.3.1
```

**What this gives us**: The linux-on-litex Buildroot produces a known-working system with:
- Kernel 6.9 + 20 LiteX driver patches (LiteUART, LiteEth, LiteSPI, etc.)
- LiteUART console that works for userspace (interactive shell, working TX)
- OpenSBI 1.3.1 (litex-hub fork)
- BusyBox rootfs with glibc

**Prior attempt (kernel 6.1 hand-built)**: Booted successfully, earlycon works, but
LiteUART tty TX broken for userspace (irq=0, base_baud=0 at probe, no TX interrupt or
timer-based TX drain). User process stdout does not transmit. The linux-on-litex patches
are expected to fix this.

### Step 2: Boot Control Experiment

Once Buildroot images are built, boot them unchanged as a control:

```bash
# Copy images to litex-build/
cp output/images/Image ~/Projects/ATOMiK/hardware/zynq/litex-build/Image69
cp output/images/rootfs.cpio ~/Projects/ATOMiK/hardware/zynq/litex-build/rootfs69.cpio
cp output/images/fw_jump.bin ~/Projects/ATOMiK/hardware/zynq/litex-build/fw_jump69.bin

# Update boot_linux.json to point to new images
# Then boot via auto_boot_linux.py or manual JTAG
```

**Verify these four things before doing ANYTHING else**:
1. Shell is interactive (type commands, get output)
2. Userspace TX works (`echo hello` actually prints the output value)
3. `/dev/mem` or `devmem` gives trustworthy MMIO readback
4. Basic file operations work (ls, cat, etc.)

### Step 3: ATOMiK Userspace Validation

Once the control passes:

```bash
# Cross-compile atomik_test for rv32ima
riscv64-unknown-elf-gcc -march=rv32ima -mabi=ilp32 -static \
    -o atomik_test hardware/zynq/test/atomik_test_linux.c

# Add to rootfs.cpio, rebuild, reboot
# Run from Linux shell:
./atomik_test
```

The test hits ATOMiK CSRs at 0xf0000000 via /dev/mem + mmap and verifies:
- Status register (version, banks, acc_zero)
- LOAD/READ roundtrip
- ACCUM XOR delta
- Self-inverse property
- SWAP reference update

### Step 4: UIO or Platform Driver

After userspace validation via /dev/mem, build a proper UIO driver:
- Device tree overlay for `atomik,delta-engine` compatible
- UIO interrupt handling for mailbox doorbell
- Clean mmap interface (no /dev/mem needed)

### Step 5: 64-bit RISC-V Upgrade (VexiiRiscv RV64IMAFDC)

Final target: 64-bit RISC-V running Ubuntu with native ATOMiK ISA extension.

## Files

| File | Description |
|------|-------------|
| `litex-build/build/gateware/hamgeek_rk7020f.bit` | FPGA bitstream |
| `litex-build/Image32` | Linux kernel (rv32ima, 2.7MB) |
| `litex-build/rootfs.cpio` | Modified BusyBox rootfs (3.9MB) |
| `litex-build/linux32.dtb` | Device tree with initrd params |
| `litex-build/linux-build/opensbi/.../fw_jump.bin` | OpenSBI firmware |
| `litex-build/boot_linux.json` | Image addresses for SFL boot |
| `scripts/auto_boot_linux.py` | Automated JTAG+serial boot script |
| `test/atomik_test_linux.c` | Userspace ATOMiK smoke test (9 tests) |
