# Run dnsperf via WSL — do NOT paste bash lines into PowerShell.
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

$mode = if ($args.Count -gt 0) { $args[0] } else { "light" }

Write-Host "WSL repo: $repoWsl"
Write-Host "Mode: $mode (use 'stress' for heavy load)"
Write-Host "Tip: install dnsperf once in WSL: sudo apt install -y dnsperf"
wsl.exe bash -lc "cd '$repoWsl' && bash scripts/run_dnsperf.sh $mode"
