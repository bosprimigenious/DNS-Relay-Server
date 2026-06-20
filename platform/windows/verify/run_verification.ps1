# Windows 原生全量验证（不依赖 WSL / 虚拟机）
# 用法（PowerShell，项目根目录）：
#   .\platform\windows\verify\run_verification.ps1
param(
    [string]$Bind = "127.0.0.1",
    [int]$Port = 15353,
    [string]$HostsFile = "参考资料\dnsrelay.txt"
)

$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$py = Join-Path $repo "platform\common\python"
$win = Join-Path $repo "platform\windows"
Set-Location $repo
$out = Join-Path $repo "docs\verification"
New-Item -ItemType Directory -Force -Path $out | Out-Null

function Expand-Tabs([string]$Text) {
    return ($Text -replace "`t", "    ")
}

function Run-Cmd([string]$Label, [scriptblock]$Block) {
    Expand-Tabs "$Label"
    Expand-Tabs (& $Block | Out-String)
    ""
}

function Invoke-LoggedExternal {
    param([scriptblock]$Block)
    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & $Block
    } finally {
        $ErrorActionPreference = $prevEap
    }
}

function Run-Nslookup([string]$Name, [string]$Qtype = "") {
    if ($Qtype -ne "") {
        $cmd = "nslookup -port=$Port -type=$Qtype $Name $Bind"
        $r = Invoke-LoggedExternal { nslookup -port=$Port -type=$Qtype $Name $Bind 2>&1 }
    } else {
        $cmd = "nslookup -port=$Port $Name $Bind"
        $r = Invoke-LoggedExternal { nslookup -port=$Port $Name $Bind 2>&1 }
    }
    Expand-Tabs "`$ $cmd"
    Expand-Tabs ($r | Out-String)
    ""
}

function Stop-Relay {
    Get-Process -Name "dnsrelay" -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Milliseconds 500
}

function Resolve-HostsFilePath {
    $rootCopy = Join-Path $repo "dnsrelay.txt"
    if (Test-Path -LiteralPath $rootCopy) {
        return (Get-Item -LiteralPath $rootCopy).FullName
    }
    $found = Get-ChildItem -Path $repo -Recurse -Filter "dnsrelay.txt" -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -notmatch "\\dist\\" } |
        Select-Object -First 1
    if ($found) {
        return $found.FullName
    }
    throw "dnsrelay.txt not found under $repo"
}

function Start-Relay([string[]]$ExtraArgs) {
    $hostsAbs = Resolve-HostsFilePath
    $args = @("-b", $Bind, "-p", "$Port", "-f", $hostsAbs, "-v") + $ExtraArgs
    return Start-Process -FilePath (Join-Path $repo "dnsrelay.exe") -ArgumentList $args -PassThru `
        -WorkingDirectory $repo `
        -RedirectStandardOutput "$out\02-server-stdout.log" `
        -RedirectStandardError "$out\02-server-startup.log" `
        -WindowStyle Hidden
}

Stop-Relay

# Screenshot 1: build
{
    "`$ powershell -File platform\windows\build.ps1 -Clean"
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $win "build.ps1") -Clean
    "`$ powershell -File platform\windows\build.ps1"
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $win "build.ps1")
} | ForEach-Object { Expand-Tabs $_ } | Set-Content "$out\01-build.log" -Encoding utf8

if (-not (Test-Path ".\dnsrelay.exe")) {
    throw "dnsrelay.exe not found after build"
}

$proc = Start-Relay @()
Start-Sleep -Seconds 2

$log = New-Object System.Text.StringBuilder

function Add-Log([string]$s) { [void]$log.AppendLine((Expand-Tabs $s)) }

Add-Log "Screenshot 2: server startup (stderr + stdout)"
Add-Log "`$ .\dnsrelay.exe -b $Bind -p $Port -f $(Resolve-HostsFilePath) -v"
Add-Log "--- stderr (config load) ---"
Add-Log (Get-Content "$out\02-server-startup.log" -Raw -ErrorAction SilentlyContinue)
Add-Log "--- stdout (listen) ---"
Add-Log (Get-Content "$out\02-server-stdout.log" -Raw -ErrorAction SilentlyContinue)
Add-Log ""

Add-Log "Screenshot 3: course case1 nslookup bupt local A"
Add-Log (Run-Nslookup "bupt")

Add-Log "Screenshot 4: course case2 nslookup 008.cn block NXDOMAIN"
Add-Log (Run-Nslookup "008.cn")

Add-Log "Screenshot 5: course case3 nslookup baidu.com upstream relay"
Add-Log (Run-Nslookup "baidu.com")

Add-Log "Screenshot 6: config test0 block (0.0.0.0 test0)"
Add-Log (Run-Nslookup "test0")

Add-Log "Screenshot 7: config test1 local (11.111.11.111 test1)"
Add-Log (Run-Nslookup "test1")

Add-Log "Screenshot 8: nslookup sina second local record"
Add-Log (Run-Nslookup "sina")

Add-Log "Screenshot 9: fix-A nslookup mx bupt empty NOERROR"
Add-Log (Run-Nslookup "bupt" "mx")

Add-Log "Screenshot 10: dns_query.py protocol check"
Add-Log "`$ python platform\common\python\dns_query.py $Bind $Port bupt 008.cn baidu.com test0 test1"
Add-Log (Expand-Tabs (Invoke-LoggedExternal { python (Join-Path $py "dns_query.py") $Bind $Port bupt 008.cn baidu.com test0 test1 2>&1 | Out-String }))
Add-Log ""

Add-Log "Screenshot 11: dig bupt A +noall +answer +comments"
Add-Log (Expand-Tabs (Invoke-LoggedExternal { python (Join-Path $py "dig_win.py") $Bind $Port bupt 2>&1 | Out-String }))
Add-Log ""

Add-Log "Screenshot 12: dig 008.cn A +noall +answer +comments"
Add-Log (Expand-Tabs (Invoke-LoggedExternal { python (Join-Path $py "dig_win.py") $Bind $Port 008.cn 2>&1 | Out-String }))
Add-Log ""

Add-Log "Screenshot 13: dig baidu.com A +noall +answer +comments"
Add-Log (Expand-Tabs (Invoke-LoggedExternal { python (Join-Path $py "dig_win.py") $Bind $Port baidu.com 2>&1 | Out-String }))
Add-Log ""

# fix-B: dead upstream -> SERVFAIL after async timeout (~5s)
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500
$procB = Start-Relay @("-s", "127.0.0.1")
Start-Sleep -Seconds 1

Add-Log "Screenshot 14: fix-B SERVFAIL (upstream 127.0.0.1 unreachable, ~5s)"
Add-Log "`$ python platform\common\python\dns_query.py $Bind $Port not-in-config-xyz123.com"
Add-Log (Expand-Tabs (Invoke-LoggedExternal { python (Join-Path $py "dns_query.py") $Bind $Port not-in-config-xyz123.com 2>&1 | Out-String }))
Add-Log "(Windows: no iptables; uses dead upstream for SERVFAIL demo)"
Add-Log ""

$log.ToString() | Set-Content "$out\03-full-verification.log" -Encoding utf8

Stop-Process -Id $procB.Id -Force -ErrorAction SilentlyContinue
Write-Host "Done. Logs: $out\01-build.log $out\03-full-verification.log"
