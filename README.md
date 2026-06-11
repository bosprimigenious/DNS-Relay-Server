# DNS-Relay-Server

北京邮电大学（BUPT）计算机网络课程设计 —— **DNS 中继服务器**

> **分支**：`main` 为**同步上游中继**（课设默认交付）；**异步**实现见 `relay-async`。详见 [docs/BRANCHES.md](docs/BRANCHES.md)。

基于 RFC 1035 实现 UDP DNS 中继：支持本地拦截、本地解析、上游转发与 **TTL 缓存**，主循环使用 `select()` 事件驱动（10ms 超时），上游中继为**同步**模型（临时 socket + 3s 超时）。

## 功能

| 模式 | 条件 | 行为 |
|------|------|------|
| 本地拦截 | `dnsrelay.txt` 中 IP 为 `0.0.0.0` | 返回 NXDOMAIN（RCODE=3） |
| 本地解析 | 配置表中为真实 IPv4 | 返回 A 记录（仅响应 QTYPE=A） |
| 缓存命中 | 上游响应未过期 | 直接返回缓存（按剩余 TTL） |
| 上游中继 | 域名不在表中且未命中缓存 | 同步转发至上游 DNS，还原 Transaction ID |

其他能力：`-b/-p/-s/-f/-c/-v` 命令行参数（兼容 `DNS_RELAY_BIND/PORT`）、分级日志、完整 `packet_len` 边界检查、非 IN 类查询返回 NOTIMP。

## 编译

需要 Linux / WSL（`arpa/inet.h`、`sys/select.h` 等 POSIX 接口）。

```bash
make clean && make
```

## 运行

绑定 53 端口通常需要 root 权限：

```bash
sudo ./dnsrelay
# 或显式参数（开发测试推荐 5353）
./dnsrelay -b 127.0.0.1 -p 5353 -s 114.114.114.114 -f 参考资料/dnsrelay.txt -c 1024 -v
```

参数：`-b` 绑定 IP · `-p` 端口 · `-s` 上游 DNS · `-f` 配置文件 · `-c` 缓存容量 · `-v/-vv` 日志级别。

启动后加载 `参考资料/dnsrelay.txt`（208 条记录）；加载失败则仅以中继模式运行。

**WSL 提示**：若 `bind: Address already in use`，可先 `sudo systemctl stop systemd-resolved`，或使用测试端口：

```bash
sudo DNS_RELAY_BIND=127.0.0.1 DNS_RELAY_PORT=5353 ./dnsrelay
python3 scripts/dns_query.py 127.0.0.1 5353 bupt
```

正式验收与课程演示仍使用默认 **53 端口**（不设置 `DNS_RELAY_PORT`）。

## 测试

将系统 DNS 指向 `127.0.0.1`，或使用 `nslookup` 指定服务器：

```bash
nslookup bupt 127.0.0.1        # 本地解析 → 123.127.134.10
nslookup sina 127.0.0.1        # 本地解析 → 202.108.33.89
nslookup 008.cn 127.0.0.1      # 拦截 → NXDOMAIN
nslookup baidu.com 127.0.0.1   # 中继 → 公网真实 IP
```

## 项目结构

```
include/              # 头文件（协议、配置、缓存、日志、选项）
src/                  # C 实现
scripts/              # 验证、截图、压测脚本
diagrams/             # 报告用 SVG 示意图
docs/
  report/             # 实验报告（.typ / .md / .pdf）
  screenshots/        # 终端验证截图
  verification/       # 集成测试日志
  dev/                # 开发任务清单
参考资料/             # dnsrelay.txt（程序配置）
Makefile
README.md
```

## 集成测试

**Linux / WSL：**

```bash
bash scripts/run_verification.sh
sudo sh scripts/test_dns.sh
python3 scripts/dns_query.py 127.0.0.1 5353 bupt 008.cn baidu.com
```

**Windows PowerShell：**

```powershell
.\scripts\run_verification.ps1
# 或一键验证 + 截图 + PDF：
.\scripts\verify_and_screenshot.ps1
```

输出保存至 `docs/verification/` 与 `docs/test-output.txt`。

## 文档与报告

| 文档 | 路径 |
|------|------|
| 实验报告（Markdown） | [docs/report/实验报告.md](docs/report/实验报告.md) |
| 实验报告（Typst 源） | [docs/report/实验报告.typ](docs/report/实验报告.typ) |
| 分支说明 | [docs/BRANCHES.md](docs/BRANCHES.md) |
| 文档索引 | [docs/README.md](docs/README.md) |

编译 PDF 并复制到根目录（课程提交）：

```bash
make report
```

## 交付清单

`实验报告.pdf`、`README.md`、`include/`、`src/`、`Makefile`、`参考资料/dnsrelay.txt`、`.gitignore`
