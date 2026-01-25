# =============================================================================
# ATOMiK Synthesis Runner (PowerShell - Windows)
#
# Usage: .\run_synthesis.ps1 [-GUI]
#
# Options:
#   -GUI    Open Gowin EDA GUI instead of command-line synthesis
#
# Prerequisites:
#   - Gowin EDA installed
#   - GOWIN_HOME environment variable set, or Gowin in PATH
#
# =============================================================================

param(
    [switch]$GUI
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

Write-Host "=============================================="  -ForegroundColor Cyan
Write-Host " ATOMiK Synthesis Runner"                       -ForegroundColor Cyan
Write-Host "=============================================="  -ForegroundColor Cyan
Write-Host ""

# Find Gowin EDA installation
$GowinHome = $env:GOWIN_HOME
if (-not $GowinHome) {
    # Try common installation paths
    $CommonPaths = @(
        "C:\Gowin\Gowin_V1.9.11.03_x64\IDE",
        "C:\Gowin\Gowin_V1.9.11.03_Education_x64\IDE",
        "$env:LOCALAPPDATA\Gowin\Gowin_V*\IDE"
    )
    
    foreach ($path in $CommonPaths) {
        $resolved = Resolve-Path $path -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($resolved -and (Test-Path "$resolved\bin\gw_sh.exe")) {
            $GowinHome = $resolved.Path
            break
        }
    }
}

if (-not $GowinHome -or -not (Test-Path "$GowinHome\bin\gw_sh.exe")) {
    Write-Host "ERROR: Gowin EDA not found" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please either:" -ForegroundColor Yellow
    Write-Host "  1. Set GOWIN_HOME environment variable to Gowin IDE directory"
    Write-Host "  2. Install Gowin EDA to default location"
    Write-Host ""
    Write-Host "Download: https://www.gowinsemi.com/en/support/download_eda/"
    exit 1
}

$GwSh = "$GowinHome\bin\gw_sh.exe"
$GwIde = "$GowinHome\bin\gw_ide.exe"

Write-Host "Gowin EDA: $GowinHome"
Write-Host "Project:   $ProjectRoot"
Write-Host ""

# GUI mode
if ($GUI) {
    Write-Host "Opening Gowin EDA GUI..."
    
    $ProjectFile = "$ProjectRoot\ATOMiK.gprj"
    if (Test-Path $ProjectFile) {
        Start-Process $GwIde -ArgumentList "`"$ProjectFile`""
    } else {
        Start-Process $GwIde
    }
    
    exit 0
}

# Command-line synthesis
Write-Host "Running command-line synthesis..."
Write-Host ""

Push-Location $ScriptDir
try {
    & $GwSh gowin_synth.tcl
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Host "ERROR: Synthesis failed with exit code $LASTEXITCODE" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}

Write-Host ""
Write-Host "=============================================="  -ForegroundColor Green
Write-Host " Synthesis Complete"                            -ForegroundColor Green
Write-Host "=============================================="  -ForegroundColor Green
Write-Host ""
Write-Host "Output files:"
Write-Host "  Bitstream: $ProjectRoot\impl\pnr\ATOMiK.fs"
Write-Host "  Netlist:   $ProjectRoot\impl\gwsynthesis\ATOMiK.vg"
Write-Host ""
