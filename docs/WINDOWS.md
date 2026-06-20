# DNS-Relay-Server — Windows 原生指南

本仓库**默认在 Windows 上开发与验收**，无需 WSL / 虚拟机。

## 环境要求

| 工具 | 用途 | 安装 |
|------|------|------|
| **MSYS2 MinGW-w64** | `gcc` + `mingw32-make` | [msys2.org](https://www.msys2.org/) → `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make` |
| **Python 3** | 协议测试、截图脚本 | python.org |
| **PowerShell** | 验证与教室部署 | Windows 自带 |

将 `C:\msys64\mingw64\bin` 加入系统 PATH。

## 编译

```powershell
cd C:\Fullstack_Development\DNS-Relay-Server

# 方式 A（推荐）
mingw32-make clean
mingw32-make

# 方式 B（无 make）
powershell -ExecutionPolicy Bypass -File platform\windows\build.ps1
```

产物：`dnsrelay.exe`（实现位于 [`platform/windows/`](../platform/windows/README.md)）

## 本地测试（端口 15353，无需管理员）

```powershell
.\dnsrelay.exe -b 127.0.0.1 -p 15353 -f 参考资料\dnsrelay.txt -v
```

另开 PowerShell：

```powershell
nslookup bupt 127.0.0.1 -port=15353
nslookup 008.cn 127.0.0.1 -port=15353
nslookup baidu.com 127.0.0.1 -port=15353
```

## 教室验收（端口 53，需管理员 PowerShell）

```powershell
.\dnsrelay.exe -b 0.0.0.0 -p 53 -f 参考资料\dnsrelay.txt -v
```

日志须含：`relay mode: async`

同学客户端（把 IP 换成你笔记本在教室网的地址）：

```powershell
nslookup bupt 10.x.x.x
nslookup 008.cn 10.x.x.x
nslookup baidu.com 10.x.x.x
```

一键冒烟：`powershell -File platform\windows\verify\classroom_deploy.ps1`

## 全量验证 + 报告截图

```powershell
powershell -File platform\windows\verify\verify_and_screenshot.ps1
```

（`scripts\verify_and_screenshot.ps1` 为兼容包装，效果相同。）

生成 `docs\screenshots\terminal-01-build.png` … `terminal-14-fix-b.png`。

## fix-B 说明（Windows）

Linux 版用 `iptables` 阻断上游；Windows 版验证脚本改为**临时将上游设为 `127.0.0.1`**（无 DNS 服务），约 5 秒后返回 SERVFAIL，语义与课设一致。

## 可选：Linux / WSL

`Makefile` 在 Linux 下仍可用，产物为 `dnsrelay`（无 `.exe`）。脚本见 [`platform/linux/`](../platform/linux/README.md)。
