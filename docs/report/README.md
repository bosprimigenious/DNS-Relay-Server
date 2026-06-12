# 实验报告（同步 + 异步）

**`relay-sync` 与 `relay-async` 均包含以下两份完整报告**（含 PDF）：

| 文件 | 对应实现 | 说明 |
|------|----------|------|
| `实验报告-同步.typ` / `.md` / `.pdf` | `relay-sync` | 同步 `relay_to_upstream` 阻塞模型 |
| `实验报告-异步.typ` / `.md` / `.pdf` | `relay-async` | 双 socket + ID 表异步回包 |

## 编译 PDF

在项目根目录（需 [Typst](https://typst.app/)）：

```bash
make report-sync    # → docs/report/实验报告-同步.pdf
make report-async   # → docs/report/实验报告-异步.pdf
make report         # 两份都编译；根目录 实验报告.pdf ← 同步版（课设提交）
```

Typst 使用 `--root ../..`，以便引用 `diagrams/` 与 `docs/screenshots/`。

## 课设提交

在 **`relay-sync`** 分支：`make report` 后提交根目录 `实验报告.pdf`（同步版副本）。

异步版 PDF：`docs/report/实验报告-异步.pdf`（答辩对比 / 扩展实验说明）。
