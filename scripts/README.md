# scripts/ — 兼容入口

**正式脚本已迁至 [`platform/`](../platform/README.md)**。本目录仅保留薄包装，便于旧文档与实验报告路径继续可用。

| 旧路径 | 实际实现 |
|--------|----------|
| `scripts/*.sh` | `platform/linux/verify/` |
| `scripts/*.ps1` | `platform/windows/` 或 `platform/windows/verify/` |
| `scripts/*.py` | `platform/common/python/` |

新开发请直接使用 `platform/` 下对应文件。
