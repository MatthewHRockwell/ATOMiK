#!/bin/bash
# =============================================================================
# ATOMiK Synthesis Runner (Gowin EDA)
#
# Usage: ./run_synthesis.sh [--gui]
#
# Options:
#   --gui    Open Gowin EDA GUI instead of command-line synthesis
#
# Prerequisites:
#   - Gowin EDA installed and in PATH (gw_sh)
#   - Valid license configured
#
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=============================================="
echo " ATOMiK Synthesis Runner"
echo "=============================================="
echo ""

# Check for Gowin tools
if ! command -v gw_sh &> /dev/null; then
    echo "ERROR: gw_sh not found in PATH"
    echo ""
    echo "Please install Gowin EDA and add to PATH:"
    echo "  export PATH=\$PATH:/path/to/gowin/IDE/bin"
    echo ""
    exit 1
fi

# Parse arguments
if [[ "$1" == "--gui" ]]; then
    echo "Opening Gowin EDA GUI..."
    
    # Check if project file exists
    if [[ -f "$PROJECT_ROOT/ATOMiK.gprj" ]]; then
        gw_ide "$PROJECT_ROOT/ATOMiK.gprj" &
    else
        echo "No project file found. Opening IDE..."
        gw_ide &
    fi
    
    exit 0
fi

# Command-line synthesis
echo "Running command-line synthesis..."
echo ""

cd "$SCRIPT_DIR"
gw_sh gowin_synth.tcl

echo ""
echo "Synthesis complete!"
