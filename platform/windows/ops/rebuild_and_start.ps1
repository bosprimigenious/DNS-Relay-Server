# 强制结束旧进程 + 重新编译 + 启动（管理员 PowerShell）
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$win = Join-Path $root "platform\windows"
Set-Location $root

Write-Host "=== 1. 结束旧 dnsrelay ===" -ForegroundColor Cyan
Get-Process -Name "dnsrelay" -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

Write-Host "=== 2. 编译 ===" -ForegroundColor Cyan
if (Get-Command mingw32-make -ErrorAction SilentlyContinue) {
    mingw32-make clean
    mingw32-make
} elseif (Get-Command make -ErrorAction SilentlyContinue) {
    make clean
    make
} else {
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $win "build.ps1") -Clean
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $win "build.ps1")
}

$exe = Join-Path $root "dnsrelay.exe"
if (-not (Test-Path $exe)) {
    Write-Host "编译失败：找不到 dnsrelay.exe" -ForegroundColor Red
    Write-Host "请在 UCRT64 终端执行: cd /c/Fullstack_Development/DNS-Relay-Server && make clean && make"
    exit 1
}

$built = (Get-Item $exe).LastWriteTime
Write-Host "dnsrelay.exe 时间戳: $built" -ForegroundColor Green

Write-Host ""
Write-Host "=== 3. 启动（须看到 build tag: win-fast3）===" -ForegroundColor Cyan
$hosts = Join-Path $root "参考资料\dnsrelay.txt"
& $exe -b 0.0.0.0 -p 53 -f $hosts -v
