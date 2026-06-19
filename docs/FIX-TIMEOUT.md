# Windows nslookup「DNS request timed out」修复说明

> 分支：`relay-async`  
> 构建标识：`build tag: win-fast3`  
> 相关提交：`1f36b8b` — `fix: Windows 原生构建并消除 nslookup timeout`

---

## 1. 现象

客户端执行：

```powershell
nslookup bupt 10.122.208.238
nslookup 008.cn 10.122.208.238
nslookup baidu.com 10.122.208.238
```

输出类似：

```text
DNS request timed out.
    timeout was 2 seconds.
服务器:  UnKnown
Address:  10.122.208.238

非权威应答:
名称:    bupt
Address:  123.127.134.10
```

**结果正确，但前面总多一行 `timeout`。**

服务端日志同时出现：

```text
[RELAY] qname=238.208.122.10.in-addr.arpa original_id=1 new_id=1 upstream=114.114.114.114
[LOCAL] qname=bupt ip=123.127.134.10
[LOCAL] qname=bupt qtype=28 empty NOERROR
[TIMEOUT] qname=238.208.122.10.in-addr.arpa original_id=1 new_id=1
[BLOCK] qname=008.cn
```

说明：**不是没收到包，也不是没转发出去**；`baidu.com` 有 `[UPSTREAM]`，`bupt` 有 `[LOCAL]`，`008.cn` 有 `[BLOCK]`，功能本身正常。

---

## 2. 根因分析（三层）

### 2.1 Windows `nslookup` 会多发课设不关心的查询

| 查询类型 | 示例 | 旧版行为 | 后果 |
|----------|------|----------|------|
| **反向 DNS（PTR）** | `238.208.122.10.in-addr.arpa`（对服务器 IP `10.122.208.238` 反查） | 转上游 `114.114.114.114` | 上游慢或无应答，超过 2s |
| **AAAA（IPv6）** | `bupt` 的 qtype=28 | sync 版转上游；早期 async 也会转上游 | 等待上游 RTT |
| **A（IPv4）** | `bupt` / `baidu.com` | 本地解析或中继 | 课设真正要测的 |

Windows `nslookup` 默认等待 **2 秒**。PTR 或 AAAA 任一卡住，就会先打印 `timeout`，再显示后续 A 查询的正确结果。

日志里的 `238.208.122.10.in-addr.arpa` 即 `10.122.208.238` 的反向域名（字节倒序 + `.in-addr.arpa`）。

### 2.2 `dns_build_error_response` 组包错误（EDNS）

Windows `nslookup` 常在查询中附带 **EDNS 扩展（OPT 记录）**。

**旧代码逻辑：**

```c
memcpy(response, query, (size_t)query_len);  // 整包拷贝，含 EDNS 附加段
hdr->arcount = 0;                             // 头部却声明附加记录数为 0
```

报文头与正文不一致 → 客户端**丢弃**该响应 → 等满 2 秒 → 再发下一个查询。

受影响场景：

- `008.cn` 的 **NXDOMAIN**
- `bupt` 的 **AAAA** 空 NOERROR
- **MX** 等 fix-A 空应答

`dns_build_a_response` 只拷贝「问题段」，格式正确，所以 A 记录有时仍能答对，但前面的 AAAA/NXDOMAIN 已被丢掉。

### 2.3 同步中继阻塞（教室多人时加重）

**sync** 版在 `relay_to_upstream()` 内 `recvfrom` **阻塞**等待上游，主循环卡住，其他同学查询排队，更容易超过 2 秒。

已恢复 **async**：`sendto` 上游后立即回到 `select()`，不阻塞其他查询。

---

## 3. 修改内容

### 3.1 恢复异步中继架构

| 模块 | 文件 | 说明 |
|------|------|------|
| 双 socket 主循环 | `src/main.c` | 持久 `client_fd` + `upstream_fd`，`select()` 双路监听 |
| 发出中继 | `handle_client_query` | 换 ID、`add_record`、`sendto` 上游后立即返回 |
| 收回中继 | `handle_upstream_response` | `find_record_by_new_id` → 还原 ID → `sendto` 客户端 |
| 超时清理 | `process_expired_queries` | 5s 无应答 → SERVFAIL（fix-B） |
| ID 映射表 | `src/id_map.c`, `include/id_map.h` | 存 query 副本、qname、qtype，支撑异步回包路由 |
| 模式标识 | `include/relay_mode.h` | `relay mode: async`，`build tag: win-fast3` |

### 3.2 修复 EDNS 错误响应组包（核心协议 bug）

**文件：** `src/dns_protocol.c` — `dns_build_error_response()`

**新逻辑：** 与 `dns_build_a_response` 一致，只拷贝「DNS 头 + 问题段」，去掉 EDNS 尾巴：

```c
offset = 12;
dns_name_skip(query, query_len, &offset);
question_len = offset + 4 - 12;
response_len = 12 + question_len;
memcpy(response, query, (size_t)response_len);  // 不再 memcpy 整包
hdr->arcount = 0;  // 与报文正文一致
return response_len;
```

### 3.3 反向 DNS 快速路径

**文件：** `src/main.c`

新增 `is_reverse_dns_qname()`，在查配置表**之前**拦截 `*.in-addr.arpa` / `*.ip6.arpa`：

```c
if (is_reverse_dns_qname(qname)) {
    LOG_INFO("FAST", "qname=%s reverse-zone empty NOERROR", qname);
    send_error_response(..., DNS_RCODE_NOERROR);
    return;
}
```

`238.208.122.10.in-addr.arpa` 不再 `[RELAY]` 到 114，而是 `[FAST]` 毫秒级回包。

### 3.4 非 A 查询不走上游

**文件：** `src/main.c` — 缓存未命中时

```c
if (qtype != DNS_QTYPE_A) {
    LOG_INFO("FAST", "qname=%s qtype=%u empty NOERROR (non-A, skip relay)", qname, qtype);
    send_error_response(..., DNS_RCODE_NOERROR);
    return;
}
// 仅 A 记录才 relay_query()
```

覆盖 AAAA(28)、PTR(12)、MX(15) 等。课设只要求 A / 拦截 / 中继 A。

### 3.5 本地域名非 A 即时应答

**文件：** `src/main.c` — `try_local_response()`

| 配置情况 | 查询类型 | 行为 |
|----------|----------|------|
| `0.0.0.0`（如 `008.cn`） | 任意 | **NXDOMAIN**（即时） |
| 有 IPv4（如 `bupt`） | A | 本地 A 记录 |
| 有 IPv4（如 `bupt`） | 非 A（AAAA/MX） | 空 **NOERROR**（fix-A），不转上游 |

### 3.6 Windows 原生支持（顺带）

| 文件 | 说明 |
|------|------|
| `include/net_compat.h` | Winsock、`strcasecmp` 跨平台适配 |
| `Makefile` | 自动识别 Windows → `dnsrelay.exe` + `-lws2_32` |
| `scripts/build.ps1` | 无 `make` 时直接 `gcc` 编译 |
| `rebuild_and_start.ps1` | 杀旧进程 → 编译 → 启动 |
| `start_server.ps1` | 一键启动服务端 |
| `scripts/run_verification.ps1` | 原生 PowerShell 14 步验收 |
| `docs/WINDOWS.md` | Windows 操作指南 |

---

## 4. 修复前后对比

以 `nslookup bupt 10.122.208.238` 为例。

### 修复前

```text
1. PTR  238.208.122.10.in-addr.arpa  → [RELAY] 上游 → >2s → 客户端 timeout
2. AAAA bupt (qtype=28)              → EDNS 坏包可能被丢 → 再 timeout
3. A    bupt                         → [LOCAL] 正确 IP
```

客户端：**timeout + 正确结果**。

### 修复后（`build tag: win-fast3`）

```text
1. PTR  238.208.122.10.in-addr.arpa  → [FAST] 空 NOERROR（<1ms）
2. AAAA bupt                         → [LOCAL] empty NOERROR（<1ms）
3. A    bupt                         → [LOCAL] 123.127.134.10
```

客户端：**直接显示答案，无 timeout**。

---

## 5. 如何确认跑的是新程序

### 5.1 启动日志

必须看到：

```text
[INFO] relay mode: async (异步上游中继（双 socket + select 非阻塞转发）)
[INFO] build tag: win-fast3
```

### 5.2 查询日志

`nslookup` 测试时，服务端应出现 **`[FAST]`**，**不应再出现**：

```text
[RELAY] qname=238.208.122.10.in-addr.arpa
[TIMEOUT] qname=238.208.122.10.in-addr.arpa
```

若仍是 `[RELAY] in-addr.arpa`，说明还在跑**旧 exe**。

### 5.3 重新编译并启动

```powershell
cd C:\Fullstack_Development\DNS-Relay-Server
git pull origin relay-async

# 管理员 PowerShell
powershell -ExecutionPolicy Bypass -File rebuild_and_start.ps1
```

或手动：

```powershell
cd C:\Fullstack_Development\DNS-Relay-Server
Get-Process -Name dnsrelay -ErrorAction SilentlyContinue | Stop-Process -Force
mingw32-make clean
mingw32-make
.\dnsrelay.exe -b 0.0.0.0 -p 53 -f 参考资料\dnsrelay.txt -v
```

---

## 6. 与 `main` 分支的差异

| 项目 | `main`（sync） | `relay-async`（本修复） |
|------|----------------|-------------------------|
| 中继模式 | 阻塞 `recvfrom` | 异步双 socket + `select` |
| EDNS 错误响应 | 整包拷贝，头体不一致 | 只拷贝问题段 |
| 反向 DNS | 转上游 | `[FAST]` 即时空 NOERROR |
| 非 A 查询（中继域名） | 转上游 | `[FAST]` 即时空 NOERROR |
| Windows 原生 | 依赖 WSL | `dnsrelay.exe` 直接运行 |

`main` 在 WSL/单人测试时 timeout 可能不明显，但协议层 bug 和 `in-addr.arpa` 转上游的问题同样存在。教室 Windows + 多人并发时问题会被放大。

---

## 7. 一句话总结

**timeout 不是 DNS 功能坏了**，而是 Windows `nslookup` 先发 PTR/AAAA，旧代码要么 **EDNS 组包错误被客户端丢弃**，要么 **转上游超过 2 秒**。

修复手段：

1. 改正 `dns_build_error_response` 组包  
2. 反向 DNS / 非 A 查询 **立即空 NOERROR**（`[FAST]` 路径）  
3. 恢复 **async**，只有 **A 记录** 才走上游中继  

---

## 8. 相关文档

- [Windows 原生操作指南](WINDOWS.md)
- [分支说明](BRANCHES.md)
- [项目 README](../README.md)
