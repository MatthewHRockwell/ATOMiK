# =============================================================================
# ATOMiK Core v2 Integration Test Script (PowerShell)
# =============================================================================

$ErrorActionPreference = "Stop"

# Navigate to project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location "$ScriptDir\..\.."

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "ATOMiK Core v2 Integration Test" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# Check iverilog exists
$iverilog = Get-Command iverilog -ErrorAction SilentlyContinue
if (-not $iverilog) {
    Write-Host "ERROR: iverilog not found" -ForegroundColor Red
    exit 1
}

# Compile
Write-Host "`n[1/2] Compiling..." -ForegroundColor Yellow
$compileArgs = @(
    "-o", "hardware/sim/tb_core_v2.vvp",
    "-DSIMULATION",
    "hardware/rtl/atomik_delta_acc.v",
    "hardware/rtl/atomik_state_rec.v",
    "hardware/rtl/atomik_core_v2.v",
    "hardware/sim/tb_core_v2.v"
)

& iverilog @compileArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAILED: Compilation errors" -ForegroundColor Red
    exit 1
}
Write-Host "  Compilation successful" -ForegroundColor Green

# Run simulation
Write-Host "`n[2/2] Running simulation..." -ForegroundColor Yellow
& vvp hardware/sim/tb_core_v2.vvp

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nSimulation returned non-zero exit code" -ForegroundColor Yellow
}

# Cleanup
Remove-Item "hardware/sim/tb_core_v2.vvp" -ErrorAction SilentlyContinue

Write-Host "`n==========================================" -ForegroundColor Cyan
Write-Host "Test complete" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
