# 分支说明（同步 / 异步中继）

本仓库用**功能型分支名**区分上游中继实现，不使用个人姓名或缩写作为分支名。

| 分支 | 中继模式 | 说明 | 推荐用途 |
|------|----------|------|----------|
| **`main`** | 同步 | 与 `relay-sync` 一致，**默认交付与答辩** | 课程提交、实验报告、现场演示 |
| **`relay-sync`** | 同步 | 单 socket + `select()`；上游用临时 socket **阻塞**等待响应（3s 超时） | 与 main 同步维护 |
| **`relay-async`** | 异步 | 双 socket + `select()` 同时监听客户端与上游；非阻塞中继 | 扩展实验、并发压测对比 |

## 同步 vs 异步（核心差异）

```
relay-sync / main:
  客户端查询 → relay_to_upstream() 内 sendto → recvfrom 阻塞 → 回包

relay-async:
  客户端查询 → sendto 上游后立即返回主循环
  上游响应到达 → handle_upstream_response() 查 ID 映射表 → 回包
```

## 共同能力（relay-sync / main 当前）

- 本地拦截、本地解析、上游中继（RFC 1035）
- TTL 缓存、CLI（`-b/-p/-s/-f/-c/-v`）、分级日志
- `select()` 10ms 事件驱动主循环

## 切换分支

```bash
git checkout main          # 或 relay-sync（同步，推荐交付）
git checkout relay-async   # 异步扩展版
make clean && make
```

## 远程分支

```bash
git fetch origin
git checkout relay-sync    # origin/relay-sync
git checkout relay-async   # origin/relay-async
```

> 历史分支 `Yhm` 已弃用，请改用 **`relay-async`**。
