# 教室验收快速部署（Windows 服务端 + 同学 nslookup 客户端）
#
# 服务端（你的笔记本，需管理员 PowerShell）：
#   cd C:\Fullstack_Development\DNS-Relay-Server
#   mingw32-make -f Makefile.win clean
#   mingw32-make -f Makefile.win
#   .\dnsrelay.exe -b 0.0.0.0 -p 53 -f 参考资料\dnsrelay.txt -v
#
# 启动日志应含：relay mode: async
#
# 客户端（同学电脑，把 10.122.227.251 换成你机器在教室网的 IP）：
#   nslookup bupt 10.122.227.251
#   nslookup 008.cn 10.122.227.251
#   nslookup baidu.com 10.122.227.251
#
# 若仍见 timeout：确认服务端已是 async；关闭本机其它 DNS 代理/VPN。

param(
    [string]$BindIp = "0.0.0.0",
    [int]$Port = 53,
    [string]$HostsFile = "参考资料\dnsrelay.txt"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path $PSScriptRoot -Parent
Set-Location $repo

Write-Host "=== Build (MinGW) ===" -ForegroundColor Cyan
mingw32-make -f Makefile.win clean 2>$null
mingw32-make -f Makefile.win
if (-not (Test-Path ".\dnsrelay.exe")) {
    throw "build failed: dnsrelay.exe not found (install MinGW-w64, add mingw32-make to PATH)"
}

Write-Host ""
Write-Host "=== Local smoke test (127.0.0.1:15353) ===" -ForegroundColor Cyan
$proc = Start-Process -FilePath ".\dnsrelay.exe" -ArgumentList @(
    "-b", "127.0.0.1", "-p", "15353", "-f", $HostsFile, "-v"
) -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 1

function Test-Dns($name) {
    Write-Host ">> nslookup $name 127.0.0.1 -port=15353" -ForegroundColor Yellow
    $out = nslookup $name 127.0.0.1 -port=15353 2>&1 | Out-String
    Write-Host $out
    if ($out -match "DNS request timed out") {
        Write-Host "WARN: timeout still present for $name" -ForegroundColor Red
    } else {
        Write-Host "OK: no timeout for $name" -ForegroundColor Green
    }
}

Test-Dns "bupt"
Test-Dns "008.cn"
Test-Dns "baidu.com"

Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "=== Classroom server command (Admin PowerShell) ===" -ForegroundColor Cyan
Write-Host ".\dnsrelay.exe -b $BindIp -p $Port -f $HostsFile -v"
$ip = (Get-NetIPAddress -AddressFamily IPv4 |
    Where-Object { $_.IPAddress -notlike "127.*" -and $_.PrefixOrigin -ne "WellKnown" } |
    Select-Object -First 1).IPAddress
if ($ip) {
    Write-Host "Your LAN IP (tell classmates): $ip" -ForegroundColor Green
    Write-Host "nslookup bupt $ip"
}
