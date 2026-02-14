# Sipeed Tang Nano 9K — Reference Materials

Reference documentation for the Tang Nano 9K FPGA development board used in ATOMiK production deployment.

## Directory Structure

```
sipeed/
├── specs/          # FPGA datasheets, board schematics, pin assignments
├── optimization/   # Clock tree, placement, timing closure guides
├── peripherals/    # SPI flash, HDMI, UART, GPIO documentation
└── community/      # Community examples, forum threads, known issues
```

## Board Specifications

| Parameter | Value |
|-----------|-------|
| **FPGA** | Gowin GW1NR-LV9QN88PC6/I5 |
| **Logic** | 8,640 LUT4, 6,693 FF |
| **BSRAM** | 26 blocks (468 Kbit) |
| **PLL** | 2 |
| **Flash** | 32 Mbit SPI NOR |
| **Interface** | USB-C (JTAG + UART), HDMI, GPIO |

## Relevant ATOMiK Docs

- [Production Deployment](../../PRODUCTION_DEPLOYMENT.md) — SoC architecture and timing results
- [Hardware Synthesis](../../HARDWARE_SYNTHESIS.md) — Parallel bank sweep results
- [Gowin Reference](../gowin/README.md) — EDA toolchain documentation
