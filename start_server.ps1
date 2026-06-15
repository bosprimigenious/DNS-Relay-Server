# 教室 DNS 服务端一键启动（管理员 PowerShell）
# 用法：右键“以管理员身份运行”或在管理员窗口执行：
#   powershell -ExecutionPolicy Bypass -File C:\Fullstack_Development\DNS-Relay-Server\start_server.ps1
$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
Set-Location $root

if (-not (Test-Path "$root\dnsrelay.exe")) {
    Write-Host "dnsrelay.exe 不存在，正在编译..." -ForegroundColor Yellow
    if (Get-Command mingw32-make -ErrorAction SilentlyContinue) {
        mingw32-make clean
        mingw32-make
    } else {
        & powershell -NoProfile -ExecutionPolicy Bypass -File "$root\scripts\build.ps1" -Clean
        & powershell -NoProfile -ExecutionPolicy Bypass -File "$root\scripts\build.ps1"
    }
}

if (-not (Test-Path "$root\dnsrelay.exe")) {
    Write-Host "编译失败。请在 UCRT64 终端执行: cd /c/Fullstack_Development/DNS-Relay-Server && make" -ForegroundColor Red
    exit 1
}

$hosts = Join-Path $root "参考资料\dnsrelay.txt"
Write-Host "启动: $root\dnsrelay.exe" -ForegroundColor Cyan
Write-Host "配置: $hosts" -ForegroundColor Cyan
Write-Host "日志须含 relay mode: async；非 A 查询应显示 [FAST] 而非 [RELAY] in-addr.arpa" -ForegroundColor Cyan
Write-Host ""

& "$root\dnsrelay.exe" -b 0.0.0.0 -p 53 -f $hosts -v
