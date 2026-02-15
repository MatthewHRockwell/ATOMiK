# Firmware Compilation and Optimization Reference

Comprehensive guide for building, flashing, and optimizing bare-metal firmware for the ATOMiK + PicoRV32 SoC on the Tang Nano 9K.

**Target:** PicoRV32 @ 25.2 MHz, SPI XIP boot, 8 KB SRAM, ATOMiK coprocessor @ 81 MHz
**Board:** Sipeed Tang Nano 9K (GW1NR-LV9QN88PC6/I5)
**Toolchain:** `riscv64-unknown-elf-gcc` (multi-lib, supports RV32I targets)

---

## Table of Contents

1. [Toolchain Setup](#1-toolchain-setup)
2. [Compiler Flags Reference](#2-compiler-flags-reference)
3. [Boot Modes](#3-boot-modes)
4. [SoC Boot Sequence (SPI XIP)](#4-soc-boot-sequence-spi-xip)
5. [Linker Script Anatomy](#5-linker-script-anatomy)
6. [Build Process Step-by-Step](#6-build-process-step-by-step)
7. [Optimization Strategies](#7-optimization-strategies)
8. [Makefile Template](#8-makefile-template)
9. [Debugging Tips](#9-debugging-tips)
10. [Common Errors and Solutions](#10-common-errors-and-solutions)

---

## 1. Toolchain Setup

### Installation

The RISC-V GCC toolchain must support the `riscv32` target via multi-lib. On Ubuntu/Kubuntu:

```bash
# Option A: System package (Ubuntu 22.04+)
sudo apt install gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf

# Option B: From source or xPack (if system package unavailable)
# Download from https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases
```

The toolchain prefix is `riscv64-unknown-elf-` despite targeting a 32-bit core. The 64-bit toolchain is multi-lib and supports RV32I via the `-march=rv32i -mabi=ilp32` flags.

### Verification

```bash
# Confirm toolchain location
which riscv64-unknown-elf-gcc
# Expected: /usr/bin/riscv64-unknown-elf-gcc

# Confirm RV32I support
riscv64-unknown-elf-gcc -print-multi-lib | grep rv32i
# Should list rv32i/ilp32 in output

# Confirm version (any recent version works)
riscv64-unknown-elf-gcc --version

# Verify objcopy and objdump are present
riscv64-unknown-elf-objcopy --version
riscv64-unknown-elf-objdump --version
```

### Toolchain Components Used

| Tool | Binary | Purpose |
|------|--------|---------|
| Compiler | `riscv64-unknown-elf-gcc` | C/assembly compilation and linking |
| Object Copy | `riscv64-unknown-elf-objcopy` | ELF to binary/hex/verilog format conversion |
| Object Dump | `riscv64-unknown-elf-objdump` | Disassembly listing for inspection |
| Size | `riscv64-unknown-elf-size` | Section size summary (optional, useful for tracking firmware size) |

---

## 2. Compiler Flags Reference

### Architecture Flags

| Flag | Value | Explanation |
|------|-------|-------------|
| `-march=rv32i` | RV32I base integer ISA | **No extensions enabled.** Our PicoRV32 IP core does not have the M (multiply/divide) or C (compressed) extensions enabled. If the IP core is reconfigured to enable them, change to `rv32im`, `rv32ic`, or `rv32imc` accordingly. The compiler flag **must match** the IP core configuration exactly. |
| `-mabi=ilp32` | ILP32 integer ABI | Standard 32-bit integer ABI. `int`, `long`, and pointers are 32-bit. No floating-point registers are used for argument passing. This is the only ABI option for RV32I without the F/D extensions. |
| `-mtune=size` | Optimize for code size | Instructs the compiler to prefer smaller instruction sequences over faster ones. Recommended for PicoRV32 where flash bandwidth is the bottleneck and code must fit in 16 KB. |
| `-mcmodel=medany` | Medium-any code model | Generates position-independent-like code where symbols can be anywhere in the 32-bit address space, addressed via `auipc` + offset. Required because our memory map spans widely separated regions (flash at `0x0000_0000`, SRAM at `0x4000_0000`, peripherals at `0x8x00_0000`, ATOMiK at `0xC000_0000`). |
| `-msmall-data-limit=8` | Small data threshold | Variables 8 bytes or smaller are placed in the `.sdata`/`.sbss` sections, addressable via the global pointer (`gp`) register with a 12-bit offset. Reduces code size for frequently accessed small globals. |
| `-mstrict-align` | Enforce aligned access | **Critical.** PicoRV32 does not support fast unaligned memory access. Without this flag, the compiler may generate `lw`/`sw` to unaligned addresses, causing a trap or hang. |

### Optimization Flags

| Flag | Value | Explanation |
|------|-------|-------------|
| `-O3` | Aggressive optimization | Maximum optimization for speed. Acceptable because `-mtune=size` and `-ffunction-sections` + `--gc-sections` counteract code bloat. Can be changed to `-Os` if firmware exceeds 16 KB. |
| **`-fno-builtin`** | **ESSENTIAL** | **Prevents GCC from recognizing and replacing loops with calls to `memset`, `memcpy`, etc.** Without this flag, GCC at `-O3` detects patterns like byte-fill loops and replaces them with calls to `memset` -- which then calls itself in an infinite recursion because we provide our own `memset`. This causes an immediate stack overflow and hang on boot. |
| `-ffunction-sections` | One function per section | Places each function in its own `.text.function_name` section so the linker can individually discard unused functions. |
| `-fdata-sections` | One variable per section | Places each global variable in its own section so unused variables can be garbage-collected by the linker. |
| `-fstrict-volatile-bitfields` | Strict volatile bit access | Ensures that accesses to volatile bitfield members use the exact width specified, preventing the compiler from widening a byte access to a word access. Important for memory-mapped peripheral registers. |
| `-ffreestanding` | Freestanding environment | Tells the compiler this is a freestanding (bare-metal) environment: no standard library startup, no hosted assumptions. Implies that `main()` need not return `int` and standard headers may not provide all functions. |
| `-g` | Debug symbols | Includes DWARF debug information in the ELF. Does not affect the final binary size (stripped during `objcopy`), but makes `objdump -S` output interleaved C source + assembly for debugging. |

### Linker Flags

| Flag | Explanation |
|------|-------------|
| `-nostartfiles` | Do not link the default C runtime startup (`crt0.o`). We provide our own startup code in `crt_flash.S` that initializes `gp`, `sp`, copies `.data`, clears `.bss`, and calls `main()`. |
| `-nostdlib` | Do not link the standard C library. We have no `libc` -- all functions (`putchar`, `memset`, `printf`, etc.) are implemented in firmware source. |
| `-lgcc` | Link `libgcc.a` for compiler intrinsics (soft-float, 64-bit shifts, etc.). This is needed even without `libc` because the compiler may emit calls to `__mulsi3`, `__divsi3`, etc. for operations not natively supported by the ISA. |
| `-Wl,--gc-sections` | Garbage-collect unused sections. Combined with `-ffunction-sections` and `-fdata-sections`, the linker discards any function or variable not reachable from the entry point. This is critical for keeping firmware under 16 KB. |
| `-Wl,-Bstatic` | Force static linking. No dynamic linking is available on bare metal. |
| `-Wl,-T,<script>` | Use the specified linker script (e.g., `linker_flash.ld` for XIP, `linker_brom.ld` for boot ROM). The linker script defines the memory layout and section placement. |
| `-Wl,-Map,<file>` | Generate a link map file showing section sizes, symbol addresses, and memory usage. Essential for analyzing firmware size and diagnosing overflow. |
| `-Wl,--print-memory-usage` | Print a summary of memory region usage (flash and RAM) after linking. Produces output like `Memory region Used Size Region Size %age Used`. |

### Complete CFLAGS and LDFLAGS (Copy-Pasteable)

```makefile
CFLAGS = -march=rv32i -mabi=ilp32 -mtune=size -mcmodel=medany \
         -msmall-data-limit=8 -mstrict-align \
         -O3 -fno-builtin -ffreestanding \
         -ffunction-sections -fdata-sections \
         -fstrict-volatile-bitfields \
         -g -MD

LDFLAGS = -march=rv32i -mabi=ilp32 -mcmodel=medany \
          -nostartfiles -nostdlib -lgcc -ffreestanding \
          -Wl,--gc-sections -Wl,-Bstatic \
          -Wl,-T,linker_flash.ld \
          -Wl,-Map,build/atomik-fw.map \
          -Wl,--print-memory-usage
```

### Data Type Widths (ILP32 ABI)

| C Type | Width (bytes) | Width (bits) | Notes |
|--------|:---:|:---:|-------|
| `char` | 1 | 8 | Unsigned by default on RISC-V |
| `short` | 2 | 16 | |
| `int` | 4 | 32 | |
| `long` | 4 | 32 | Same as `int` in ILP32 |
| `long long` | 8 | 64 | Emulated via `libgcc` intrinsics |
| `void *` | 4 | 32 | |
| `float` | 4 | 32 | Software emulation only (no FPU) |
| `double` | 8 | 64 | Software emulation only (no FPU) -- **avoid** |
| `size_t` | 4 | 32 | `unsigned int` equivalent |
| `uint32_t` | 4 | 32 | Preferred type for register access |

---

## 3. Boot Modes

The Gowin PicoRV32 IP core supports three firmware boot/execution modes. Each requires a different linker script and build procedure.

| Boot Mode | Define | Linker Script | Code Runs From | Code Stored In | Use Case |
|-----------|--------|---------------|----------------|----------------|----------|
| **ITCM Load** | `BUILD_LOAD` | `sections.lds` | ITCM (on-chip SRAM) | Baked into bitstream (`ram32.hex`) | Development: fast iteration, code in BRAM |
| **ITCM Burn** | `BUILD_BURN` | `sections.lds` | ITCM (on-chip SRAM) | External SPI flash, copied to ITCM at boot | Production with fast execution |
| **SPI XIP** | `BUILD_XIP` | `sections_xip.lds` / `linker_flash.ld` | External SPI flash (execute-in-place) | External SPI flash | **Our production mode.** Saves BRAM for data. |

### ITCM Load (BUILD_LOAD)

Code is compiled into a hex file (`ram32.hex`) and baked directly into the FPGA bitstream as BRAM initialization data. The PicoRV32 executes directly from the BRAM. This is the simplest mode for development but requires re-synthesizing the entire bitstream for every firmware change.

**Build flow:** `.c` -> `.elf` -> `makehex32` -> `ram32.hex` -> bake into bitstream via Gowin IP Core GUI

### ITCM Burn (BUILD_BURN)

Code is stored in external SPI flash as a separate binary. At boot, the boot ROM copies firmware from flash into ITCM SRAM, then jumps to it. This gives the speed advantage of SRAM execution with the ability to update firmware independently of the bitstream.

**Build flow:** `.c` -> `.elf` -> `.bin` -> merge with bitstream via `mergebin` tool -> flash combined binary

### SPI XIP (BUILD_XIP) -- Our Production Mode

Code executes directly from external SPI flash via the SPI XIP (Execute-In-Place) interface. The CPU fetches instructions over SPI on demand. This is the most memory-efficient mode: all on-chip BRAM is free for the data path, and firmware can be updated by reflashing just the firmware region of SPI flash.

**Build flow:** `.c` -> `.elf` -> `.v` (Verilog hex) -> flash via `pico-programmer.py`

**Trade-off:** SPI fetch is slower than SRAM execution (1 instruction per ~4 SPI clock cycles in standard SPI mode, ~1 cycle with DSPI/CRM enabled). Our firmware compensates by enabling DSPI + Continuous Read Mode at startup.

---

## 4. SoC Boot Sequence (SPI XIP)

This is the exact sequence that occurs when the Tang Nano 9K is powered on or reset with our ATOMiK SoC bitstream:

```
Power-On / Reset
      |
      v
[1] FPGA configures from onboard SPI flash
    - Bitstream occupies the first portion of flash
    - FPGA fabric comes alive, PLLs lock:
        * HDMI PLL: 126 MHz -> divides to 25.2 MHz (pixel clock + CPU clock)
        * ATOMiK PLL: 81 MHz (independent clock domain)
      |
      v
[2] Reset synchronizer releases CPU reset
    - CPU begins fetching from 0x80000000 (Boot ROM)
    - Boot ROM is a 2 KB hard-coded SPI flasher/loader in BRAM
      |
      v
[3] Boot ROM initializes SPI flash interface
    - Configures QSPI controller for XIP mode
    - Maps flash address space starting at 0x00000000
    - Jumps to 0x00000000 (start of firmware in flash)
      |
      v
[4] crt_flash.S (crtStart) executes from flash
    - Sets global pointer:    la gp, __global_pointer$
    - Sets stack pointer:     la sp, _stack_start
    - Copies .data section:   flash (_sidata) -> SRAM (_sdata to _edata)
    - Clears .bss section:    zero-fills _bss_start to _bss_end
    - Runs C++ constructors:  _ctors_start to _ctors_end (if any)
    - Calls main()
      |
      v
[5] main() in firmware.c
    - Configures UART baud rate: CLK_FREQ / UART_BAUD - 2
    - Sets GPIO direction and initial state (LEDs)
    - Enables DSPI + CRM for faster flash reads
    - Prints boot banner
    - Enters interactive command loop
```

### Memory Map at Runtime

```
0x0000_0000 +---------------------------+
            |  SPI Flash XIP (8 MB)     |  <- Code executes here (read-only)
            |  .text, .rodata, .ctors   |
            |  _sidata (init values)    |
0x007F_FFFF +---------------------------+

0x4000_0000 +---------------------------+
            |  SRAM (8 KB)              |  <- Data memory (read/write)
            |  .data (copied from flash)|
            |  .bss  (zeroed at boot)   |
            |  heap  (2 KB)             |
            |  stack (1 KB, grows down) |
0x4000_1FFF +---------------------------+

0x8000_0000 +---------------------------+
            |  Boot ROM (2 KB)          |  <- ISP flasher, SPI init
0x8000_07FF +---------------------------+

0x8100_0000   SPI Flash Config registers
0x8200_0000   GPIO (7-bit, active-low LEDs)
0x8300_0000   UART (115200 baud, 8N1)

0xC000_0000 +---------------------------+
            |  ATOMiK Registers         |  <- Delta accumulator
            |  7 registers per bank     |
            |  32-byte stride per bank  |
0xC000_001F +---------------------------+
```

---

## 5. Linker Script Anatomy

Two linker scripts exist in the firmware directory, each for a different boot mode.

### linker_flash.ld (SPI XIP -- Our Production Script)

This is the linker script used for SPI XIP boot. Code and read-only data stay in flash; mutable data is placed in SRAM.

```
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 8M    /* SPI flash, execute-in-place */
  RAM   (xrw) : ORIGIN = 0x40000000, LENGTH = 8k    /* On-chip SRAM */
}
```

**Section Placement:**

| Section | Memory Region | Contents |
|---------|:---:|---------|
| `.vector` | FLASH | Startup code (`crt_flash.o` `.text`). Entry point at address 0. |
| `.text` | FLASH | All compiled code (`.text`, `.text.*`), plus read-only data (`.rodata`, `.srodata`). |
| `.rodata` | FLASH | Additional read-only data (`.rdata`, `.rodata.*`). |
| `.ctors` | FLASH | C++ constructor table (`init_array`, `.ctors`). |
| `.data` | RAM (load from FLASH) | Initialized global/static variables. The `AT(_sidata)` directive places the initialization values in flash; `crt_flash.S` copies them to SRAM at boot. |
| `.bss` | RAM | Zero-initialized global/static variables. Cleared by `crt_flash.S` at boot. |
| `.heap` | RAM | Bump allocator region (2 KB default). |
| `.stack` | RAM | Stack region (1 KB default, grows downward). |

**Key Linker Symbols:**

| Symbol | Meaning |
|--------|---------|
| `_sidata` | Flash address of `.data` initialization values (source for copy) |
| `_sdata` / `_edata` | SRAM start/end of `.data` section (destination for copy) |
| `_bss_start` / `_bss_end` | SRAM start/end of `.bss` section (zeroed at boot) |
| `__global_pointer$` | Center of `.sdata` section + 0x800, loaded into `gp` register |
| `_stack_start` | Top of stack (initial `sp` value) |
| `_heap_start` / `_heap_end` | Bounds of heap region |

### linker_brom.ld (Boot ROM Mode)

Same structure as `linker_flash.ld` but places code at the Boot ROM address:

```
MEMORY
{
  BROM (rx)  : ORIGIN = 0x80000000, LENGTH = 32k    /* Boot ROM region */
  RAM  (xrw) : ORIGIN = 0x40000000, LENGTH = 8k     /* On-chip SRAM */
}
```

This script is used when building firmware that runs from the Boot ROM BRAM region instead of SPI flash. Not used in production.

### SRAM Budget (8 KB Total)

The 8 KB of SRAM must hold all mutable state. Budget carefully:

```
+----------------------------+
|  .data   (initialized)     |  ~ variable (keep small)
|  .bss    (zeroed)          |  ~ variable (keep small)
|  heap    (2 KB default)    |  2,048 bytes
|  stack   (1 KB default)    |  1,024 bytes
+----------------------------+
Total available: 8,192 bytes

Overhead (.data + .bss): check with "riscv64-unknown-elf-size build/atomik-fw.elf"
```

---

## 6. Build Process Step-by-Step

### Step 1: Compile C Sources to Object Files

```bash
riscv64-unknown-elf-gcc -c \
    -march=rv32i -mabi=ilp32 -mtune=size -mcmodel=medany \
    -msmall-data-limit=8 -mstrict-align \
    -O3 -fno-builtin -ffreestanding \
    -ffunction-sections -fdata-sections \
    -fstrict-volatile-bitfields -g -MD \
    -o build/firmware.o firmware.c
```

Repeat for each `.c` file: `printf.c`, `atomik_mem.c`, `atomik_alloc.c`, `perf_bench.c`.

### Step 2: Assemble Startup Code

```bash
riscv64-unknown-elf-gcc -c \
    -march=rv32i -mabi=ilp32 \
    -ffunction-sections -fdata-sections \
    -g -MD -D__ASSEMBLY__=1 \
    -o build/crt_flash.o crt_flash.S
```

### Step 3: Link into ELF

```bash
riscv64-unknown-elf-gcc \
    -march=rv32i -mabi=ilp32 -mcmodel=medany \
    -nostartfiles -nostdlib -lgcc -ffreestanding \
    -Wl,--gc-sections -Wl,-Bstatic \
    -Wl,-T,linker_flash.ld \
    -Wl,-Map,build/atomik-fw.map \
    -Wl,--print-memory-usage \
    -o build/atomik-fw.elf \
    build/crt_flash.o build/firmware.o build/printf.o \
    build/atomik_mem.o build/atomik_alloc.o
```

**Note:** `crt_flash.o` should be listed first so it appears at address 0x00000000 in flash, matching the `.vector` section placement in the linker script.

The `--print-memory-usage` flag outputs:

```
Memory region         Used Size  Region Size  %age Used
           FLASH:       15312 B         8 MB      0.18%
             RAM:        4104 B         8 KB     48.88%
```

### Step 4: Generate Output Formats

```bash
# Verilog hex format (for pico-programmer.py flashing)
riscv64-unknown-elf-objcopy -O verilog build/atomik-fw.elf build/atomik-fw.v

# Intel HEX format (alternative)
riscv64-unknown-elf-objcopy -O ihex build/atomik-fw.elf build/atomik-fw.hex

# Raw binary (for mergebin or direct flash tools)
riscv64-unknown-elf-objcopy -O binary build/atomik-fw.elf build/atomik-fw.bin

# Disassembly listing (for debugging)
riscv64-unknown-elf-objdump -S -d build/atomik-fw.elf > build/atomik-fw.asm
```

### Step 5: Flash to Tang Nano 9K

The firmware is flashed to the SPI flash via UART using `pico-programmer.py`. The FPGA bitstream must already be loaded.

**Method A: With physical reset button**

```bash
# 1. Press and hold the reset button on the Tang Nano 9K
# 2. Run the programmer:
python3 pico-programmer.py build/atomik-fw.v /dev/ttyUSB1
# 3. Release the reset button
```

**Method B: ISP flash trick (no physical reset needed)**

```bash
# 1. Re-flash the bitstream (this resets the FPGA and CPU):
openFPGALoader -b tangnano9k picotiny.fs

# 2. Immediately run the firmware programmer (within ~1 second):
python3 pico-programmer.py build/atomik-fw.v /dev/ttyUSB1
```

The ISP flash trick works because `openFPGALoader` resets the FPGA, which causes the CPU to enter the boot ROM's ISP flash-receive mode. The `pico-programmer.py` script then sends the firmware data over UART before the boot ROM times out and jumps to the existing firmware in flash.

### Step 6: Verify

Connect to UART at 115200 baud, 8N1:

```bash
# Using screen
screen /dev/ttyUSB1 115200

# Using minicom
minicom -D /dev/ttyUSB1 -b 115200

# Using picocom
picocom -b 115200 /dev/ttyUSB1
```

You should see the PicoSoC boot banner and interactive menu. Press `X` to run the ATOMiK hardware test (11/11 PASS expected).

---

## 7. Optimization Strategies

### Code Size Optimization

The production firmware must fit in approximately 16 KB of SPI flash (the firmware region after the FPGA bitstream). Use these techniques:

| Technique | Savings | How |
|-----------|---------|-----|
| `-ffunction-sections` + `--gc-sections` | Large | Automatically removes unreachable functions. Always enable. |
| `-mtune=size` | Moderate | Compiler prefers smaller instruction sequences. |
| Switch `-O3` to `-Os` | Moderate | If firmware exceeds budget, `-Os` optimizes for size instead of speed. |
| Avoid `float`/`double` | Large | Software FP emulation from `libgcc` pulls in hundreds of bytes per operation. Use fixed-point integer math instead. |
| Avoid `printf` format strings | Moderate | Each unique format string is stored in flash. Consolidate messages. |
| Use `mini_printf` | Large | Our custom `mini_printf` is ~400 bytes total. Standard `printf` would be 5-10 KB. |
| Minimize string literals | Variable | Every `print("...")` string lives in `.text`/`.rodata` in flash. |
| Use `static` for file-local functions | Small | Enables the compiler to inline or eliminate unused local functions. |

### Checking Firmware Size

```bash
# Quick size summary
riscv64-unknown-elf-size build/atomik-fw.elf

# Example output:
#    text    data     bss     dec     hex filename
#   15312      56    4048   19416    4bd8 build/atomik-fw.elf
#
# "text" is the flash footprint (code + rodata)
# "data" is initialized variables (in flash AND ram)
# "bss" is zeroed variables (ram only)

# Detailed section sizes
riscv64-unknown-elf-size -A build/atomik-fw.elf

# Check actual binary size
ls -la build/atomik-fw.bin

# Inspect the link map for largest functions
# (open build/atomik-fw.map and look for large .text.* sections)
```

### Avoiding Common Pitfalls

#### No Multiply/Divide Without RVM Extension

RV32I (base integer ISA) has **no multiply or divide instructions**. The compiler will emit calls to `libgcc` software routines (`__mulsi3`, `__divsi3`, `__udivsi3`, `__umodsi3`) which are slow (dozens of cycles per operation).

**Consequences:**
- Standard `printf` with `%d` requires division by 10 to convert integers to decimal. Our `mini_printf` avoids this by using a powers-of-10 table with repeated subtraction.
- Avoid `*` and `/` operators in hot loops. Use bit shifts where possible: `x * 4` becomes `x << 2`, `x / 8` becomes `x >> 3`.
- If multiply performance is critical, enable the M extension in both the PicoRV32 IP core **and** the compiler (`-march=rv32im`). This adds a hardware multiplier at the cost of additional LUTs.

```c
// BAD: Uses software division (slow on RV32I)
int baud_divisor = clock_freq / baud_rate;

// GOOD: Pre-compute at compile time
#define CLK_FREQ  25175000
#define UART_BAUD 115200
UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;  // Computed at compile time by GCC
```

#### No Unaligned Memory Access

PicoRV32 traps on unaligned word access. Always ensure:

- Buffers used with `lw`/`sw` are 4-byte aligned
- Structures accessed as `uint32_t *` are word-aligned
- Use `-mstrict-align` to prevent the compiler from generating unaligned access
- When casting byte buffers to `uint32_t *`, ensure alignment: `__attribute__((aligned(4)))`

```c
// BAD: Potentially unaligned
uint8_t buffer[100];
uint32_t *words = (uint32_t *)&buffer[1];  // Misaligned!

// GOOD: Ensure alignment
uint8_t buffer[100] __attribute__((aligned(4)));
uint32_t *words = (uint32_t *)buffer;  // Aligned
```

#### The -fno-builtin Rule

This deserves special emphasis. At `-O3`, GCC performs **loop idiom recognition**: it detects byte-copy loops and replaces them with calls to `memcpy`, and byte-fill loops with calls to `memset`. In a hosted environment this is an optimization. In bare-metal, where we provide our own `memcpy`/`memset`, it creates infinite recursion:

```
Our memset() -> GCC recognizes the loop pattern -> calls memset() -> calls memset() -> ...
```

**Always compile with `-fno-builtin`.** There is no safe alternative for bare-metal code at `-O3`.

### Printf Alternatives for Bare-Metal

| Approach | Flash Cost | Features | Recommendation |
|----------|-----------|----------|----------------|
| `mini_printf` (our implementation) | ~400 bytes | `%d`, `%u`, `%x`, `%s`, `%c`, `%%`, width specifiers | **Use this.** Covers all needs. |
| `print()` + `print_hex()` | ~150 bytes | String output + hex dump only | Use for minimal builds if `mini_printf` is too large. |
| Standard `printf` (`newlib-nano`) | 5-10 KB | Full format string support | **Do not use.** Too large, requires `malloc`, and pulls in soft-float. |
| No output (LED-only debugging) | 0 bytes | None | Last resort for extreme size constraints. |

Our `mini_printf` avoids hardware division by using a powers-of-10 lookup table with repeated subtraction for decimal conversion:

```c
static const uint32_t pow10[] = {
    1000000000, 100000000, 10000000, 1000000,
    100000, 10000, 1000, 100, 10, 1
};
// For each power of 10, subtract repeatedly to find the digit
```

---

## 8. Makefile Template

This is the production Makefile used in `hardware/picorv32/firmware/`. It is a complete, copy-pasteable build system.

```makefile
# ==============================================================================
# ATOMiK Firmware Makefile
# Target: PicoRV32 @ 25.2 MHz on Tang Nano 9K (SPI XIP boot)
# ==============================================================================

PROJ_NAME = atomik-fw

# Source files (all .c and .S in current directory)
SRCS = $(wildcard *.c) $(wildcard *.S)

# Linker script: linker_flash.ld for SPI XIP, linker_brom.ld for Boot ROM
LDSCRIPT = ./linker_flash.ld

# Toolchain configuration
RISCV_NAME ?= riscv64-unknown-elf
RISCV_PATH ?= /usr

RISCV_CC      = $(RISCV_PATH)/bin/$(RISCV_NAME)-gcc
RISCV_OBJCOPY = $(RISCV_PATH)/bin/$(RISCV_NAME)-objcopy
RISCV_OBJDUMP = $(RISCV_PATH)/bin/$(RISCV_NAME)-objdump
RISCV_SIZE    = $(RISCV_PATH)/bin/$(RISCV_NAME)-size

# Architecture flags
MABI  = ilp32
MARCH = rv32i

# Compiler flags
CFLAGS  = -march=$(MARCH) -mabi=$(MABI)
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -g -O3
CFLAGS += -MD                            # Auto-generate dependency files
CFLAGS += -fstrict-volatile-bitfields    # Exact-width volatile access
CFLAGS += -fno-builtin                   # ESSENTIAL: prevent memset/memcpy recursion

# Linker flags
LDFLAGS  = -march=$(MARCH) -mabi=$(MABI)
LDFLAGS += -Wl,--gc-sections             # Remove unused sections
LDFLAGS += -nostdlib -lgcc               # No libc, but keep compiler intrinsics
LDFLAGS += -mcmodel=medany               # Symbols anywhere in 32-bit space
LDFLAGS += -nostartfiles -ffreestanding  # Bare-metal, custom crt
LDFLAGS += -Wl,-Bstatic,-T,$(LDSCRIPT),-Map,$(OBJDIR)/$(PROJ_NAME).map,--print-memory-usage

# Build directory
OBJDIR = build

# Object file list (substitute extensions)
OBJS := $(SRCS)
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.S=.o)
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))

# ==============================================================================
# Targets
# ==============================================================================

all: $(OBJDIR)/$(PROJ_NAME).elf $(OBJDIR)/$(PROJ_NAME).hex $(OBJDIR)/$(PROJ_NAME).v $(OBJDIR)/$(PROJ_NAME).asm
	@echo ""
	@echo "--- Firmware size ---"
	@$(RISCV_SIZE) $(OBJDIR)/$(PROJ_NAME).elf
	@echo ""

# Link ELF
$(OBJDIR)/%.elf: $(OBJS) | $(OBJDIR)
	$(RISCV_CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Generate Intel HEX
%.hex: %.elf
	$(RISCV_OBJCOPY) -O ihex $^ $@

# Generate raw binary
%.bin: %.elf
	$(RISCV_OBJCOPY) -O binary $^ $@

# Generate Verilog hex (for pico-programmer.py)
%.v: %.elf
	$(RISCV_OBJCOPY) -O verilog $^ $@

# Generate disassembly listing
%.asm: %.elf
	$(RISCV_OBJDUMP) -S -d $^ > $@

# Compile C sources
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(RISCV_CC) -c $(CFLAGS) -o $@ $^

# Assemble .S sources
$(OBJDIR)/%.o: %.S | $(OBJDIR)
	$(RISCV_CC) -c $(CFLAGS) -o $@ $^ -D__ASSEMBLY__=1

# Create build directory
$(OBJDIR):
	mkdir -p $@

# Flash firmware to Tang Nano 9K (requires ISP reset first)
flash: $(OBJDIR)/$(PROJ_NAME).v
	@echo "Flash bitstream first: openFPGALoader -b tangnano9k picotiny.fs"
	@echo "Then immediately run:  python3 pico-programmer.py $< /dev/ttyUSB1"

# Clean build artifacts
clean:
	rm -rf $(OBJDIR)

# Include auto-generated dependencies (for incremental builds)
-include $(OBJS:.o=.d)

.SECONDARY: $(OBJS)
.PHONY: all clean flash
```

### Quick Build Commands

```bash
# Full build
make

# Clean and rebuild
make clean && make

# Build with size optimization instead of speed
make CFLAGS="-march=rv32i -mabi=ilp32 -Os -fno-builtin -ffunction-sections -fdata-sections -MD -fstrict-volatile-bitfields -g"

# Build for boot ROM mode instead of XIP
make LDSCRIPT=./linker_brom.ld

# Flash (two-step process)
openFPGALoader -b tangnano9k picotiny.fs && python3 pico-programmer.py build/atomik-fw.v /dev/ttyUSB1
```

---

## 9. Debugging Tips

### UART Printf Debugging

The primary debugging mechanism. UART is configured at 115200 baud by default.

```c
#include "printf.h"

// Print a status message
mini_printf("State: 0x%08x at cycle %u\n", value, cycles());

// Print hex dump of a memory region
void hex_dump(const uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (i % 16 == 0) mini_printf("\n%08x: ", (uint32_t)buf + i);
        mini_printf("%02x ", buf[i]);
    }
    mini_printf("\n");
}
```

### LED Toggle Debugging

When UART is not available (e.g., UART is broken, or debugging very early boot), use the 6 onboard LEDs:

```c
#define GPIO0 ((PICOGPIO*)0x82000000)

// LEDs are active-low: OUT=0 turns LED on, OUT=1 turns LED off
GPIO0->OE = 0x3F;       // Enable all 6 LED outputs

// Blink pattern to indicate a checkpoint was reached
GPIO0->OUT = 0x00;      // All LEDs on
for (volatile int i = 0; i < 50000; i++);  // Delay
GPIO0->OUT = 0x3F;      // All LEDs off

// Show a 6-bit value on LEDs (inverted: 0=on, 1=off)
void led_show(uint8_t val) {
    GPIO0->OUT = ~val & 0x3F;
}

// Show error code on LEDs and halt
void led_halt(uint8_t code) {
    GPIO0->OE = 0x3F;
    GPIO0->OUT = ~code & 0x3F;
    while (1);  // Hang with error code displayed
}
```

### Cycle Counting

Use the `rdcycle` CSR for performance measurement:

```c
#include "atomik_mem.h"  // provides cycles()

uint32_t c0 = cycles();
// ... code to measure ...
uint32_t c1 = cycles();
mini_printf("Operation took %u cycles\n", c1 - c0);

// Convert cycles to microseconds (at 25.2 MHz):
// 1 cycle = 1/25,175,000 seconds ~ 39.7 ns
// microseconds = cycles * 1000 / 25175  (approximate)
```

### Memory Inspection

Use the link map and `objdump` to diagnose issues:

```bash
# View section sizes and addresses
riscv64-unknown-elf-objdump -h build/atomik-fw.elf

# View symbol table (all function and variable addresses)
riscv64-unknown-elf-objdump -t build/atomik-fw.elf | sort

# View disassembly with source interleaving
riscv64-unknown-elf-objdump -S -d build/atomik-fw.elf | less

# Check for unexpected large sections in the map file
grep -E "^\s+\.(text|data|bss|rodata)" build/atomik-fw.map

# Find the largest functions
riscv64-unknown-elf-nm --size-sort --print-size build/atomik-fw.elf | tail -20
```

### ATOMiK Register Inspection

Read ATOMiK hardware state for debugging:

```c
#include "atomik.h"

// Dump all ATOMiK registers for bank 0
void atomik_dump(void) {
    mini_printf("ATOMiK Bank 0:\n");
    mini_printf("  INIT:   0x%08x\n", atomik_get_initial(0));
    mini_printf("  DELTA:  0x%08x\n", atomik_get_delta(0));
    mini_printf("  STATE:  0x%08x\n", atomik_state(0));
    mini_printf("  UNCHANGED: %d\n", atomik_unchanged(0));
}
```

### Common Debugging Workflow

1. **Firmware does not boot (no UART output):**
   - Check that the bitstream is flashed correctly (`openFPGALoader` reports success)
   - Try LED toggle in `crt_flash.S` before `call main` to confirm startup code runs
   - Verify UART baud divisor: `CLK_FREQ / UART_BAUD - 2` with `CLK_FREQ = 25175000`
   - Check that the correct serial device is used (`/dev/ttyUSB1`, not `/dev/ttyUSB0` which is JTAG)

2. **Firmware hangs mid-execution:**
   - Add LED toggle checkpoints to narrow down the hang location
   - Check for stack overflow: reduce local variable sizes, increase `_stack_size` in linker script
   - Check for infinite recursion (missing `-fno-builtin`?)
   - Check for unaligned memory access (missing `-mstrict-align`?)

3. **Garbage on UART:**
   - Baud rate mismatch: verify `CLK_FREQ` matches actual CPU clock (25.175 MHz, not 27 MHz)
   - Wrong serial port: Tang Nano 9K exposes two USB serial devices; UART is typically `/dev/ttyUSB1`

---

## 10. Common Errors and Solutions

| Error / Symptom | Cause | Solution |
|-----------------|-------|----------|
| **Firmware hangs immediately on boot, no output** | `-fno-builtin` missing. GCC replaced `memset`/`memcpy` loops with recursive calls. Stack overflow on first `memset`. | Add `-fno-builtin` to `CFLAGS`. This is **mandatory** for bare-metal at `-O3`. |
| **`undefined reference to __mulsi3`** | Code uses `*` operator but `libgcc` is not linked. | Add `-lgcc` to `LDFLAGS`. |
| **`undefined reference to memset`** or **`memcpy`** | GCC auto-generates calls to `memset`/`memcpy` for struct copies or array init. | Provide implementations of `memset` and `memcpy` in your source (see `atomik_mem.c`), **and** use `-fno-builtin`. |
| **SRAM overflow: `.data` + `.bss` + heap + stack > 8 KB** | Too many global variables or large static buffers. | Reduce buffer sizes. Move constant data to `const` (stays in flash). Check `--print-memory-usage` output. |
| **Trap/hang on memory access** | Unaligned `lw`/`sw` to non-4-byte-aligned address. | Use `-mstrict-align`. Ensure all `uint32_t *` casts point to aligned addresses. |
| **Wrong values from ATOMiK registers** | Reading/writing to wrong address. ATOMiK base is `0xC0000000`, not `0xC000000`. | Double-check `ATOMIK_BASE` in `atomik.h`. Verify the bus wrapper address decode in RTL. |
| **`pico-programmer.py` times out** | Boot ROM ISP window expired before programmer connected. | Use the ISP flash trick: run `openFPGALoader` immediately before `pico-programmer.py`. |
| **`/dev/ttyUSB1` not found** | FTDI driver not loaded, or device permissions. | Run `sudo dmesg | tail` to check USB enumeration. Add user to `dialout` group: `sudo usermod -a -G dialout $USER`. |
| **Firmware too large (> 16 KB)** | Too many string literals, or unused functions not stripped. | Switch to `-Os`. Verify `--gc-sections` is in LDFLAGS. Remove unused `#include` and functions. Check `nm --size-sort` output. |
| **Link error: `cannot move location counter backwards`** | Stack + heap + data exceeds 8 KB SRAM. | Reduce `_stack_size` or `_heap_size` in linker script, or reduce `.data`/`.bss` usage. |
| **Garbled UART output** | Baud rate calculation wrong. `CLK_FREQ` does not match actual clock. | Verify: `UART0->CLKDIV = 25175000 / 115200 - 2 = 216`. Measure actual clock with `rdcycle` and LED toggle timing. |
| **FPGA Fmax too low after adding firmware code** | N/A -- firmware does not affect FPGA timing. | Firmware changes only affect flash contents, not the FPGA bitstream. Re-synthesize only if RTL changes. |
| **Firmware partially corrupted after `openFPGALoader --external-flash`** | JTAG SPI bitbang writes to MSPI NOR flash in a format incompatible with `spimemio` XIP controller. Early code pages work but later pages cause CPU hangs on MMIO operations. | **Never use `openFPGALoader --external-flash` for firmware.** Always use the ISP programmer (`pico-programmer.py`) via UART. See `docs/KNOWN_ISSUES.md` HW-001. |
| **Compiler warns `division by zero` or similar with constants** | Compile-time division is fine; runtime division on RV32I uses software emulation. | Ensure divisions are compile-time constants (`#define` ratios) or use shift-based alternatives. |
| **`__attribute__((constructor))` not called** | `.ctors` section not included in linker script, or `ctors_init` loop missing from `crt_flash.S`. | Both are present in our linker script and startup code. Verify `.ctors` section exists in the ELF: `objdump -h atomik-fw.elf`. |

---

## Appendix: Gowin PicoRV32 Extension Configuration

If you reconfigure the PicoRV32 IP core in Gowin EDA to enable ISA extensions, update the compiler flags to match:

| Extension | IP Core Setting | Compiler Flag | Effect |
|-----------|----------------|---------------|--------|
| Base (RV32I) | Default | `-march=rv32i` | Integer ALU only. No multiply/divide. |
| Multiply (RVM) | Enable `ENABLE_MUL` + `ENABLE_DIV` | `-march=rv32im` | Adds `mul`, `div`, `rem` instructions. Uses ~200 additional LUTs. |
| Compressed (RVC) | Enable `ENABLE_COMPRESSED` | `-march=rv32ic` or `-march=rv32imc` | 16-bit instruction encoding for common operations. ~25% code size reduction. Small LUT cost. |
| Both M+C | Enable both | `-march=rv32imc` | Best performance + density. Recommended if LUT budget allows. |

**The compiler `-march` flag must exactly match the IP core configuration.** Using `-march=rv32im` with an IP core that lacks the M extension will generate `mul`/`div` instructions that the CPU cannot execute, causing an illegal instruction trap.

---

*Last Updated: February 14, 2026*
