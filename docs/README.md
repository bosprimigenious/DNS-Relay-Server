# 文档目录

| 路径 | 说明 |
|------|------|
| [report/](./report/) | 实验报告源文件（`.typ` / `.md`）与编译 PDF |
| [screenshots/](./screenshots/) | 终端验证截图（`terminal-*.png`） |
| [verification/](./verification/) | 集成测试日志 |
| [dev/](./dev/) | 开发任务清单（`TODO_MCP.md`） |
| [BRANCHES.md](./BRANCHES.md) | 同步 / 异步分支说明 |

## 编译报告 PDF

在项目根目录：

```bash
make report
```

输出：`docs/report/实验报告.pdf`，并复制到根目录 `实验报告.pdf` 供课程提交。
