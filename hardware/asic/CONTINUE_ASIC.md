# Continue ASIC Tape-Out Session

**Last updated**: March 18, 2026

---

## Context

We validated ATOMiK for ASIC tape-out on chipIgnite (ChipFoundry) using the SKY130 130nm process. The full RTL-to-GDSII flow passed with zero DRC/LVS/timing violations.

## What's Complete

### Tooling Installed
- **Yosys** 0.38 at `/usr/bin/yosys` — RTL synthesis
- **Docker** 28.2.2 — container runtime for OpenLane
- **OpenLane** 2023.07.19-1 — Docker image `efabless/openlane:2023.07.19-1`
- **SKY130 PDK** — installed via volare at `hardware/asic/caravel_atomik/dependencies/pdks/`
- **ARM cross-compiler** — `arm-linux-gnueabihf-gcc` 13.3.0

### RTL Written & Verified
| File | Purpose | Tests |
|------|---------|-------|
| `hardware/asic/rtl/atomik_wb_wrapper.v` | Wishbone B4 slave, 32-bit ↔ 64-bit LO/HI register map | 15/15 PASS |
| `hardware/asic/rtl/user_project_wrapper.v` | Caravel top-level (Wishbone + LA + GPIO + IRQ) | 15/15 PASS |
| `hardware/v3/rtl/atomik_v3_atomik.v` | ATOMiK core (shared, ASIC-portable) | 112+ PASS |

### Synthesis Results (Yosys → SKY130 HD)
- **Area**: 0.614 mm2 (6.1% of Caravel's 10mm2 user area)
- **Cells**: 59,852 total (16,521 flip-flops + 43,331 logic)

### Place & Route Results (OpenLane full flow)
- **Timing**: Setup slack +17.76ns (71% margin at 40 MHz, could run ~138 MHz)
- **Hold**: +0.34ns slack (PASS)
- **DRC**: 0 violations (CLEAN)
- **LVS**: PASS
- **Power**: 1.13 mW at 40 MHz
- **Area**: 0.81 mm2 die (8.1% of Caravel user area)
- **GDS**: Generated and ready

### Caravel Project Structure
```
hardware/asic/caravel_atomik/
├── verilog/rtl/
│   ├── atomik_wb_wrapper.v      # Our Wishbone wrapper
│   ├── atomik_v3_atomik.v       # Our ATOMiK core
│   └── user_project_wrapper.v   # Our Caravel top-level
├── openlane/atomik_core/
│   └── config.json              # OpenLane config (25ns clock, 900x900 die)
├── dependencies/
│   ├── pdks/                    # SKY130A + SKY130B (1.9GB)
│   └── openlane_src/            # OpenLane source
└── openlane/atomik_core/runs/atomik_run1/  # P&R results
    ├── results/final/           # GDS, LEF, DEF, netlist
    └── reports/                 # Timing, power, area, DRC, LVS
```

---

## What's Next

### If proceeding with $500 deposit:
1. **Reserve spot** on CI2609 (September 2026) shuttle at chipfoundry.io/reservations
2. **Scale up to N=4 or N=16 banks** — we have 91.9% area remaining
3. **Integrate hardened macro into user_project_wrapper** — update wrapper config to reference our .lef/.gds/.lib
4. **Run wrapper-level P&R** — the full Caravel integration flow
5. **Write Caravel firmware** — C code for the management SoC (PicoRV32) to exercise ATOMiK via Wishbone
6. **Run mpw-precheck** — Caravel's pre-submission validation suite
7. **Submit GDS** to chipIgnite portal

### If exploring multi-bank first:
- Modify `openlane/atomik_core/config.json` to use `atomik_core_zynq_parallel.v` with N_BANKS=4 or N_BANKS=16
- Re-run OpenLane: `docker run --rm -v ... efabless/openlane:2023.07.19-1 flow.tcl -design .../atomik_core -tag atomik_n4 -overwrite`
- Compare area/timing/power vs single-bank

### Key Commands
```bash
# Run OpenLane on ATOMiK
cd hardware/asic/caravel_atomik
export PDK_ROOT=$(pwd)/dependencies/pdks
export PDK=sky130A
docker run --rm \
  -v $(pwd):$(pwd) \
  -v $PDK_ROOT:$PDK_ROOT \
  -e PDK_ROOT=$PDK_ROOT \
  -e PDK=$PDK \
  -w $(pwd)/openlane \
  efabless/openlane:2023.07.19-1 \
  flow.tcl -design $(pwd)/openlane/atomik_core \
  -save_path $(pwd) -save -tag atomik_run1 -overwrite

# Run Wishbone testbench
cd hardware/asic
iverilog -o sim/tb_wb_wrapper.vvp -DMPRJ_IO_PADS=38 \
  rtl/atomik_wb_wrapper.v rtl/user_project_wrapper.v \
  ../v3/rtl/atomik_v3_atomik.v sim/tb_wb_wrapper.v
vvp sim/tb_wb_wrapper.vvp

# Standalone Yosys synthesis (quick check)
cd hardware/asic
yosys synth_caravel.ys
```

---

## chipIgnite Details
- **Platform**: chipfoundry.io / chipIgnite
- **Process**: SKY130 (130nm)
- **Cost**: $14,950 total ($500 deposit → 50% at -60 days → balance at -14 days)
- **Shuttles**: CI2605 (May 2026), CI2609 (September 2026)
- **What you get**: 100 QFN packaged chips + eval board + 10 M.2 daughter cards
- **SoC**: Caravel — PicoRV32 RISC-V + Wishbone bus + 38 GPIOs + 128-bit LA
- **User area**: ~10mm2 (ATOMiK uses 8.1%)
- **Timeline**: ~5 months from submission to silicon
