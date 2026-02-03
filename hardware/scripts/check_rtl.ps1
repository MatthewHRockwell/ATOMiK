# =============================================================================
# ATOMiK RTL Compilation and Lint Script (PowerShell)
# For native Windows with Icarus Verilog
# =============================================================================

$ErrorActionPreference = "Stop"

# Navigate to project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location "$ScriptDir\..\.."

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "ATOMiK RTL Compilation Check" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# Define RTL files (order matters - stubs first, then dependencies, then top)
$RTL_FILES = @(
    "hardware/sim/stubs/gowin_rpll_stub.v",   # Gowin primitive simulation stub
    "hardware/rtl/pll/atomik_pll_94p5m.v",    # PLL wrapper
    "hardware/rtl/uart_genome_loader.v",      # UART loader
    "hardware/rtl/atomik_core.v",             # Core logic
    "hardware/rtl/atomik_top.v"               # Top module
)

# Check all files exist
Write-Host "`n[1/3] Checking files exist..." -ForegroundColor Yellow
foreach ($f in $RTL_FILES) {
    if (Test-Path $f) {
        Write-Host "  OK $f" -ForegroundColor Green
    } else {
        Write-Host "  MISSING: $f" -ForegroundColor Red
        exit 1
    }
}

# Run Icarus Verilog compilation
Write-Host "`n[2/3] Running Icarus Verilog compilation..." -ForegroundColor Yellow
$iverilog = Get-Command iverilog -ErrorAction SilentlyContinue
if ($iverilog) {
    # Create sim directory if needed
    if (!(Test-Path "hardware/sim")) {
        New-Item -ItemType Directory -Path "hardware/sim" | Out-Null
    }

    # Run compilation
    $compileArgs = @("-o", "hardware/sim/test_compile.vvp") + $RTL_FILES
    & iverilog @compileArgs
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  PASSED: Icarus Verilog compilation successful" -ForegroundColor Green
        Remove-Item "hardware/sim/test_compile.vvp" -ErrorAction SilentlyContinue
    } else {
        Write-Host "  FAILED: Icarus Verilog compilation errors" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "  SKIP: iverilog not found" -ForegroundColor Yellow
    Write-Host "  Install from: https://bleyer.org/icarus/" -ForegroundColor Gray
}

# Run Verilator lint (if available)
Write-Host "`n[3/3] Running Verilator lint..." -ForegroundColor Yellow
$verilator = Get-Command verilator -ErrorAction SilentlyContinue
if ($verilator) {
    $lintArgs = @("--lint-only", "-Wall", "--top-module", "atomik_top") + $RTL_FILES
    & verilator @lintArgs
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  PASSED: Verilator lint" -ForegroundColor Green
    } else {
        Write-Host "  WARNINGS: Verilator lint (see above)" -ForegroundColor Yellow
    }
} else {
    Write-Host "  SKIP: verilator not found" -ForegroundColor Yellow
    Write-Host "  Install via MSYS2: pacman -S mingw-w64-ucrt-x86_64-verilator" -ForegroundColor Gray
    Write-Host "  Or use WSL: sudo apt install verilator" -ForegroundColor Gray
}

Write-Host "`n==========================================" -ForegroundColor Cyan
Write-Host "Compilation check complete!" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Note: The rPLL primitive is a Gowin hardware block." -ForegroundColor Gray
Write-Host "      sim/stubs/gowin_rpll_stub.v provides a behavioral model" -ForegroundColor Gray
Write-Host "      for simulation. Real synthesis uses Gowin EDA." -ForegroundColor Gray
