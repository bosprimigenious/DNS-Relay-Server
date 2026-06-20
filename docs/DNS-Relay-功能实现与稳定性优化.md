# DNS Relay 功能实现与稳定性优化说明

| 成员   | 学号       |
|--------|------------|
| 张恒基 | 2024210926 |
| 尹浩铭 | 2024210910 |
| 林旭东 | 2024210915 |

> **版本**：`relay-async` · `build tag: win-fast3` · Linux/WSL + Windows 原生双平台  
> **依据**：`docs/FIX-TIMEOUT.md`、14 步验收日志 `docs/verification/03-full-verification.log`

---

## 一、稳定性优化说明

本节根据 FIX-TIMEOUT 报告整理，说明验收前后的稳定性保障、运行环境确认，以及上游响应不及时时的处理策略。

### 1. 运行现象与优化目标

服务端可以正常启动，也能加载 `dnsrelay.txt` 中的 **208 条**配置。为了让验收过程更稳定，我们重点关注：

1. 客户端响应是否及时（Windows `nslookup` 默认 2s 超时）  
2. 链路是否可从日志区分（`[LOCAL]` / `[BLOCK]` / `[RELAY]` / `[FAST]` / `[UPSTREAM]` / `[TIMEOUT]`）  
3. 上游慢或不可达时，客户端能否收到**明确的 DNS 语义**（SERVFAIL），而不是长时间无响应  

典型异常：

```text
;; communications error to 127.0.0.1#53: timed out
;; no servers could be reached
DNS request timed out. (timeout was 2 seconds.)
```

排查结论：影响因素包括 **验收工具、残留进程、端口占用、Windows 附带 PTR/AAAA 查询、EDNS 组包错误、上游 RTT**，需分层处理。

### 2. 原因分析

#### 2.1 验收工具确认

**Linux/WSL**：需安装 `bind9-dnsutils`，否则 `dig`/`nslookup` 报 `command not found`，看起来像服务无响应。

```bash
sudo apt update && sudo apt install bind9-dnsutils
nslookup -version && dig -v
```

**Windows**：使用系统自带 `nslookup`；协议对照用 `platform/common/python/dns_query.py`。全量验收：

```powershell
powershell -File platform\windows\verify\run_verification.ps1
```

#### 2.2 dnsrelay 进程状态确认

多个 `dnsrelay` 争用同一 UDP 端口会导致响应不稳定。

```bash
sudo pkill dnsrelay          # Linux
sudo ss -lunp | grep 15353
```

```powershell
Get-Process -Name dnsrelay -ErrorAction SilentlyContinue | Stop-Process -Force
```

#### 2.3 53 端口容易和系统 DNS 混在一起

WSL 的 `systemd-resolved`、Windows 系统 DNS 都可能占用相关端口。自测先用 **15353**，正式验收前确认 53 端口归属：

```bash
sudo ss -lunp | grep ':53'
```

#### 2.4 上游 DNS 与 Windows 附带查询

未命中本地表的 **A 查询** 转发上游（默认 `114.114.114.114:53`）。Windows `nslookup` 还会附带：

| 查询 | 示例 | 旧版问题 | 现版 |
|------|------|----------|------|
| 反向 PTR | `x.in-addr.arpa` | 转上游 >2s | `[FAST]` 空 NOERROR |
| AAAA | `bupt` type=28 | EDNS 坏包被丢弃 | `[LOCAL]` 空 NOERROR |
| A | `bupt`/`baidu.com` | — | 本地或 `[RELAY]` |

#### 2.5 EDNS 错误响应组包（协议层）

Windows 查询常带 EDNS。旧版 `dns_build_error_response` 整包拷贝 query 却设 `arcount=0`，客户端丢弃 → 2s timeout。现版**只拷贝 DNS 头 + 问题段**，与 A 响应策略一致。

### 3. 程序层面的修复

#### 3.1 双 socket + select 非阻塞事件循环

- `client_fd`：收客户端查询  
- `upstream_fd`：收上游响应  
- `select(maxfd+1, …, 10ms)` 同时监听两路  
- `relay_query()`：`sendto` 上游后**立即返回**，不阻塞主循环  

#### 3.2 异步请求记录池（核心：不是多线程）

> **重要**：本程序是 **单进程、单线程** + `select` 事件驱动，**未使用** `pthread`/线程池。  
> 报告中的「高并发」指 **多条上游中继可同时 in-flight**，而非 CPU 多核并行。

**为何需要池**：`sendto` 上游后 ID 已改写为 `new_id`，上游回包时必须找回「客户端地址 + 原始 ID + query 副本」。该状态集合即 **1024 槽请求记录池**（`id_map.c`，`ID_MAP_SIZE=1024`）。

| 阶段 | 函数 | 行为 |
|------|------|------|
| 入池 | `allocate_upstream_id` + `add_record` | 环形写入 `g_records[]`，存 query 512B 副本 |
| 正常出池 | `find_record_by_new_id` → 还原 ID → `release_record` | 上游回包路由 |
| 超时出池 | `process_expired_queries`（每轮 select 前） | ≥5s → SERVFAIL → `release_record` |
| 池满/send 失败 | — | 当场 SERVFAIL |

**单槽字段**：`original_id`、`new_id`、`client_ip`、`client_port`、`qname`、`qtype`、`qclass`、`created_at`、`query[512]`、`in_use`。

#### 3.3 超时兜底返回 SERVFAIL（fix-B）

`ID_MAP_TIMEOUT_SEC = 5`（应用层超时，非 UDP 连接超时）。Windows 验收用 `-s 127.0.0.1` 模拟上游不可达；Linux 可用 `iptables` 阻断 114。

#### 3.4 本地命中 FAST 路径

1. `0.0.0.0` → **NXDOMAIN**（如 `008.cn`）  
2. 非零 IPv4 + QTYPE=A → **本地 A**（如 `bupt` → `123.127.134.10`）  
3. 非零 IPv4 + 非 A → **空 NOERROR**（fix-A，如 MX/AAAA）  
4. `*.in-addr.arpa` / 表外非 A → **`[FAST]`**，不走上游  

表内域名验收**不应受上游网络影响**。

### 4. 验收流程上的修复

#### 4.1 自测：15353 端口

```bash
make clean && make
sudo pkill dnsrelay
DNS_RELAY_BIND=127.0.0.1 DNS_RELAY_PORT=15353 ./dnsrelay -f dnsrelay.txt -v
python3 platform/common/python/dns_query.py 127.0.0.1 15353 bupt
```

Windows：

```powershell
mingw32-make
.\dnsrelay.exe -b 127.0.0.1 -p 15353 -f dnsrelay.txt -v
nslookup bupt 127.0.0.1 -port=15353
```

#### 4.2 正式验收：53 端口

```bash
sudo ./dnsrelay -b 0.0.0.0 -p 53 -f 参考资料/dnsrelay.txt -v
nslookup bupt 127.0.0.1
nslookup 008.cn 127.0.0.1
nslookup baidu.com 127.0.0.1
```

启动日志须含：`relay mode: async`、`build tag: win-fast3`、`loaded 208 entries`。

#### 4.3 上游兜底验证

```bash
sudo iptables -A OUTPUT -d 114.114.114.114 -j DROP
dig @127.0.0.1 -p 15353 not-in-config-xyz123.com A +time=5 +comments
sudo iptables -D OUTPUT -d 114.114.114.114 -j DROP
```

### 5. 修复前后对比

| 对比项 | 修复前 | 修复后 |
|--------|--------|--------|
| 主循环 | 同步阻塞等上游 | 双 socket + select，非阻塞转发 |
| Windows nslookup | PTR/AAAA 易 2s timeout | `[FAST]` + EDNS 修正 |
| 上游慢/不可达 | 长时间无响应 | 5s 后 SERVFAIL，`[TIMEOUT]` 日志 |
| 教室多人查询 | 单线程阻塞排队 | 1024 槽池，多条 in-flight |
| 配置加载 | 相对路径易失败 | 验收脚本用绝对路径，208 条确认 |
| 本地 vs 上游 | 日志混杂 | 分标签验证 |

### 6. 修复边界

- 本地只构造 **A 记录**；不伪造 AAAA  
- TCP 回退、完整 IPv6 中继未展开  
- 错误响应已修 EDNS；后续可继续增强 OPT 处理  

### 7. 小结

稳定性优化分层处理：**FAST 本地路径** + **1024 槽请求池** + **5s SERVFAIL 兜底**。验收时按工具→进程→端口→日志标签逐项排查，流程可复现、可答辩。

---

## 二、功能实现亮点

### 1. 三个基本功能是否都实现了？

**已实现**：本地拦截、本地解析、上游中继。

| 功能 | 配置/条件 | 行为 | 验证 |
|------|-----------|------|------|
| 本地拦截 | IP=0.0.0.0 | NXDOMAIN (RCODE=3) | `008.cn`、`test0` |
| 本地解析 | 非零 IPv4 + QTYPE=A | 构造 A 记录 | `bupt` → `123.127.134.10` |
| 上游中继 | 未命中表 + QTYPE=A | 改 ID→上游→还原 ID；超时 SERVFAIL | `baidu.com` |

**工程增强**：请求记录池、TTL 缓存、双路 `select`、EDNS 修复、双平台 14 步脚本。

### 2. 端口号

- 默认 **UDP 53**；调试推荐 **15353**  
- 环境变量：`DNS_RELAY_BIND`、`DNS_RELAY_PORT`  
- CLI：`-b`、`-p`、`-s`、`-f`、`-c`、`-v`

```bash
./dnsrelay -b 127.0.0.1 -p 15353
sudo ./dnsrelay -b 0.0.0.0 -p 53
```

### 3. TCP 还是 UDP？

**UDP**。课设查询短、实现直接；配合 **Transaction ID + 请求池** 完成请求–响应配对。TCP 未实现。

### 4. 测试的 DNS 地址

| 角色 | 地址 | 用途 |
|------|------|------|
| DNS Relay | `127.0.0.1` 或教室局域网 IP | 客户端查询目标 |
| 上游 DNS | 默认 `114.114.114.114`（`-s` 可改） | A 未命中表时转发 |
| 客户端 | `nslookup 域名 <Relay IP>` | 明确测我们的服务 |

### 5. 北邮相关域名

| 对象 | 本地表？ | 结果 |
|------|----------|------|
| `bupt` | 是 | 本地 A |
| `www.bupt.edu.cn` | 否 | 上游中继 |
| 北邮官方 DNS | 非配置项 | 默认上游 114 |

### 6. Wireshark 验证

- 过滤器：`dns` 或 `udp.port == 53` / `15353`  
- 看 QNAME、QTYPE、QR、RCODE、ANCOUNT  
- 中继场景对比客户端 **original_id** 与上游 **new_id**  

### 7. 代码量与语言

- **C11**；`src/` + `include/` 约 **1680 行**  
- `make` / `mingw32-make` 零告警  

| 模块 | 文件 | 作用 |
|------|------|------|
| 主流程 | `main.c` | 双 socket、select、三路分支、池调度、超时 |
| 协议 | `dns_protocol.c/h` | RFC 1035、A/错误响应、EDNS-safe |
| 配置 | `config.c/h` | 208 条线性查表 |
| **请求池** | `id_map.c/h` | 1024 槽映射、入池/出池/超时 |
| 缓存 | `dns_cache.c/h` | 上游 A 响应 TTL 缓存 |
| 参数/日志 | `options.c`、`logger.c` | CLI、环境变量、分级日志 |
| 跨平台 | `include/platform/net_compat.h` | Winsock / POSIX |

> 答辩时「ID 映射」与「请求记录池」是**同一模块**，合并讲更清晰。

### 8. IPv6 支持边界

非完整 IPv6 DNS。`:: domain` 的 AAAA→NXDOMAIN；表内 IPv4 域名的 AAAA/MX→空 NOERROR。

### 9. 命令行参数

| 方式 | 含义 | 默认 |
|------|------|------|
| `DNS_RELAY_BIND` | 绑定 IP | `0.0.0.0` |
| `DNS_RELAY_PORT` | 端口 | `53` |
| `-s` | 上游 | `114.114.114.114` |
| `-f` | 配置文件 | `参考资料/dnsrelay.txt` 或根目录 `dnsrelay.txt` |
| `-c` | 缓存条目 | `1024` |

### 10. 小组分工

| 成员 | 学号 | 负责 |
|------|------|------|
| 张恒基 | 2024210926 | `main.c`：socket、select、三路分支、池与稳定性 |
| 尹浩铭 | 2024210910 | `dns_protocol.c`：编解码、A/错误响应、EDNS 修复 |
| 林旭东 | 2024210915 | `config.c`、`id_map.c`（**请求池**）、测试脚本、报告 |

联调、14 步验收、FIX-TIMEOUT 文档三人共同完成。

### 11. Bug 与定位

| 场景 | 观察点 | 处理 |
|------|--------|------|
| 非 A 查询 | 不应返回 A 记录 | 空 NOERROR / NXDOMAIN |
| 上游慢 | 客户端挂起 | 请求池 + 5s SERVFAIL |
| EDNS 坏包 | Windows 2s timeout | 只拷贝问题段 |
| PTR 反查 | `in-addr.arpa` 慢 | `[FAST]` 拦截 |
| 配置未加载 | 本地走上游 | `loaded 208 entries` + 绝对路径 |
| 验收脚本中断 | nslookup stderr | PowerShell 不因 stderr 终止 |

定位：**日志标签 + Wireshark + 14 步 log**。

### 12. socket + select 解决并发？

**用了，但不是多线程。**

- 两个 UDP socket + `select(10ms)`  
- 转发前：**入池**（`add_record`）  
- 上游回包：**出池**（`find_record_by_new_id`）  
- 超时：**5s SERVFAIL**  

同步版在 `recvfrom` 上阻塞；异步版 RTT 窗口内仍可收新查询——**在途并行度**由 1024 槽池支撑。

### 13. 数据结构与算法

| 结构 | 用途 |
|------|------|
| `dns_header_t` | RFC 1035 头部 |
| `config_entry_t[]` | 208 条配置，线性查找 |
| **`id_map_record_t[1024]`** | **请求记录池** |
| `dns_cache_entry_t[]` | 上游响应 TTL 缓存 |
| QNAME 编解码 | 标签 + 压缩指针（跳转次数限制） |

### 14. 整体工作流程

1. 解析参数，创建 `client_fd` / `upstream_fd`  
2. 加载配置 → `loaded 208 entries`  
3. 循环：`process_expired_queries` → 缓存淘汰 → `select(10ms)`  
4. 客户端包：反向区/本地表/缓存/非 A `[FAST]`/A 则 `relay_query` **入池**  
5. 上游包：按 `new_id` **出池**、还原 ID、回客户端  

### 15. 代码结构（双平台）

```
src/ include/                    # 跨平台 C 核心
include/platform/net_compat.h   # Linux / Windows 网络适配
platform/linux/verify/            # Bash 14 步验收
platform/windows/verify/          # PowerShell 14 步验收
platform/common/python/           # dns_query.py 等
scripts/                          # 旧路径兼容入口
```

主控层函数为 `handle_client_query` / `handle_upstream_response` / `relay_query`（**不是** `relay_to_upstream` 阻塞模型）。

### 16. UDP 阻塞与超时兜底

UDP 无连接；阻塞风险来自**同步 recvfrom**。本实现用 `select` + **请求池**管理等待：

- 无事件每 10ms 扫描过期池记录  
- 上游及时回包 → 正常出池  
- 超过 5s → SERVFAIL（应用层兜底，非 UDP 连接超时）  

---

## 附录：与您原稿的差异（写 Word 时注意改）

| 原稿表述 | 建议改为 |
|----------|----------|
| 代码约 1538 行 | 约 **1680 行**（含 options/logger/platform 头文件） |
| `scripts/dns_query.py` | `platform/common/python/dns_query.py`（或说明 scripts 为兼容入口） |
| ID 映射表与请求池分两行重复 | 合并为 **「异步请求记录池（id_map，1024 槽）」** |
| 未提 EDNS / FAST / Windows 原生 | **必须补充** §一 2.4–2.5、3.4 |
| 「多线程/高并发」易误解 | 写清：**单线程 + 多条 in-flight**，不是 pthread |
| `relay_to_upstream` | 改为 **relay_query + handle_upstream_response** |
| fix-B 3s / SO_RCVTIMEO | 异步版为 **5s 请求池超时**，无 SO_RCVTIMEO |
| 修复边界未提 EDNS | 已修；可写「只拷贝问题段」 |

---

## Word 文档怎么改（操作步骤）

1. **打开** `DNS-Relay-功能实现与稳定性优化.docx`（或从本 Markdown 全选复制粘贴）。  
2. **第一部分** 在「3. 程序层面」**新增一小节「3.2 异步请求记录池」**（约半页），粘贴上文 **§3.2 表格 + 入池/出池/超时**；原 3.2–3.4 顺延编号。  
3. **必须加粗一句**：「单进程单线程，非多线程；高并发指多条在途中继并行。」  
4. **第二部分第 12、13、16 题** 把「ID 映射」统一改称 **「请求记录池（1024 槽）」**，删除与池重复的单独一行。  
5. **第 7 题** 行数改 **1680**；模块表增加 `net_compat`、`options`、`logger`。  
6. **第 15 题** 目录树改为 `platform/linux`、`platform/windows`；删除 `relay_to_upstream`。  
7. **第 11 题** 增加两行：EDNS 组包、验收脚本 nslookup stderr。  
8. **验收命令** 中 Linux 脚本路径改为 `platform/linux/verify/...`，Windows 改为 `platform\windows\verify\...`。  
9. 通读后 **生成 PDF** 或与 Typst 报告 `实验报告.pdf` 一并提交（课设若只交一份 PDF，以 `实验报告.pdf` 为主，本文档作答辩附录）。

完整可复制正文见本文件；与 `docs/FIX-TIMEOUT.md`、14 步 log 一致。
