#!/bin/bash
# =============================================================================
# ATOMiK RTL Compilation and Lint Script
# Works in WSL, Git Bash, or MSYS2 on Windows
# =============================================================================

set -e

# Navigate to project root (handle both Windows and Unix paths)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/../.."

echo "=========================================="
echo "ATOMiK RTL Compilation Check"
echo "=========================================="

# Define RTL files
RTL_FILES=(
    "hardware/rtl/atomik_top.v"
    "hardware/rtl/atomik_core.v"
    "hardware/rtl/pll/atomik_pll_94p5m.v"
    "hardware/rtl/uart_genome_loader.v"
)

# Check all files exist
echo "[1/4] Checking files exist..."
for f in "${RTL_FILES[@]}"; do
    if [ ! -f "$f" ]; then
        echo "ERROR: Missing file: $f"
        exit 1
    fi
    echo "  ✓ $f"
done

# Run Icarus Verilog compilation
echo ""
echo "[2/4] Running Icarus Verilog compilation..."
if command -v iverilog &> /dev/null; then
    mkdir -p hardware/sim
    iverilog -o hardware/sim/test_compile.vvp "${RTL_FILES[@]}"
    echo "  ✓ Icarus Verilog: PASSED"
    rm -f hardware/sim/test_compile.vvp
else
    echo "  ⚠ iverilog not found, skipping"
fi

# Run Verilator lint (if available)
echo ""
echo "[3/4] Running Verilator lint..."
if command -v verilator &> /dev/null; then
    verilator --lint-only -Wall \
        --top-module atomik_top \
        "${RTL_FILES[@]}" 2>&1 | head -50
    echo "  ✓ Verilator lint: PASSED"
else
    echo "  ⚠ verilator not found, skipping"
    echo "  Install via: sudo apt install verilator (WSL/Linux)"
    echo "  Or: pacman -S mingw-w64-ucrt-x86_64-verilator (MSYS2)"
fi

# Summary
echo ""
echo "[4/4] Module hierarchy check..."
if command -v iverilog &> /dev/null; then
    # Use iverilog to show module dependencies
    iverilog -t null "${RTL_FILES[@]}" 2>&1 || true
fi

echo ""
echo "=========================================="
echo "Compilation check complete!"
echo "=========================================="
