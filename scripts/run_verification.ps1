# 在 Windows PowerShell 中通过 WSL 运行验证脚本（勿在 PS 里使用 /mnt/c 路径）
$ErrorActionPreference = "Stop"
$repoWin = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path "$repoWin\Makefile")) {
    $repoWin = "C:\projects\DNS-Relay-Server"
}
$repoWsl = "/mnt/c/projects/DNS-Relay-Server"
if ($repoWin -match "^([A-Za-z]):\\(.*)$") {
    $drive = $Matches[1].ToLower()
    $rest = ($Matches[2] -replace "\\", "/")
    $repoWsl = "/mnt/$drive/$rest"
}

Write-Host "WSL repo: $repoWsl"
Write-Host "Tip: fix-B iptables needs root — script uses wsl -u root"
wsl.exe -u root bash -lc "cd '$repoWsl' && sed -i 's/\r$//' scripts/run_verification.sh && bash scripts/run_verification.sh"
