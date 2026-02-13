# ATOMiK + PicoRV32 SoC Memory Map

## Address Space (picotiny SoC with ATOMiK)

| Address Range | Peripheral | Notes |
|---------------|-----------|-------|
| `0x0000_0000 - 0x007F_FFFF` | SPI Flash XIP | 8 MB, instruction fetch + data |
| `0x4000_0000 - 0x4000_1FFF` | SRAM | 8 KB data memory |
| `0x8000_0000 - 0x8000_1FFF` | Boot ROM | 2 KB ISP flasher |
| `0x8100_0000 - 0x8100_000F` | SPI Flash Config | Bitbang + mode control |
| `0x8200_0000 - 0x8200_000F` | GPIO | 7-bit I/O |
| `0x8300_0000 - 0x8300_000F` | UART | 115200 baud, 8N1 |
| **`0xC000_0000 - 0xC000_001F`** | **ATOMiK Delta Accumulator** | **NEW** |

## ATOMiK Register Map (base: 0xC000_0000)

| Offset | Name | R/W | Description |
|--------|------|-----|-------------|
| `0x00` | LOAD | W | Write initial state (32-bit). Clears delta accumulator. |
| `0x04` | ACCUM | W | Write delta to accumulate (XOR into accumulator). |
| `0x08` | STATE | R | Read current reconstructed state (initial XOR accumulator). |
| `0x0C` | STATUS | R | Bit 0: accumulator_zero. Bits 31:1: reserved. |
| `0x10` | CONFIG | W | Bit 0: soft reset (clears both registers). Bits 31:1: reserved. |
| `0x14` | INIT | R | Read current initial state (debug). |
| `0x18` | DELTA | R | Read current delta accumulator (debug). |

## C Header Definitions

```c
#define ATOMIK_BASE    0xC0000000
#define ATOMIK_LOAD    (*(volatile uint32_t*)(ATOMIK_BASE + 0x00))
#define ATOMIK_ACCUM   (*(volatile uint32_t*)(ATOMIK_BASE + 0x04))
#define ATOMIK_STATE   (*(volatile uint32_t*)(ATOMIK_BASE + 0x08))
#define ATOMIK_STATUS  (*(volatile uint32_t*)(ATOMIK_BASE + 0x0C))
#define ATOMIK_CONFIG  (*(volatile uint32_t*)(ATOMIK_BASE + 0x10))
#define ATOMIK_INIT    (*(volatile uint32_t*)(ATOMIK_BASE + 0x14))
#define ATOMIK_DELTA   (*(volatile uint32_t*)(ATOMIK_BASE + 0x18))
```

## Bus Integration

ATOMiK connects to the S3 (Wishbone) port of picotiny's primary 1:4 memory mux. This port was previously unused. Address decoding uses bits [31:30] = 2'b11 to select the ATOMiK peripheral.
