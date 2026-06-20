# Native Windows build (no WSL). Requires MinGW-w64 gcc in PATH.
#   powershell -ExecutionPolicy Bypass -File platform\windows\build.ps1
param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
Set-Location $repo

$gcc = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $gcc) {
  throw "gcc not found. Install MSYS2 MinGW-w64 and add C:\msys64\mingw64\bin to PATH."
}

$buildDir = "build"
$target = "dnsrelay.exe"
$flags = @("-Wall", "-Wextra", "-g", "-std=c11", "-Iinclude")
$ldflags = @("-lws2_32")

if ($Clean) {
  if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir }
  if (Test-Path $target) { Remove-Item -Force $target }
  Write-Host "clean done"
  exit 0
}

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
$objs = @()
Get-ChildItem "src\*.c" | ForEach-Object {
  $obj = Join-Path $buildDir ($_.BaseName + ".o")
  & gcc @flags -c $_.FullName -o $obj
  if ($LASTEXITCODE -ne 0) { throw "compile failed: $($_.Name)" }
  $objs += $obj
}

& gcc @flags -o $target @objs @ldflags
if ($LASTEXITCODE -ne 0) { throw "link failed" }
Write-Host "Built $target"
