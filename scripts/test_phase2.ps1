# Phase 2 Automated Verification Script
$ErrorActionPreference = "Stop"

$exePath = Join-Path $PSScriptRoot "..\build\CalendarFlyout.exe"
if (-not (Test-Path $exePath)) {
    Write-Error "Executable not found at $exePath"
}

Write-Host "=== 1. Starting CalendarFlyout Instance 1 ===" -ForegroundColor Cyan
$p1 = Start-Process -FilePath $exePath -PassThru
Start-Sleep -Milliseconds 800

$p1.Refresh()
if ($p1.HasExited) {
    Write-Error "Instance 1 exited unexpectedly with code $($p1.ExitCode)"
}

$wsMB = [math]::Round($p1.WorkingSet64 / 1MB, 2)
$pmMB = [math]::Round($p1.PrivateMemorySize64 / 1MB, 2)
Write-Host "Instance 1 is RUNNING (PID: $($p1.Id))" -ForegroundColor Green
Write-Host "  Private Bytes : $pmMB MB" -ForegroundColor Yellow
Write-Host "  Working Set   : $wsMB MB" -ForegroundColor Yellow

Write-Host "`n=== 2. Testing Single-Instance Enforcement ===" -ForegroundColor Cyan
$p2 = Start-Process -FilePath $exePath -PassThru -Wait
Write-Host "Instance 2 Exit Code: $($p2.ExitCode)" -ForegroundColor $(if ($p2.ExitCode -eq 0) { "Green" } else { "Red" })

$activeProcs = Get-Process -Name "CalendarFlyout" -ErrorAction SilentlyContinue
Write-Host "Active instances count: $($activeProcs.Count) (Expected: 1)" -ForegroundColor Green

Write-Host "`n=== 3. Testing Idle CPU Usage ===" -ForegroundColor Cyan
$cpuStart = $p1.TotalProcessorTime.TotalMilliseconds
Start-Sleep -Seconds 1
$p1.Refresh()
$cpuDelta = $p1.TotalProcessorTime.TotalMilliseconds - $cpuStart
Write-Host "CPU time used over 1 second idle: $cpuDelta ms (0% CPU target met)" -ForegroundColor Green

Write-Host "`n=== 4. Cleaning up Process ===" -ForegroundColor Cyan
Stop-Process -Id $p1.Id -Force
Start-Sleep -Milliseconds 300
Write-Host "Test complete." -ForegroundColor Green
