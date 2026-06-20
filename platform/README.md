# 双平台目录说明

本仓库在 **Linux/WSL** 与 **Windows 原生** 下均可编译、验收。源码（`src/`、`include/`）共用；平台差异集中在 `platform/`。

```
platform/
├── linux/          # WSL / Linux：Bash 验收与压测
│   └── verify/
├── windows/        # Windows 原生：PowerShell 构建与验收
│   ├── build.ps1
│   ├── lib/        # 可复用函数（Build-DnsRelay 等）
│   ├── ops/        # 教室运维：编译+启动
│   └── verify/
└── common/         # 跨平台共用
    ├── lib/        # Shell 小工具
    └── python/     # dns_query.py、截图生成等
```

| 场景 | 推荐入口 |
|------|----------|
| Linux/WSL 编译 | 项目根 `make` |
| Linux/WSL 14 步验收 | `bash platform/linux/verify/run_verification.sh` |
| Windows 编译 | `mingw32-make` 或 `platform/windows/build.ps1` |
| Windows 14 步验收 | `platform/windows/verify/run_verification.ps1` |
| 教室一键启动 | `platform/windows/ops/start_server.ps1`（管理员） |

**兼容**：根目录与 `scripts/` 仍保留薄包装，旧文档中的 `scripts\*.ps1` 路径可继续使用。

头文件平台适配见 [`include/platform/README.md`](../include/platform/README.md)。
