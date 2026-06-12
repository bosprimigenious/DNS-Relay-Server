# DNS-Relay-Server

北京邮电大学（BUPT）计算机网络课程设计 —— **DNS 中继服务器**

本仓库以 **`main` 为默认主分支**（项目入口与分支导航）。

## 分支导航

| 分支 | 中继模式 | 代码 | 报告（两分支均有双份） | 用途 |
|------|----------|------|------------------------|------|
| [**`relay-sync`**](https://github.com/bosprimigenious/DNS-Relay-Server/tree/relay-sync) | 同步 | 本分支 | 同步 + 异步 | **课设交付** |
| [**`relay-async`**](https://github.com/bosprimigenious/DNS-Relay-Server/tree/relay-async) | 异步 | 本分支 | 同步 + 异步 | 扩展实验 |
| **`main`** | — | 同步（与 relay-sync 同代） | 同步 + 异步 | 默认克隆入口 |

> 详见 [docs/BRANCHES.md](docs/BRANCHES.md)。

## 实验报告（同步 + 异步）

**`relay-sync` 与 `relay-async` 的 `docs/report/` 均包含两份报告：**

| 报告 | 文件 |
|------|------|
| 同步版 | `实验报告-同步.{typ,md,pdf}` |
| 异步版 | `实验报告-异步.{typ,md,pdf}` |

```bash
make report-sync    # 仅同步 PDF
make report-async   # 仅异步 PDF
make report         # 两份都编译；根目录 实验报告.pdf ← 同步版（课设）
```

索引：[docs/report/README.md](docs/report/README.md)

## 快速开始

**课设（`relay-sync`）：**

```bash
git clone https://github.com/bosprimigenious/DNS-Relay-Server.git
cd DNS-Relay-Server
git checkout relay-sync
make clean && make && make report
```

**异步扩展（`relay-async`）：**

```bash
git checkout relay-async
make clean && make && make report
```

## 核心差异

```
relay-sync:  relay_to_upstream() 内阻塞 recvfrom → 回包
relay-async: sendto 上游 → select 继续 → handle_upstream_response 回包
```

## 文档

| 文档 | 路径 |
|------|------|
| 分支说明 | [docs/BRANCHES.md](docs/BRANCHES.md) |
| 文档索引 | [docs/README.md](docs/README.md) |
| 同步报告 | [docs/report/实验报告-同步.md](docs/report/实验报告-同步.md) |
| 异步报告 | [docs/report/实验报告-异步.md](docs/report/实验报告-异步.md) |

## 交付清单（课设 · `relay-sync`）

`实验报告.pdf`、`README.md`、`include/`、`src/`、`Makefile`、`参考资料/dnsrelay.txt`、`.gitignore`
