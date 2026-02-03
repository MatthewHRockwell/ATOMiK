# Gowin GPIO Reference

Reference documentation for GPIO configuration on GW1NR-9 FPGA.

**Source**: UG289-1.9.2E (Gowin Programmable IO User Guide), UG803-1.6E (GW1NR-9 Pinout)

---

## Bank Configuration (GW1NR-9)

The GW1NR-9 has 4 I/O banks, each with independent VCCO supply.

| Bank | VCCO Range | Special Features |
|------|------------|------------------|
| **Bank 0** | 1.14V - 3.6V | 100Ω input differential matching, MIPI input (top) |
| **Bank 1** | 1.14V - 3.6V | True LVDS output, MIPI output (bottom) |
| **Bank 2** | 1.14V - 3.6V | General I/O |
| **Bank 3** | 1.14V - 3.6V | True LVDS output, JTAG pins |

### Bank Layout (GW1NR-9)

```
              ┌─────────────────────┐
              │      BANK 0         │  ← Top (MIPI input capable)
              │   (100Ω diff in)    │
    ┌─────────┼─────────────────────┼─────────┐
    │         │                     │         │
    │ BANK 3  │      GW1NR-9        │ BANK 1  │
    │ (JTAG)  │                     │ (LVDS)  │
    │         │                     │         │
    └─────────┼─────────────────────┼─────────┘
              │      BANK 2         │  ← Bottom (MIPI output capable)
              └─────────────────────┘
```

---

## Supported I/O Standards

### Single-Ended Standards

| Standard | VCCO | Description |
|----------|------|-------------|
| LVCMOS33 | 3.3V | Low-Voltage CMOS |
| LVCMOS25 | 2.5V | Low-Voltage CMOS |
| LVCMOS18 | 1.8V | Low-Voltage CMOS |
| LVCMOS15 | 1.5V | Low-Voltage CMOS |
| LVCMOS12 | 1.2V | Low-Voltage CMOS |
| LVTTL | 3.3V | Low-Voltage TTL |
| PCI33 | 3.3V | PCI 33 MHz |

### Differential Standards

| Standard | VCCO | Notes |
|----------|------|-------|
| LVDS25 | 2.5V | True LVDS (Bank 1/2/3 output) |
| LVDS25E | 2.5V | Emulated LVDS (external resistors) |
| BLVDS25 | 2.5V | Bus LVDS |
| MLVDS25 | 2.5V | Multipoint LVDS |

---

## CST Physical Constraints Syntax

The CST (Constraint) file defines physical pin locations and I/O attributes.

### Basic Pin Location

```
// Lock signal to specific pin
IO_LOC "signal_name" pin_number;

// Examples
IO_LOC "sys_clk" 52;
IO_LOC "led[0]" 10;
IO_LOC "uart_rx" 18;
```

### I/O Type (Level Standard)

```
// Set I/O standard
IO_PORT "signal_name" IO_TYPE=standard;

// Examples
IO_PORT "sys_clk" IO_TYPE=LVCMOS33;
IO_PORT "led[0]" IO_TYPE=LVCMOS18;
```

### Drive Strength

```
// Set output drive strength in mA
IO_PORT "signal_name" DRIVE=strength;

// Valid values: 4, 8, 12, 16, 24 (device dependent)
// Examples
IO_PORT "uart_tx" DRIVE=8;
IO_PORT "led[0]" DRIVE=4;
```

### Pull-Up / Pull-Down

```
// Set pull mode
IO_PORT "signal_name" PULL_MODE=mode;

// Valid modes:
//   UP     - Weak pull-up resistor
//   DOWN   - Weak pull-down resistor
//   KEEPER - Bus-hold (maintains last driven state)
//   NONE   - High impedance (no pull)

// Examples
IO_PORT "sys_rst_n" PULL_MODE=UP;
IO_PORT "data_in" PULL_MODE=NONE;
```

### Slew Rate

```
// Set output slew rate
IO_PORT "signal_name" SLEW_RATE=rate;

// Valid rates:
//   SLOW - Low noise, reduced EMI
//   FAST - High speed, sharper edges

// Examples
IO_PORT "uart_tx" SLEW_RATE=SLOW;
IO_PORT "spi_clk" SLEW_RATE=FAST;
```

### Hysteresis (Schmitt Trigger)

```
// Set input hysteresis
IO_PORT "signal_name" HYSTERESIS=level;

// Valid levels:
//   NONE - No hysteresis
//   L2H  - Low to high threshold
//   H2L  - High to low threshold  
//   HIGH - Large hysteresis

// Example (noisy input signal)
IO_PORT "button" HYSTERESIS=HIGH;
```

### Open Drain

```
// Enable open-drain output
IO_PORT "signal_name" OPEN_DRAIN=ON;

// Example (I2C signals)
IO_PORT "i2c_sda" OPEN_DRAIN=ON;
IO_PORT "i2c_scl" OPEN_DRAIN=ON;
```

### Termination Resistors

```
// Single-ended termination (100 ohm)
IO_PORT "signal_name" SINGLE_RESISTOR=100;

// Differential termination (100 ohm, Bank 0 only)
IO_PORT "diff_signal" DIFF_RESISTOR=100;
```

---

## Complete CST Example

```
//=============================================================================
// ATOMiK Physical Constraints
// Target: Tang Nano 9K (GW1NR-LV9QN88PC6/I5)
//=============================================================================

// Clock input (27 MHz crystal)
IO_LOC "sys_clk" 52;
IO_PORT "sys_clk" IO_TYPE=LVCMOS33 PULL_MODE=NONE;

// Reset button (directly active-low on button press)
IO_LOC "sys_rst_n" 4;
IO_PORT "sys_rst_n" IO_TYPE=LVCMOS18 PULL_MODE=UP;

// UART interface
IO_LOC "uart_rx" 18;
IO_LOC "uart_tx" 17;
IO_PORT "uart_rx" IO_TYPE=LVCMOS33 PULL_MODE=UP;
IO_PORT "uart_tx" IO_TYPE=LVCMOS33 DRIVE=8 SLEW_RATE=SLOW;

// LEDs (directly accent, accent, accent, accent, accent, accent accent-low accent-low)
IO_LOC "led[0]" 10;
IO_LOC "led[1]" 11;
IO_LOC "led[2]" 13;
IO_LOC "led[3]" 14;
IO_LOC "led[4]" 15;
IO_LOC "led[5]" 16;
IO_PORT "led[0]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[1]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[2]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[3]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[4]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[5]" IO_TYPE=LVCMOS18 DRIVE=8;
```

---

## GPIO Primitives

### IBUF (Input Buffer)

```verilog
IBUF ibuf_inst (
    .O(internal_signal),  // Output to fabric
    .I(pad_input)         // Input from pad
);
```

### OBUF (Output Buffer)

```verilog
OBUF obuf_inst (
    .O(pad_output),       // Output to pad
    .I(internal_signal)   // Input from fabric
);
```

### IOBUF (Bidirectional Buffer)

```verilog
IOBUF iobuf_inst (
    .O(read_data),        // Output to fabric (read)
    .IO(pad_bidir),       // Bidirectional pad
    .I(write_data),       // Input from fabric (write)
    .OEN(output_enable_n) // Output enable (active-LOW)
);
```

### TBUF (Tristate Buffer)

```verilog
TBUF tbuf_inst (
    .O(pad_output),       // Output to pad
    .I(internal_signal),  // Input from fabric
    .OEN(tristate_n)      // Output enable (active-LOW)
);
```

---

## Special Pin Functions

### JTAG Pins (Bank 3)

| Signal | Pin | Function |
|--------|-----|----------|
| TMS | IOL11A | JTAG Mode Select |
| TCK | IOL11B | JTAG Clock |
| TDI | IOL12B | JTAG Data In |
| TDO | IOL13A | JTAG Data Out |

**Note**: JTAG pins can be repurposed as GPIO if JTAG is disabled.

### Configuration Pins

| Signal | Pin | Function |
|--------|-----|----------|
| MODE0 | IOT5A | Configuration mode select |
| MODE1 | IOT6B | Configuration mode select |
| MODE2 | IOT5B | Configuration mode select |
| DONE | IOL14A | Configuration complete |
| READY | IOL14B | Ready for configuration |
| RECONFIG_N | IOL13B | Reconfiguration trigger |

### Global Clock Pins

| Signal | Pin | Bank | Description |
|--------|-----|------|-------------|
| GCLKT_0 | IOT28A | 0 | Global clock True |
| GCLKC_0 | IOT28B | 0 | Global clock Comp |
| GCLKT_1 | IOT29A | 0 | Global clock True |
| GCLKC_1 | IOT29B | 0 | Global clock Comp |
| GCLKT_2 | IOR9A | 1 | Global clock True |
| GCLKC_2 | IOR9B | 1 | Global clock Comp |

### PLL Input Pins

| Signal | Pin | Bank | Description |
|--------|-----|------|-------------|
| RPLL_T_in | IOR5A | 1 | Right PLL clock input True |
| RPLL_C_in | IOR5B | 1 | Right PLL clock input Comp |
| LPLL_T_in | IOL5A | 3 | Left PLL clock input True |
| LPLL_C_in | IOL5B | 3 | Left PLL clock input Comp |

---

## Power Supply Requirements

### QN88P Package (with PSRAM)

| Supply | Pins | Min | Max | Description |
|--------|------|-----|-----|-------------|
| VCC | Core | 1.14V | 1.26V | Core voltage |
| VCCO0 | Bank 0 | 1.14V | 3.6V | I/O voltage |
| VCCO1 | Bank 1 | 1.14V | 3.6V | I/O voltage |
| VCCO2 | Bank 2 | 1.14V | 3.6V | I/O voltage |
| VCCO3 | Bank 3 | 1.14V | 3.6V | I/O voltage + PSRAM |
| VCCX | Aux | 2.375V | 3.6V | Auxiliary voltage |

**Note**: On Tang Nano 9K, VCCO is typically 3.3V for most banks.

---

## Drive Strength Table (LVCMOS33)

| Setting | Typical Current | Use Case |
|---------|-----------------|----------|
| DRIVE=4 | 4 mA | Low power, LED |
| DRIVE=8 | 8 mA | Standard I/O |
| DRIVE=12 | 12 mA | Moderate load |
| DRIVE=16 | 16 mA | Heavy load |
| DRIVE=24 | 24 mA | High current |

---

## Best Practices

1. **Match VCCO to I/O standard**: LVCMOS33 requires 3.3V VCCO
2. **Use pull-ups on unused inputs**: Prevents floating inputs
3. **Slow slew rate for EMI**: Use SLOW unless speed is critical
4. **Hysteresis for noisy signals**: Enable for buttons, external interfaces
5. **Group signals by bank**: Minimize bank voltage conflicts
6. **Use dedicated clock pins**: Better timing for clock inputs

---

## Related Files

| File | Description |
|------|-------------|
| `hardware/constraints/atomik_constraints.cst` | Project physical constraints |
| [TANG_NANO_9K_PINOUT.md](TANG_NANO_9K_PINOUT.md) | Board-specific pin mapping |
| [TIMING_REFERENCE.md](TIMING_REFERENCE.md) | SDC timing constraints |

---

*Reference: UG289-1.9.2E, UG803-1.6E, DS117-2.9.3E*
