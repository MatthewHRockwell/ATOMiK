# =============================================================================
# ATOMiK Phase 3 RTL Test Suite (PowerShell)
# Runs all RTL unit and integration tests
# =============================================================================

$ErrorActionPreference = "Continue"

# Navigate to project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location "$ScriptDir\.."

Write-Host "=============================================="  -ForegroundColor Cyan
Write-Host "  ATOMiK Phase 3 RTL Test Suite"               -ForegroundColor Cyan
Write-Host "=============================================="  -ForegroundColor Cyan
Write-Host ""

# Check iverilog exists
$iverilog = Get-Command iverilog -ErrorAction SilentlyContinue
if (-not $iverilog) {
    Write-Host "ERROR: iverilog not found in PATH" -ForegroundColor Red
    Write-Host "Install: https://bleyer.org/icarus/" -ForegroundColor Yellow
    exit 1
}

$totalTests = 0
$totalPassed = 0
$totalFailed = 0

# Function to parse test output
function Parse-TestOutput {
    param([string[]]$Output)
    
    $result = @{ Tests = 0; Passed = 0; Failed = 0; AllPassed = $false }
    
    foreach ($line in $Output) {
        if ($line -match "Total Tests:\s+(\d+)") {
            $result.Tests = [int]$Matches[1]
        }
        if ($line -match "Passed:\s+(\d+)") {
            $result.Passed = [int]$Matches[1]
        }
        if ($line -match "Failed:\s+(\d+)") {
            $result.Failed = [int]$Matches[1]
        }
        if ($line -match "ALL TESTS PASSED") {
            $result.AllPassed = $true
        }
    }
    
    return $result
}

# -----------------------------------------------------------------------------
# Test 1: Delta Accumulator (atomik_delta_acc)
# -----------------------------------------------------------------------------
Write-Host "[1/3] Testing atomik_delta_acc..." -ForegroundColor Yellow

$compileArgs = @(
    "-o", "sim/tb_delta_acc.vvp",
    "-DSIMULATION",
    "rtl/atomik_delta_acc.v",
    "sim/tb_delta_acc.v"
)

& iverilog @compileArgs 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "      COMPILE FAILED" -ForegroundColor Red
    $totalFailed++
} else {
    $output = & vvp sim/tb_delta_acc.vvp 2>&1
    $result = Parse-TestOutput $output
    
    $totalTests += $result.Tests
    $totalPassed += $result.Passed
    $totalFailed += $result.Failed
    
    if ($result.AllPassed) {
        Write-Host "      PASSED ($($result.Tests) tests)" -ForegroundColor Green
    } else {
        Write-Host "      FAILED ($($result.Failed) of $($result.Tests) tests)" -ForegroundColor Red
        $output | Select-String "FAIL" | ForEach-Object { Write-Host "      $_" -ForegroundColor Red }
    }
}
Remove-Item "sim/tb_delta_acc.vvp" -ErrorAction SilentlyContinue

# -----------------------------------------------------------------------------
# Test 2: State Reconstructor (atomik_state_rec)
# -----------------------------------------------------------------------------
Write-Host "[2/3] Testing atomik_state_rec..." -ForegroundColor Yellow

$compileArgs = @(
    "-o", "sim/tb_state_rec.vvp",
    "rtl/atomik_state_rec.v",
    "sim/tb_state_rec.v"
)

& iverilog @compileArgs 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "      COMPILE FAILED" -ForegroundColor Red
    $totalFailed++
} else {
    $output = & vvp sim/tb_state_rec.vvp 2>&1
    $result = Parse-TestOutput $output
    
    $totalTests += $result.Tests
    $totalPassed += $result.Passed
    $totalFailed += $result.Failed
    
    if ($result.AllPassed) {
        Write-Host "      PASSED ($($result.Tests) tests)" -ForegroundColor Green
    } else {
        Write-Host "      FAILED ($($result.Failed) of $($result.Tests) tests)" -ForegroundColor Red
        $output | Select-String "FAIL" | ForEach-Object { Write-Host "      $_" -ForegroundColor Red }
    }
}
Remove-Item "sim/tb_state_rec.vvp" -ErrorAction SilentlyContinue

# -----------------------------------------------------------------------------
# Test 3: Core v2 Integration (atomik_core_v2)
# -----------------------------------------------------------------------------
Write-Host "[3/3] Testing atomik_core_v2..." -ForegroundColor Yellow

$compileArgs = @(
    "-o", "sim/tb_core_v2.vvp",
    "-DSIMULATION",
    "rtl/atomik_delta_acc.v",
    "rtl/atomik_state_rec.v",
    "rtl/atomik_core_v2.v",
    "sim/tb_core_v2.v"
)

& iverilog @compileArgs 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "      COMPILE FAILED" -ForegroundColor Red
    $totalFailed++
} else {
    $output = & vvp sim/tb_core_v2.vvp 2>&1
    $result = Parse-TestOutput $output
    
    $totalTests += $result.Tests
    $totalPassed += $result.Passed
    $totalFailed += $result.Failed
    
    if ($result.AllPassed) {
        Write-Host "      PASSED ($($result.Tests) tests)" -ForegroundColor Green
    } else {
        Write-Host "      FAILED ($($result.Failed) of $($result.Tests) tests)" -ForegroundColor Red
        $output | Select-String "FAIL" | ForEach-Object { Write-Host "      $_" -ForegroundColor Red }
    }
}
Remove-Item "sim/tb_core_v2.vvp" -ErrorAction SilentlyContinue

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
Write-Host ""
Write-Host "=============================================="  -ForegroundColor Cyan
Write-Host "  Test Summary"                                 -ForegroundColor Cyan
Write-Host "=============================================="  -ForegroundColor Cyan
Write-Host "  Total Tests:  $totalTests"
Write-Host "  Passed:       $totalPassed" -ForegroundColor Green
Write-Host "  Failed:       $totalFailed" -ForegroundColor $(if ($totalFailed -eq 0) { "Green" } else { "Red" })
Write-Host ""

if ($totalFailed -eq 0 -and $totalTests -gt 0) {
    Write-Host "  *** ALL PHASE 3 RTL TESTS PASSED ***" -ForegroundColor Green
    Write-Host ""
    exit 0
} else {
    Write-Host "  *** SOME TESTS FAILED ***" -ForegroundColor Red
    Write-Host ""
    exit 1
}
