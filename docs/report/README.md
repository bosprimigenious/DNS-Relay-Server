# 实验报告（同步 + 异步）

**两个功能分支均包含以下两份报告**（源码与 PDF 同目录存放，便于对照阅读）：

| 文件 | 对应分支实现 | 说明 |
|------|--------------|------|
| `实验报告-同步.typ` / `.md` / `.pdf` | `relay-sync`（`main` 导航） | 同步 `relay_to_upstream` 阻塞模型 |
| `实验报告-异步.typ` / `.md` / `.pdf` | `relay-async` | 双 socket + ID 表异步回包 |

## 编译

在项目根目录（需安装 [Typst](https://typst.app/)）：

```bash
make report-sync    # 仅同步版 PDF
make report-async   # 仅异步版 PDF
make report         # 两份都编译；根目录 实验报告.pdf ← 同步版（课设提交）
```

## 课设提交

打包 **`relay-sync`** 分支时，提交根目录 `实验报告.pdf`（由 `make report` 从同步版复制）。

异步版 PDF 位于 `docs/report/实验报告-异步.pdf`，用于扩展实验与答辩对比说明。
