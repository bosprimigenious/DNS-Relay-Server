# 教室验收：Windows 原生编译 + 本地冒烟 + 启动命令
param(
    [string]$BindIp = "0.0.0.0",
    [int]$Port = 53,
    [string]$HostsFile = "参考资料\dnsrelay.txt"
)

$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$win = Join-Path $repo "platform\windows"
Set-Location $repo

Write-Host "=== Build (native Windows) ===" -ForegroundColor Cyan
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $win "build.ps1") -Clean
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $win "build.ps1")

Write-Host ""
Write-Host "=== Smoke test 127.0.0.1:15353 ===" -ForegroundColor Cyan
$proc = Start-Process -FilePath ".\dnsrelay.exe" -ArgumentList @(
    "-b", "127.0.0.1", "-p", "15353", "-f", $HostsFile, "-v"
) -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 1

foreach ($name in @("bupt", "008.cn", "baidu.com")) {
    Write-Host ">> nslookup $name 127.0.0.1 -port=15353" -ForegroundColor Yellow
    $out = nslookup $name 127.0.0.1 -port=15353 2>&1 | Out-String
    Write-Host $out
    if ($out -match "DNS request timed out") {
        Write-Host "WARN: timeout for $name" -ForegroundColor Red
    } else {
        Write-Host "OK: $name" -ForegroundColor Green
    }
}

Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "=== Classroom (Admin PowerShell) ===" -ForegroundColor Cyan
Write-Host ".\dnsrelay.exe -b $BindIp -p $Port -f $HostsFile -v"
$ip = (Get-NetIPAddress -AddressFamily IPv4 |
    Where-Object { $_.IPAddress -notlike "127.*" -and $_.PrefixOrigin -ne "WellKnown" } |
    Select-Object -First 1).IPAddress
if ($ip) {
    Write-Host "LAN IP for classmates: $ip" -ForegroundColor Green
    Write-Host "nslookup bupt $ip"
}
