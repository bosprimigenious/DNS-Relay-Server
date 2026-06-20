# Windows 构建入口：委托 Makefile，避免与 Linux 双份维护编译规则
# 用法：. .\platform\windows\lib\Build-DnsRelay.ps1
#       Build-DnsRelay
#       Build-DnsRelay -Clean

function Get-DnsRelayRepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
}

function Get-DnsRelayBinaryPath {
    $root = Get-DnsRelayRepoRoot
    return Join-Path $root "dnsrelay.exe"
}

function Stop-DnsRelayProcess {
    Get-Process -Name "dnsrelay" -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Milliseconds 500
}

function Invoke-DnsRelayMake {
    param(
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$MakeArgs
    )

    if (Get-Command mingw32-make -ErrorAction SilentlyContinue) {
        & mingw32-make @MakeArgs
        return
    }
    if (Get-Command make -ErrorAction SilentlyContinue) {
        & make @MakeArgs
        return
    }
    throw "未找到 mingw32-make 或 make。请安装 MSYS2 MinGW-w64 并将 C:\msys64\mingw64\bin 加入 PATH。"
}

function Build-DnsRelay {
    param([switch]$Clean)

    $root = Get-DnsRelayRepoRoot
    Push-Location $root
    try {
        if ($Clean) {
            Invoke-DnsRelayMake clean
        }
        Invoke-DnsRelayMake
        $binary = Get-DnsRelayBinaryPath
        if (-not (Test-Path $binary)) {
            throw "编译完成但未找到 $binary"
        }
        Write-Host "Built $binary" -ForegroundColor Green
    }
    finally {
        Pop-Location
    }
}

Export-ModuleMember -Function Build-DnsRelay, Stop-DnsRelayProcess, Get-DnsRelayBinaryPath, Get-DnsRelayRepoRoot, Invoke-DnsRelayMake
