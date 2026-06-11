# 一键：WSL 全量验证 + 生成 14 张终端 PNG + 编译 PDF
# 用法（PowerShell）：
#   cd C:\projects\DNS-Relay-Server
#   .\scripts\verify_and_screenshot.ps1
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

Write-Host "=== Step 1/3: run_verification.sh (WSL root, fix-B needs iptables) ==="
Write-Host "Repo: $repoWsl"
wsl.exe -u root bash -lc "cd '$repoWsl' && sed -i 's/\r$//' scripts/run_verification.sh && bash scripts/run_verification.sh"

Write-Host "=== Step 2/3: gen_terminal_screenshots.py ==="
Set-Location $repoWin
python scripts/gen_terminal_screenshots.py

if (Get-Command typst -ErrorAction SilentlyContinue) {
    Write-Host "=== Step 3/3: typst compile ==="
    typst compile --pdf-standard a-2u 实验报告.typ 实验报告.pdf
    Write-Host "Done: 实验报告.pdf"
} else {
    Write-Host "=== Step 3/3: skip typst (not in PATH) ==="
    Write-Host "Install typst or run: typst compile 实验报告.typ 实验报告.pdf"
}

Write-Host "Screenshots: $repoWin\docs\screenshots\terminal-01-build.png ... terminal-14-fix-b.png"
