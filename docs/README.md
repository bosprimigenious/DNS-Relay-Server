# 文档目录

| 路径 | 说明 |
|------|------|
| [report/](./report/) | **同步版**实验报告（`实验报告-同步.*`，分支 `main`） |
| [screenshots/](./screenshots/) | 终端验证截图（`terminal-*.png`） |
| [verification/](./verification/) | 集成测试日志 |
| [dev/](./dev/) | 开发任务清单（`TODO_MCP.md`） |
| [BRANCHES.md](./BRANCHES.md) | 同步 / 异步分支说明 |

## 编译报告 PDF

在项目根目录：

```bash
make report
```

输出：`docs/report/实验报告-同步.pdf`，并复制到根目录 `实验报告.pdf` 供课程提交。

异步版报告在 `relay-async` 分支：`make report` 编译 `实验报告-异步.pdf`。
