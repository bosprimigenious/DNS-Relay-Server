# 分支说明（同步 / 异步中继）

## 分支一览

| 分支 | 角色 | 中继模式 | 报告 | 推荐用途 |
|------|------|----------|------|----------|
| **`main`** | **默认主分支** | — | — | 克隆入口、分支导航（本文件） |
| **`relay-sync`** | 功能线 | 同步 | `docs/report/实验报告-同步.*` | **课设交付、答辩** |
| **`relay-async`** | 功能线 | 异步 | `docs/report/实验报告-异步.*` | 扩展实验、并发压测 |

```
克隆仓库（默认落在 main）
        │
        ├── git checkout relay-sync   → 同步实现 + 同步版报告
        └── git checkout relay-async  → 异步实现 + 异步版报告
```

## 核心差异

```
relay-sync（同步）:
  单 client socket + 临时 upstream socket
  relay_to_upstream() 内 sendto → 阻塞 recvfrom(3s) → 回包

relay-async（异步）:
  持久 client_fd + upstream_fd，select 双路监听
  sendto 上游后立即回到 select
  handle_upstream_response() + find_record_by_new_id() 回包
```

## 共同能力

- 本地拦截、本地解析、上游中继（RFC 1035）
- TTL 缓存、CLI（`-b/-p/-s/-f/-c/-v`）、分级日志
- `select()` 10ms 事件驱动主循环

## 切换与编译

```bash
git fetch origin

git checkout relay-sync    # 课设交付
make clean && make
make report

git checkout relay-async   # 扩展实验
make clean && make
make report
```

## 远程分支

```bash
git branch -a
# origin/main          默认主分支
# origin/relay-sync    同步功能线
# origin/relay-async   异步功能线
```

> 历史分支 `Yhm`、`copilot/*`、`test-yolo` 已删除。
