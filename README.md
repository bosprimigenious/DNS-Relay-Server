# DNS-Relay-Server

北京邮电大学（BUPT）计算机网络课程设计 —— **DNS 中继服务器**

**Windows 原生开发与验收**（`dnsrelay.exe` + PowerShell `nslookup`），无需 WSL。详见 [docs/WINDOWS.md](docs/WINDOWS.md)。

## 分支导航

| 分支 | 中继模式 | 用途 |
|------|----------|------|
| **`relay-sync`** | 同步 | 课设交付 |
| **`relay-async`** | 异步 | **推荐**：教室多人并发、无 nslookup timeout |
| **`main`** | — | 默认克隆入口 |

> 详见 [docs/BRANCHES.md](docs/BRANCHES.md)。

## 快速开始（Windows）

```powershell
git clone https://github.com/bosprimigenious/DNS-Relay-Server.git
cd DNS-Relay-Server
git checkout relay-async

mingw32-make clean && mingw32-make

.\dnsrelay.exe -b 127.0.0.1 -p 15353 -f 参考资料\dnsrelay.txt -v
```

另开终端：`nslookup bupt 127.0.0.1 -port=15353`

## 教室验收

```powershell
.\dnsrelay.exe -b 0.0.0.0 -p 53 -f 参考资料\dnsrelay.txt -v
powershell -File scripts\classroom_deploy.ps1
.\scripts\verify_and_screenshot.ps1
```

## 文档

| 文档 | 路径 |
|------|------|
| Windows 指南 | [docs/WINDOWS.md](docs/WINDOWS.md) |
| 分支说明 | [docs/BRANCHES.md](docs/BRANCHES.md) |
