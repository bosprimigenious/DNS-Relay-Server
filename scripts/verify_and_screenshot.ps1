# 一键：Windows 原生验证 + 14 张终端 PNG +（可选）编译 PDF
#   cd C:\Fullstack_Development\DNS-Relay-Server
#   .\scripts\verify_and_screenshot.ps1
$ErrorActionPreference = "Stop"
$repo = Split-Path $PSScriptRoot -Parent
Set-Location $repo

Write-Host "=== Step 1/3: run_verification.ps1 (native Windows) ===" -ForegroundColor Cyan
& "$PSScriptRoot\run_verification.ps1"

Write-Host "=== Step 2/3: gen_terminal_screenshots.py ===" -ForegroundColor Cyan
python scripts\gen_terminal_screenshots.py

if (Get-Command typst -ErrorAction SilentlyContinue) {
    Write-Host "=== Step 3/3: typst report-async ===" -ForegroundColor Cyan
    Push-Location docs\report
    typst compile --root ..\.. 实验报告-异步.typ 实验报告-异步.pdf
    Pop-Location
    Write-Host "Done: docs\report\实验报告-异步.pdf"
} else {
    Write-Host "=== Step 3/3: skip typst (install typst for PDF) ===" -ForegroundColor Yellow
}

Write-Host "Screenshots: docs\screenshots\terminal-01-build.png ... terminal-14-fix-b.png"
