# TODO_MCP.md — DNS-Relay-Server 编码任务清单

> **角色**：你是 Cursor，一个严格执行指令的 AI 编码器。逐条完成以下任务，不要自行发挥。
> **验证**：每个 Phase 完成后执行 `make clean && make`，必须零错误零警告。

## 状态文件约定

**路径**：`C:\projects\DNS-Relay-Server\.cursor-status.txt`

**格式**：单行，字段以 ` | ` 分隔。
```
Phase | Status | Time
```
示例：
```
1 | done | 2026-05-18T14:30:00+08:00
```

**规则**：
- **开始**一个 Phase 时，覆盖写入 `Phase | in_progress | <当前时间>`
- **完成**且 `make clean && make` 通过后，覆盖写入 `Phase | done | <当前时间>`
- 若编译失败，覆盖写入 `Phase | failed | <当前时间>` 并**停止**后续所有 Phase
- 每次写状态时覆盖整个文件（只保留当前一行 Phase | Status | Time 记录，不含表头）
- 文件不存在时自动创建
- 时间格式：ISO 8601 带时区偏移，`date +%Y-%m-%dT%H:%M:%S%z`（Linux）或手动写入当前时间

## 审查请求文件约定（闭环触发 Claude Code Review）

**路径**：`C:\projects\DNS-Relay-Server\.cursor-review-request.txt`

**格式**：单行。
```
Phase=<N> | Status=done | Files=<file1,file2,...> | Time=<ISO 8601>
```

**规则**：
- Phase 完成并 commit 后，覆盖写入审查请求文件
- 示例：
  ```
  echo "Phase=1 | Status=done | Files=include/dns_protocol.h,src/dns_protocol.c,src/main.c,Makefile | Time=$(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-review-request.txt
  ```
- Claude 端的 Cron 守护进程每 2 分钟自动轮询此文件
- 发现非空后自动进行 Code Review，审完清空文件
- **你（Cursor）无需手动执行 `claude` 命令**

## Git 提交约定

**格式**：`<type>: <中文描述>`

**type 速查**：
| type | 场景 |
|------|------|
| `feat` | Phase 完成、新增模块 |
| `fix` | Code Review 修复 |
| `docs` | README、报告、注释 |
| `refactor` | 重构（不改行为） |
| `chore` | Makefile、.gitignore、配置 |

**规则**：
- 每个 Phase 的 `make clean && make` 通过后立即 commit
- `git add` 只加源码文件，**不要**加 `build/`、`dnsrelay` 二进制
- 每 3 小时执行一次 `git push origin main`（Phase 完成后检查是否需要 push）
- 若距上次 push ≥ 3 小时且有未推送 commit，一并 push

---

## Phase 1：DNS 协议数据结构与域名编解码

**开始前**，追加状态：
```
echo "1 | in_progress | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
```

### 1.1 修改 `C:\projects\DNS-Relay-Server\include\dns_protocol.h`

在现有 `dns_header_t` 及 `static inline` 函数 **之后**、`#endif` **之前** 追加以下内容：

#### 1.1a DNS 协议常量
```c
#define DNS_PORT            53
#define DNS_MAX_MESSAGE     512
#define DNS_MAX_LABEL_LEN   63
#define DNS_MAX_NAME_LEN    255
#define DNS_QTYPE_A         1
#define DNS_QTYPE_AAAA      28
#define DNS_QTYPE_CNAME     5
#define DNS_QTYPE_MX        15
#define DNS_QTYPE_NS        2
#define DNS_QCLASS_IN       1
#define DNS_RCODE_NOERROR   0
#define DNS_RCODE_FORMAT    1
#define DNS_RCODE_SERVFAIL  2
#define DNS_RCODE_NXDOMAIN  3
#define DNS_RCODE_NOTIMP    4
#define DNS_RCODE_REFUSED   5
#define DNS_TYPE_A          1
#define DNS_TYPE_AAAA       28
#define DNS_TYPE_CNAME      5
#define DNS_TYPE_MX         15
#define DNS_TYPE_NS         2
```

#### 1.1b Question 固定字段结构体
```c
typedef struct {
    uint16_t qtype;
    uint16_t qclass;
} dns_question_fixed_t;
```

#### 1.1c Resource Record 固定字段结构体
```c
typedef struct {
    uint16_t type;
    uint16_t class_;
    uint32_t ttl;
    uint16_t rdlength;
} dns_rr_fixed_t;
```

#### 1.1d 域名编解码函数声明
```c
int dns_name_decode(const unsigned char *packet, int *offset,
                    char *out_buf, int buf_size);

int dns_name_encode(const char *name, unsigned char *out_buf, int buf_size);

int dns_name_skip(const unsigned char *packet, int *offset);
```

---

### 1.2 新建 `C:\projects\DNS-Relay-Server\src\dns_protocol.c`

实现以下三个函数：

#### 1.2a `int dns_name_skip(const unsigned char *packet, int *offset)`
```
逻辑：
  循环：
    读取 packet[*offset] 得到 label_len
    若 label_len == 0：
      *offset += 1
      返回 0
    若 (label_len & 0xC0) == 0xC0：  // 指针压缩
      *offset += 2
      返回 0
    否则：  // 普通 label
      *offset += 1 + label_len
      // 防御：若 *offset > DNS_MAX_MESSAGE 返回 -1
```

#### 1.2b `int dns_name_decode(const unsigned char *packet, int *offset, char *out_buf, int buf_size)`
```
逻辑：
  设 int jumped = 0, ptr_countdown = 10
  设 int pos = *offset
  设 int out_len = 0
  若 pos >= DNS_MAX_MESSAGE，返回 -1
  循环：
    若 (packet[pos] & 0xC0) == 0xC0：
      若 ptr_countdown-- == 0，返回 -1（防循环指针攻击）
      uint16_t ptr = ((packet[pos] & 0x3F) << 8) | packet[pos+1]
      若 !jumped：*offset += 2；jumped = 1
      pos = ptr；continue
    label_len = packet[pos]
    若 label_len == 0：
      若 !jumped：*offset = pos + 1
      若 out_len > 0：out_buf[out_len-1] = '\0'（去掉末尾点）
      否则：out_buf[0] = '\0'
      返回 0
    若 label_len > DNS_MAX_LABEL_LEN，返回 -1
    pos++
    逐字节复制 label_len 个字符到 out_buf
    每个 label 后追加 '.'
    out_len += label_len + 1
    若 out_len >= buf_size，返回 -1
```

#### 1.2c `int dns_name_encode(const char *name, unsigned char *out_buf, int buf_size)`
```
逻辑：
  若 name 为 NULL 或 strlen(name) == 0 或 strlen(name) > DNS_MAX_NAME_LEN：
    返回 -1
  设 int written = 0, label_start = 0, i = 0
  循环 i <= strlen(name)：
    若 name[i] == '.' 或 name[i] == '\0'：
      int label_len = i - label_start
      若 label_len == 0 且 name[i] == '.'：返回 -1（连续点号）
      若 label_len > DNS_MAX_LABEL_LEN：返回 -1
      若 written + 1 + label_len + 1 > buf_size：返回 -1
      out_buf[written++] = (unsigned char)label_len
      memcpy(&out_buf[written], &name[label_start], label_len)
      written += label_len
      label_start = i + 1
    若 name[i] == '\0'：break
    i++
  out_buf[written++] = 0x00  // 终止符
  返回 written
```

文件头部需 `#include "dns_protocol.h"` 和 `#include <string.h>`。

---

### 1.3 修改 `C:\projects\DNS-Relay-Server\src\main.c`

在现有 `#include` 区块末尾追加：
```c
#include "dns_protocol.h"
#include "id_map.h"
```
（这是为后续 Phase 预留的头文件引用，当前不会导致编译错误）

---

### 1.4 修改 `C:\projects\DNS-Relay-Server\Makefile`

将 `CFLAGS` 行改为：
```makefile
CFLAGS := -Wall -Wextra -g -std=c11 -Iinclude
```
（添加 `-std=c11` 确保 C11 标准）

---

### Phase 1 验证
```bash
cd C:\projects\DNS-Relay-Server
make clean && make
```
预期：编译成功，零错误，零警告。生成 `build/main.o`、`build/id_map.o`、`build/dns_protocol.o` 和可执行文件 `dnsrelay`。

编译通过后，写状态文件并提交：
```
echo "1 | done | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
git add include/dns_protocol.h src/dns_protocol.c src/main.c Makefile && git commit -m "feat: 完成 Phase 1 DNS 协议数据结构与域名编解码"
```

---

## Phase 2：配置加载模块（dnsrelay.txt 解析）

**开始前**，追加状态：
```
echo "2 | in_progress | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
```

### 2.1 新建 `C:\projects\DNS-Relay-Server\include\config.h`

```c
#ifndef CONFIG_H
#define CONFIG_H

#include <netinet/in.h>

#define CONFIG_MAX_ENTRIES 4096

typedef struct {
    struct in_addr ip;
    char domain[256];
} config_entry_t;

typedef struct {
    config_entry_t entries[CONFIG_MAX_ENTRIES];
    int count;
} config_t;

int config_load(const char *path, config_t *cfg);

const config_entry_t *config_lookup(const config_t *cfg, const char *domain);

#endif
```

### 2.2 新建 `C:\projects\DNS-Relay-Server\src\config.c`

实现 `config_load`：
```
1. 用 fopen(path, "r") 打开文件
2. 逐行读取（fgets，buffer 512 字节）
3. 跳过空行和以 '#' 开头的注释行
4. 用 sscanf 解析 "IP 域名" 格式：
   char ip_str[64], domain[256]
   若 sscanf(line, "%63s %255s", ip_str, domain) != 2 → 跳过
5. 用 inet_pton(AF_INET, ip_str, &entry.ip) 转换 IP
   失败 → 跳过
6. 将 entry 写入 cfg->entries[cfg->count++]
7. 若 cfg->count >= CONFIG_MAX_ENTRIES → 停止读取
8. fclose，返回 0
```

实现 `config_lookup`：
```
1. 遍历 cfg->entries[0..count-1]
2. strcasecmp(entry.domain, domain) == 0 → 返回 &entry
   （注意：不区分大小写，因为 DNS 域名不区分大小写）
3. 未找到 → 返回 NULL
```

文件头部需 `#include "config.h"` + `#include <stdio.h>` + `#include <string.h>` + `#include <strings.h>`（为 strcasecmp）+ `#include <arpa/inet.h>`。

---

### Phase 2 验证
```bash
make clean && make
```
预期：新增 `build/config.o`，编译零错误零警告。

编译通过后，写状态文件并提交：
```
echo "2 | done | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
git add include/config.h src/config.c && git commit -m "feat: 完成 Phase 2 配置加载模块"
```

---

## Phase 3：DNS 查询解析与本地响应生成

**开始前**，追加状态：
```
echo "3 | in_progress | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
```

### 3.1 修改 `C:\projects\DNS-Relay-Server\src\dns_protocol.c`

追加以下函数（在现有三个函数之后）：

#### 3.1a `int dns_parse_query(const unsigned char *packet, int packet_len, char *qname, int qname_size, uint16_t *qtype, uint16_t *qclass)`
```
逻辑：
  1. 若 packet_len < 12（小于 DNS 头部），返回 -1
  2. 解析 Header（只读，不做校验）：
     const dns_header_t *hdr = (const dns_header_t *)packet
     uint16_t qdcount = ntohs(hdr->qdcount)
  3. 若 qdcount != 1，返回 -1（仅支持单问题查询）
  4. 若 (ntohs(hdr->flags.value) >> 15) & 1（QR=1，是响应而非查询），返回 -1
  5. 设 int offset = 12
  6. 调用 dns_name_decode(packet, &offset, qname, qname_size)，失败返回 -1
  7. 若 offset + 4 > packet_len，返回 -1
  8. 读取 qtype = ntohs(*(uint16_t *)(packet + offset))
  9. 读取 qclass = ntohs(*(uint16_t *)(packet + offset + 2))
  10. 返回 0
```

#### 3.1b `int dns_build_error_response(const unsigned char *query, int query_len, unsigned char *response, int response_size, uint8_t rcode)`
```
功能：基于查询报文构造错误响应（用于拦截和格式错误）

逻辑：
  1. 若 query_len < 12 || response_size < query_len，返回 -1
  2. memcpy(response, query, query_len)  // 原样复制查询报文
  3. dns_header_t *hdr = (dns_header_t *)response
  4. dns_header_network_to_host(hdr)  // 先转为主机序
  5. hdr->flags.bits.qr = 1       // 标记为响应
  6. hdr->flags.bits.ra = 0       // 不支持递归
  7. hdr->flags.bits.rcode = rcode // 设置错误码
  8. hdr->ancount = 0
  9. hdr->nscount = 0
  10. hdr->arcount = 0
  11. dns_header_host_to_network(hdr)  // 转回网络序
  12. 返回 query_len（响应长度与查询相同，无 Answer Section）
```

#### 3.1c `int dns_build_a_response(const unsigned char *query, int query_len, unsigned char *response, int response_size, struct in_addr ip, uint32_t ttl)`
```
功能：构造携带 A 记录的 DNS 成功响应（用于本地解析）

逻辑：
  1. 若 query_len < 12，返回 -1
  2. 从 query 中提取 Question Section：
     a. 用 dns_name_skip 跳过 QNAME
     b. 记录 qname_end_offset
  3. 计算 question_len = qname_end_offset + 4 - 12  // +4 for QTYPE+QCLASS
  4. 计算 response_len = 12 + question_len + 2 + 2 + 4 + 2 + 4
     = 12 + question_len + 16（一个 A 记录的固定 RDATA 开销 + 域名指针）
     = 12 + question_len + DNS 应答部分（NAME指针2 + TYPE2 + CLASS2 + TTL4 + RDLENGTH2 + RDATA4 = 16）
  5. 实际应答 NAME 使用指针压缩，指向查询报文中的 QNAME 位置
     指针偏移 = 12（Header 后面即 QNAME）
     指针值 = 0xC000 | 12 = 0xC00C
     → htons(0xC00C)
  6. 若 response_len > response_size，返回 -1
  7. 构造响应：
     a. memcpy(response, query, 12 + question_len)  // 复制 Header + Question
     b. 修改 Header：qr=1, ra=0, rcode=0, ancount=1, 其他计数=0
     c. 在报文末尾追加 Answer RR：
        - NAME：uint16_t，值=htons(0xC00C)（指针压缩指向 Question 中的 QNAME）
        - TYPE：uint16_t，值=htons(1)（A 记录）
        - CLASS：uint16_t，值=htons(1)（IN）
        - TTL：uint32_t，值=htonl(ttl)
        - RDLENGTH：uint16_t，值=htons(4)（IPv4 地址长度）
        - RDATA：4 字节，ip.s_addr（已是网络字节序，直接 memcpy）
  8. 返回 response_len
```

函数声明需同步添加到 `include/dns_protocol.h` 的声明区（在域名编解码声明下方）。

---

### Phase 3 验证
```bash
make clean && make
```
预期：零错误零警告。

编译通过后，写状态文件并提交：
```
echo "3 | done | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
git add src/dns_protocol.c include/dns_protocol.h && git commit -m "feat: 完成 Phase 3 DNS 查询解析与本地响应生成"
```

---

## Phase 4：DNS 中继引擎（上游转发 + ID 映射 + 超时处理）

**开始前**，追加状态：
```
echo "4 | in_progress | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
```

### 4.1 修改 `C:\projects\DNS-Relay-Server\src\main.c`

#### 4.1a 添加全局变量和常量（在 `main()` 之前）
```c
#define UPSTREAM_DNS_IP      "114.114.114.114"
#define ID_MAP_TIMEOUT_SEC   5
#define SELECT_TIMEOUT_USEC  10000
```

#### 4.1b 添加转发函数声明（在 `main()` 之前，全局变量之后）
```c
static int relay_to_upstream(int sockfd,
                             const unsigned char *query,
                             int query_len,
                             uint16_t original_id,
                             struct sockaddr_in *client_addr);
```

#### 4.1c 实现 `relay_to_upstream()`
```
逻辑：
  1. 创建上游 socket：
     int upstream_fd = socket(AF_INET, SOCK_DGRAM, 0)
     若 < 0，返回 -1
  2. 设置上游 socket 超时（SO_RCVTIMEO = 3 秒）：
     struct timeval tv = {3, 0}
     setsockopt(upstream_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))
  3. 构造上游服务器地址：
     struct sockaddr_in upstream_addr
     memset 清零
     upstream_addr.sin_family = AF_INET
     upstream_addr.sin_port = htons(53)
     inet_pton(AF_INET, "114.114.114.114", &upstream_addr.sin_addr)
  4. 生成新的 Transaction ID：
     static uint16_t g_next_id = 1
     uint16_t new_id = g_next_id++
     若 g_next_id == 0：g_next_id = 1
  5. 将 original_id ↔ new_id 映射写入 id_map：
     调用 add_record(original_id, new_id, client_addr->sin_addr, client_addr->sin_port, time(NULL))
     若返回 -1（表满），close(upstream_fd)，返回 -1
  6. 修改查询报文中的 ID 为 new_id：
     注意：需要复制一份 query 到临时 buffer，修改 buffer 中的 id 字段
     用 memcpy 复制，然后把前 2 字节改为 htons(new_id)
  7. 发送：sendto(upstream_fd, modified_query, query_len, 0, &upstream_addr, sizeof(upstream_addr))
     若 < 0，清理 id_map 记录（或让超时清理），close(upstream_fd)，返回 -1
  8. 接收响应：
     unsigned char response[DNS_MAX_MESSAGE]
     socklen_t upstream_len = sizeof(upstream_addr)
     ssize_t resp_len = recvfrom(upstream_fd, response, sizeof(response), 0,
                                  (struct sockaddr *)&upstream_addr, &upstream_len)
     若 < 0：检查 errno，清理记录，close(upstream_fd)，返回 -1
  9. 还原响应中的 ID：
     将 response 前 2 字节改为 htons(original_id)
  10. 转发给客户端：
      sendto(sockfd, response, resp_len, 0,
             (const struct sockaddr *)client_addr, sizeof(*client_addr))
  11. close(upstream_fd)
  12. 返回 0
```

---

### 4.2 修改 `C:\projects\DNS-Relay-Server\src\main.c` 的 `main()` 函数

当前 main() 只有骨架。在 `recvfrom` 成功接收数据后，插入以下处理逻辑：

```c
// === 在 recvfrom 之后，printf 之前插入 ===

// 1. 检查报文最小长度
if (received < 12) {
    continue;  // 报文太短，静默丢弃
}

// 2. 解析查询域名
char qname[DNS_MAX_NAME_LEN + 1];
uint16_t qtype, qclass;
int query_offset = 0;
if (dns_parse_query(buffer, (int)received, qname, sizeof(qname), &qtype, &qclass) != 0) {
    // 格式错误，返回 FORMERR
    unsigned char err_resp[DNS_MAX_MESSAGE];
    int err_len = dns_build_error_response(buffer, (int)received, err_resp, sizeof(err_resp), DNS_RCODE_FORMAT);
    if (err_len > 0) {
        sendto(sockfd, err_resp, err_len, 0,
               (const struct sockaddr *)&client_addr, client_len);
    }
    continue;
}

// 3. 查配置表
const config_entry_t *entry = config_lookup(&g_config, qname);
if (entry != NULL) {
    // 命中配置
    if (entry->ip.s_addr == 0) {
        // 0.0.0.0 → 拦截，返回 NXDOMAIN
        unsigned char resp[DNS_MAX_MESSAGE];
        int rlen = dns_build_error_response(buffer, (int)received, resp, sizeof(resp), DNS_RCODE_NXDOMAIN);
        if (rlen > 0) {
            sendto(sockfd, resp, rlen, 0,
                   (const struct sockaddr *)&client_addr, client_len);
        }
    } else {
        // 本地解析，返回 A 记录
        unsigned char resp[DNS_MAX_MESSAGE];
        int rlen = dns_build_a_response(buffer, (int)received, resp, sizeof(resp), entry->ip, 300);
        if (rlen > 0) {
            sendto(sockfd, resp, rlen, 0,
                   (const struct sockaddr *)&client_addr, client_len);
        }
    }
} else {
    // 4. 未命中 → 中继到上游 DNS
    const dns_header_t *hdr = (const dns_header_t *)buffer;
    uint16_t original_id = ntohs(hdr->id);
    relay_to_upstream(sockfd, buffer, (int)received, original_id, &client_addr);
}
```

此外，`main()` 开头需要初始化配置：
```c
config_t g_config;  // 全局或 main() 局部（传递给处理逻辑）
```
并在 bind 成功后调用：
```c
if (config_load("参考资料/dnsrelay.txt", &g_config) != 0) {
    fprintf(stderr, "warning: failed to load config, relay-only mode\n");
}
```

同时，在主循环中（`select` 返回后、`recvfrom` 之前）需要定期清理超时映射：
```c
// 每收到一个包时顺便清理一次超时记录
clear_timeout_records(time(NULL), ID_MAP_TIMEOUT_SEC);
```

**注意**：需在 main.c 顶部 `#include` 区块追加：
```c
#include "config.h"
```

---

### Phase 4 验证
```bash
make clean && make
```
预期：零错误零警告。程序可运行并在 53 端口监听。

编译通过后，写状态文件并提交：
```
echo "4 | done | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
git add src/main.c && git commit -m "feat: 完成 Phase 4 DNS 中继引擎与主循环集成"
```

---

## Phase 5：最终组装与健壮性加固

**开始前**，追加状态：
```
echo "5 | in_progress | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
```

### 5.1 修改 `C:\projects\DNS-Relay-Server\src\main.c`

#### 5.1a 确保 main() 函数结构完整
main() 最终结构应为：
```
1. socket() 创建 UDP socket
2. bind() 绑定 0.0.0.0:53
3. config_load() 加载 dnsrelay.txt
4. printf("DNS relay server listening on 0.0.0.0:53 ...\n")
5. for (;;)：
   a. FD_ZERO + FD_SET + select(超时 10ms)
   b. select 返回 < 0 且 errno == EINTR → continue
   c. select 返回 < 0 → perror + break
   d. select 返回 0（超时）→ continue
   e. FD_ISSET(sockfd) → recvfrom
   f. 若 received < 12 → continue（静默丢弃）
   g. clear_timeout_records(time(NULL), ID_MAP_TIMEOUT_SEC)
   h. dns_parse_query → 失败则返回 FORMERR
   i. config_lookup → 命中则拦截/本地解析
   j. 未命中 → relay_to_upstream
6. close(sockfd); return 0
```

#### 5.1b 检查 relay_to_upstream 的 ID 映射表空间回收
确认 `add_record` 失败时不会内存泄漏——当前 id_map.c 实现为线性探测，表满返回 -1。在 relay_to_upstream 中，若 add_record 返回 -1，应先调用 `clear_timeout_records(time(NULL), 0)` 强制清理全部过期记录，再重试一次。

#### 5.1c 防御性编程检查清单
- [ ] 所有 `recvfrom` / `sendto` 返回值检查
- [ ] 所有 `malloc` 不可用（全用栈分配，当前设计已满足）
- [ ] DNS 报文缓冲区索引不越界
- [ ] 域名解码 loop 有最大跳转次数限制（防循环指针）
- [ ] `relay_to_upstream` 中 upstream_fd 在所有路径上关闭（包括错误路径）
- [ ] `strcasecmp` 确保域名比较不区分大小写

### 5.2 修改 `C:\projects\DNS-Relay-Server\.gitignore`

确认已忽略：
```
/build/
/dnsrelay
```

（当前 .gitignore 已正确，无需修改）

---

### Phase 5 验证
```bash
make clean && make
```
零错误零警告。然后用管理员权限运行 `./dnsrelay` 测试。

编译通过后，写状态文件并提交：
```
echo "5 | done | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
git add src/main.c .gitignore && git commit -m "feat: 完成 Phase 5 最终组装与健壮性加固"
```

---

## Phase 6：Code Review 修复（fix-A / fix-B）

> **说明**：此 Phase 的修复指令由 Claude（架构师）在 Code Review 后动态下发。Cursor 收到指令后按 fix-A / fix-B 标识执行。

### fix-A 验证
```bash
make clean && make
```
写状态文件并提交：
```
echo "fix-A | done | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
git add src/main.c && git commit -m "fix: 本地解析增加 QTYPE 检查，非 A 查询返回空 NOERROR"
```

### fix-B 验证
```bash
make clean && make
```
写状态文件并提交：
```
echo "fix-B | done | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
git add src/main.c && git commit -m "fix: 上游中继失败时返回 SERVFAIL 给客户端"
```

### Push 检查（每个 Phase 完成后执行）
```bash
LAST_PUSH=$(git log --format="%at" origin/main -1 2>/dev/null || echo 0)
NOW=$(date +%s)
ELAPSED=$(( (NOW - LAST_PUSH) / 3600 ))
if [ $ELAPSED -ge 3 ] || [ "$LAST_PUSH" = "0" ]; then
    git push origin main
    echo "已推送到远程（距上次 push ${ELAPSED} 小时）"
else
    echo "距上次推送仅 ${ELAPSED} 小时，暂不 push"
fi
```

---

## 附录 A：文件结构总览

```
C:\projects\DNS-Relay-Server\
├── include/
│   ├── dns_protocol.h    # DNS 数据结构 + 常量 + 编解码声明
│   ├── id_map.h           # ID 映射表接口
│   └── config.h           # 配置加载接口
├── src/
│   ├── main.c             # 主循环 + select 事件分发 + relay_to_upstream
│   ├── dns_protocol.c     # 域名编解码 + 查询解析 + 响应构造
│   ├── id_map.c           # ID 映射表实现
│   └── config.c           # dnsrelay.txt 解析实现
├── 参考资料/
│   ├── dnsrelay.txt       # 域名-IP 映射配置文件
│   ├── RFC1035.TXT        # DNS 协议规范
│   ├── RFC1035.pdf        # DNS 协议规范 (PDF)
│   └── 计算机网络课程设计-DNS(6).pptx  # 课程设计需求
├── Makefile
├── .gitignore
├── .cursorrules
└── TODO_MCP.md
```

## 附录 B：关键协议规范速查

### DNS Header（12 字节，大端序）
```
 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                    ID                           |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|QR| OPCODE  |AA|TC|RD|RA| Z  |     RCODE        |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                  QDCOUNT                         |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                  ANCOUNT                         |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                  NSCOUNT                         |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                  ARCOUNT                         |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
```

### RCODE 取值
- 0 = NOERROR
- 1 = FORMERR（格式错误）
- 2 = SERVFAIL（服务器失败）
- **3 = NXDOMAIN（域名不存在，用于拦截）**
- 4 = NOTIMP（未实现）
- 5 = REFUSED（拒绝）

### DNS 域名指针压缩
- 若某字节高 2 位为 `11`（即 `& 0xC0 == 0xC0`），则该字节与下一字节组成 16 位指针
- 指针值 = `((byte0 & 0x3F) << 8) | byte1`，表示在报文中的绝对偏移量

---

## Phase 7：最终交付准备（文档 + 测试 + 打包）

> **状态文件**：`C:\projects\DNS-Relay-Server\.cursor-status.txt`
> **审查请求文件**：`C:\projects\DNS-Relay-Server\.cursor-review-request.txt`

### 7.1 实验报告完善

#### 7.1a 填写个人信息
修改 `实验报告.md` 顶部表格：
```
| **姓名** | _（请填写）_  →  你的真实姓名
| **学号** | _（请填写）_  →  你的学号
| **班级** | _（请填写）_  →  你的班级
```

#### 7.1b 补测试截图
用 `nslookup` 或 `dig` 实际运行程序，对以下用例截屏并粘贴到报告中对应位置：
- 用例 1：`nslookup bupt 127.0.0.1` → 本地解析
- 用例 3：`nslookup 008.cn 127.0.0.1` → 拦截 NXDOMAIN
- 用例 4：`nslookup baidu.com 127.0.0.1` → 上游中继

截图位置在 `实验报告.md` 的 4.3 节（三个 `（截图占位）` 处）。

#### 7.1c 导出 PDF
将 Markdown 报告导出为 PDF。推荐方案：
```bash
# 方案 A：Pandoc（需要 texlive 或 wkhtmltopdf）
pandoc 实验报告.md -o 实验报告.pdf --pdf-engine=xelatex -V CJKmainfont="SimSun"

# 方案 B：Typora / VS Code Markdown PDF 插件直接导出
# 方案 C：转 Typst 后编译（更美观，但需要重新排版）
```
输出文件命名为 `实验报告.pdf`，放在项目根目录。

### 7.2 确认编译 & 运行

在 WSL2 中执行：
```bash
make clean && make
sudo ./dnsrelay
```
确保程序正常监听 `0.0.0.0:53`。

### 7.3 交付文件清单

最终提交的文件：
```
实验报告.pdf           # 课程设计报告
README.md              # 使用说明
include/               # 头文件
src/                   # 源代码
Makefile               # 编译脚本
参考资料/dnsrelay.txt  # 配置文件
.gitignore
```

### 7.4 清理仓库

确认以下不进入提交：
- `dnsrelay.exe`（根目录的 0 字节占位文件 → 删除）
- `build/` 目录（已在 .gitignore）
- `dnsrelay` 可执行文件（已在 .gitignore）
- `.cursor-review-request.txt` / `.cursor-status.txt`（本地追踪文件）

### Phase 7 验证

- [ ] 实验报告个人信息已填写
- [ ] 测试截图已粘贴
- [ ] PDF 已导出
- [ ] `make clean && make` 通过
- [ ] 仓库干净（无二进制文件残留）

编译通过后，写状态文件并提交：
```
echo "7 | done | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
git add 实验报告.md 实验报告.pdf README.md && git commit -m "docs: 完善实验报告，补测试截图，导出 PDF"
```

---

## Phase 8：报告视觉增强（SVG 矢量图 + Typst 编译）

> **说明**：当前 `实验报告.typ` 使用 ASCII art 字符画表示系统架构图和流程图，视觉效果不符合课程设计交付标准。本 Phase 将所有框图替换为专业 SVG 矢量图。

**开始前**，追加状态：
```
echo "8 | in_progress | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
```

### 8.1 创建 `diagrams/` 目录

```bash
mkdir -p C:/projects/DNS-Relay-Server/diagrams
```

### 8.2 绘制系统架构 SVG

创建 `C:\projects\DNS-Relay-Server\diagrams\architecture.svg`：

**设计要求**：
- 从左到右：客户端 → DNS-Relay-Server（中间大框）→ 上游 DNS 114.114.114.114
- DNS-Relay-Server 内部展示三个分支：拦截(NXDOMAIN)、本地解析(A记录)、上游中继
- 使用圆角矩形、箭头连接、中文标注
- 配色：主色调 #2563EB（蓝），背景 #F8FAFC，边框 #94A3B8

**SVG 尺寸**：800×400

### 8.3 绘制主循环流程 SVG

创建 `C:\projects\DNS-Relay-Server\diagrams\flowchart.svg`：

**设计要求**：
- 从上到下展示 `select()` → `recvfrom` → `dns_parse_query` → `config_lookup` → 三条分支
- 三条分支分别对应：NXDOMAIN、A记录/空NOERROR、relay_to_upstream（成功/SERVFAIL）
- 使用菱形表示判断节点，矩形表示处理节点
- 配色统一：判断节点 #F59E0B（黄），处理节点 #3B82F6（蓝），错误节点 #EF4444（红）

**SVG 尺寸**：700×700

### 8.4 绘制 DNS 报文结构 SVG

创建 `C:\projects\DNS-Relay-Server\diagrams\dnspacket.svg`：

**设计要求**：
- 展示 DNS Header（12字节）各字段布局：ID、FLAGS（QR/OPCODE/AA/TC/RD/RA/Z/RCODE）、QDCOUNT、ANCOUNT、NSCOUNT、ARCOUNT
- 展示 Question Section 和 Answer RR Section 结构
- 标注各字段位宽（bit 数）
- 颜色区分 Header / Question / Answer 三部分

**SVG 尺寸**：800×500

### 8.5 修改 `实验报告.typ` 替换 ASCII art

#### 8.5a 系统架构图（第 2.1 节）

找到：
```
#figure(
  text(size: 9pt, font: "Consolas")[
    ```
                        ┌─────────────────────────────────────┐
                        │         DNS-Relay-Server            │
      客户端 ──UDP:53──►│  main.c (select 主循环)              │
    ...
```
替换为：
```typst
#figure(
  image("diagrams/architecture.svg", width: 100%),
  caption: [DNS 中继服务器系统架构],
)
```

#### 8.5b 主循环流程图（第 2.3 节）

找到 ASCII art 流程图，替换为：
```typst
#figure(
  image("diagrams/flowchart.svg", width: 100%),
  caption: [主循环处理流程],
)
```

#### 8.5c 新增 DNS 报文结构图（第 3.1 节末尾）

在 "== DNS 报文与字节序" 节末尾追加：
```typst
#figure(
  image("diagrams/dnspacket.svg", width: 100%),
  caption: [DNS 报文结构（Header + Question + Resource Record）],
)
```

### 8.6 编译 Typst → PDF

```bash
cd C:/projects/DNS-Relay-Server
typst compile 实验报告.typ 实验报告.pdf
```

**要求**：零错误，PDF 中 SVG 图片清晰可读。

### 8.7 验证检查清单

- [ ] 三个 SVG 文件创建成功，内容完整
- [ ] Typst 中 ASCII art 全部替换为 `#image()` 引用
- [ ] `typst compile` 通过，无错误
- [ ] PDF 中所有 SVG 正常渲染、表格对齐、页码连续
- [ ] 目录超链接可点击跳转

### Phase 8 验证

```bash
cd C:/projects/DNS-Relay-Server
typst compile 实验报告.typ 实验报告.pdf
```

编译通过后，写状态文件和审查请求文件并提交：
```
echo "8 | done | $(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-status.txt
echo "Phase=8 | Status=done | Files=diagrams/architecture.svg,diagrams/flowchart.svg,diagrams/dnspacket.svg,实验报告.typ,实验报告.pdf | Time=$(date +%Y-%m-%dT%H:%M:%S%z)" > .cursor-review-request.txt
git add diagrams/ 实验报告.typ 实验报告.pdf && git commit -m "docs: 替换 ASCII 框图为 SVG 矢量图，Typst 重编译 PDF"
```

### Push 检查（Phase 8 完成后执行）
```bash
LAST_PUSH=$(git log --format="%at" origin/main -1 2>/dev/null || echo 0)
NOW=$(date +%s)
ELAPSED=$(( (NOW - LAST_PUSH) / 3600 ))
if [ $ELAPSED -ge 3 ] || [ "$LAST_PUSH" = "0" ]; then
    git push origin main
    echo "已推送到远程（距上次 push ${ELAPSED} 小时）"
else
    echo "距上次推送仅 ${ELAPSED} 小时，暂不 push"
fi
```
