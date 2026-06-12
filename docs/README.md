# 文档目录

| 路径 | 说明 |
|------|------|
| [BRANCHES.md](./BRANCHES.md) | **分支导航**（main / relay-sync / relay-async） |
| [report/](./report/) | **同步 + 异步两份实验报告**（各分支均含） |
| [screenshots/](./screenshots/) | 终端验证截图 |
| [verification/](./verification/) | 集成测试日志 |
| [dev/](./dev/) | 开发任务清单 |

## 实验报告

`relay-sync` 与 `relay-async` **均包含**：

| 报告 | 文件 | 编译 |
|------|------|------|
| 同步版 | `report/实验报告-同步.{typ,md,pdf}` | `make report-sync` |
| 异步版 | `report/实验报告-异步.{typ,md,pdf}` | `make report-async` |

```bash
make report   # 两份 PDF + 根目录 实验报告.pdf（同步版，课设用）
```

详见 [report/README.md](./report/README.md)。
