# libatomik Changelog

## 1.0.0 (2026-04-08)

### Added
- `ATOMIK_LAYOUT_ADAPTER` backend for CFU adapter Wishbone wrapper
- `atomik_detect_changed()` convenience function for change detection
- Mock backend for `atomik_open_devmem()` (compile with `-DATOMIK_MOCK`)
- `atomik_version_string()` function
- `LIBATOMIK_VERSION_MAJOR/MINOR/PATCH` defines
- 59 mock tests (was 33)
- `ACCUMULATOR_MODEL.md` documenting single-bank architecture

### Backends
- **CSR** (LiteX Wishbone): direct register writes, HI triggers operation
- **AXI** (ARM GP0): same protocol, different register offsets
- **Adapter** (CFU Wishbone): CMD-based protocol, LO/HI pair for 64-bit on RV32

### Validated
- 16/16 PASS from Linux userspace (CSR path)
- 9/9 PASS from Linux userspace (adapter path)
- 20/20 PASS in Verilator simulation (adapter)
- 7/7 correct demo_state_monitor on live Zynq hardware
- 108 Lean4 theorems proving the underlying algebra
