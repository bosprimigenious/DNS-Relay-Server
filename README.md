# DNS-Relay-Server

北京邮电大学（BUPT）计算机网络课程设计 —— **DNS 中继服务器**

**Linux/WSL 与 Windows 原生双平台**开发与验收；源码共用，平台脚本分目录存放。详见 [platform/README.md](platform/README.md)。

## 分支导航

| 分支 | 中继模式 | 用途 |
|------|----------|------|
| **`relay-sync`** | 同步 | 课设交付 |
| **`relay-async`** | 异步 | **推荐**：教室多人并发、无 nslookup timeout |
| **`main`** | — | 默认克隆入口 |

> 详见 [docs/BRANCHES.md](docs/BRANCHES.md)。

## 项目结构（摘要）

```
src/ include/          # 跨平台 C11 核心（net_compat 适配 Linux / Windows）
platform/
  linux/verify/        # Bash 验收
  windows/             # PowerShell 构建与验收
  common/python/       # 共用探针与截图脚本
Makefile               # Linux 与 Windows 自动识别
scripts/               # 旧路径兼容包装 → platform/
```

## 快速开始（Windows）

```powershell
git clone https://github.com/bosprimigenious/DNS-Relay-Server.git
cd DNS-Relay-Server
git checkout relay-async

mingw32-make clean && mingw32-make

.\dnsrelay.exe -b 127.0.0.1 -p 15353 -f 参考资料\dnsrelay.txt -v
```

另开终端：`nslookup bupt 127.0.0.1 -port=15353`

## 快速开始（Linux / WSL）

```bash
make clean && make
DNS_RELAY_BIND=127.0.0.1 DNS_RELAY_PORT=15353 ./dnsrelay -f 参考资料/dnsrelay.txt -v
```

## 教室验收（Windows）

```powershell
powershell -File platform\windows\ops\start_server.ps1
powershell -File platform\windows\verify\classroom_deploy.ps1
powershell -File platform\windows\verify\verify_and_screenshot.ps1
```

## 文档

| 文档 | 路径 |
|------|------|
| 双平台目录 | [platform/README.md](platform/README.md) |
| Windows 指南 | [docs/WINDOWS.md](docs/WINDOWS.md) |
| Linux 指南 | [platform/linux/README.md](platform/linux/README.md) |
| 分支说明 | [docs/BRANCHES.md](docs/BRANCHES.md) |
