# =============================================================================
# ATOMiK State Reconstructor Test Script (PowerShell)
# =============================================================================

$ErrorActionPreference = "Stop"

# Navigate to project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location "$ScriptDir\.."

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "ATOMiK State Reconstructor Test" -ForegroundColor Cyan
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
    "-o", "sim/tb_state_rec.vvp",
    "rtl/atomik_state_rec.v",
    "sim/tb_state_rec.v"
)

& iverilog @compileArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAILED: Compilation errors" -ForegroundColor Red
    exit 1
}
Write-Host "  Compilation successful" -ForegroundColor Green

# Run simulation
Write-Host "`n[2/2] Running simulation..." -ForegroundColor Yellow
& vvp sim/tb_state_rec.vvp

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nSimulation returned non-zero exit code" -ForegroundColor Yellow
}

# Cleanup
Remove-Item "sim/tb_state_rec.vvp" -ErrorAction SilentlyContinue

Write-Host "`n==========================================" -ForegroundColor Cyan
Write-Host "Test complete" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
