# Windows 原生

## 编译

```powershell
mingw32-make clean && mingw32-make
# 或
powershell -File platform\windows\build.ps1
```

产物：`dnsrelay.exe`

## 教室运维

```powershell
# 管理员 PowerShell
powershell -File platform\windows\ops\start_server.ps1
powershell -File platform\windows\ops\rebuild_and_start.ps1
```

## 验收

```powershell
powershell -File platform\windows\verify\run_verification.ps1
powershell -File platform\windows\verify\verify_and_screenshot.ps1
powershell -File platform\windows\verify\classroom_deploy.ps1
```

## 库函数

```powershell
. .\platform\windows\lib\Build-DnsRelay.ps1
Build-DnsRelay
Build-DnsRelay -Clean
```

兼容：根目录 `rebuild_and_start.ps1`、`scripts\*.ps1` 仍可用。

详见 [docs/WINDOWS.md](../../docs/WINDOWS.md)。
