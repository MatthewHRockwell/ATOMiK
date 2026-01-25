# =============================================================================
# ATOMiK Delta Accumulator Test Script (PowerShell)
# =============================================================================

$ErrorActionPreference = "Stop"

# Navigate to project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location "$ScriptDir\.."

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "ATOMiK Delta Accumulator Test" -ForegroundColor Cyan
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
    "-o", "sim/tb_delta_acc.vvp",
    "-DSIMULATION",
    "rtl/atomik_delta_acc.v",
    "sim/tb_delta_acc.v"
)

& iverilog @compileArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAILED: Compilation errors" -ForegroundColor Red
    exit 1
}
Write-Host "  Compilation successful" -ForegroundColor Green

# Run simulation
Write-Host "`n[2/2] Running simulation..." -ForegroundColor Yellow
& vvp sim/tb_delta_acc.vvp

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nSimulation returned non-zero exit code" -ForegroundColor Yellow
}

# Cleanup
Remove-Item "sim/tb_delta_acc.vvp" -ErrorAction SilentlyContinue

Write-Host "`n==========================================" -ForegroundColor Cyan
Write-Host "Test complete" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
