# DNS-Relay-Server

北京邮电大学（BUPT）计算机网络课程设计 —— **DNS 中继服务器**

基于 RFC 1035 实现 UDP DNS 中继：支持本地拦截、本地解析与上游转发，主循环使用 `select()` 事件驱动（10ms 超时），避免忙等待。

## 功能

| 模式 | 条件 | 行为 |
|------|------|------|
| 本地拦截 | `dnsrelay.txt` 中 IP 为 `0.0.0.0` | 返回 NXDOMAIN（RCODE=3） |
| 本地解析 | 配置表中为真实 IPv4 | 返回 A 记录（仅响应 QTYPE=A） |
| 上游中继 | 域名不在表中 | 转发至 `114.114.114.114`，还原 Transaction ID |

## 编译

需要 Linux / WSL（`arpa/inet.h`、`sys/select.h` 等 POSIX 接口）。

```bash
make clean && make
```

## 运行

绑定 53 端口通常需要 root 权限：

```bash
sudo ./dnsrelay
```

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

## 配置文件

路径：`参考资料/dnsrelay.txt`

每行格式：`IP 域名`（空格分隔）

- `0.0.0.0 域名` — 拦截
- `x.x.x.x 域名` — 本地 A 记录
- 未列出域名 — 中继到上游 DNS

## 项目结构

```
include/          # 头文件（dns_protocol, config, id_map）
src/              # 实现
参考资料/         # dnsrelay.txt、RFC 文档
Makefile
实验报告.md       # 课程设计报告（导出 PDF 后提交）
```

## 集成测试

**Linux / WSL 终端内：**

```bash
cd /mnt/c/projects/DNS-Relay-Server   # 或你的克隆路径
bash scripts/run_verification.sh
sudo sh scripts/test_dns.sh
python3 scripts/dns_query.py 127.0.0.1 5353 bupt 008.cn baidu.com
```

**Windows PowerShell（项目已在 `C:\projects\DNS-Relay-Server`）：**

```powershell
# 不要用 cd /mnt/c/... —— 那是 WSL 路径，PowerShell 无法识别
.\scripts\run_verification.ps1
# 或一行：
wsl bash -c "cd /mnt/c/projects/DNS-Relay-Server && bash scripts/run_verification.sh"
```

输出保存至 `docs/verification/` 与 `docs/test-output.txt`。

生成报告用终端截图：

```bash
python3 scripts/gen_terminal_screenshots.py   # → docs/screenshots/terminal-*.png
typst compile 实验报告.typ 实验报告.pdf
```

## 文档

详细实验报告见 [实验报告.md](./实验报告.md)。提交前导出 `实验报告.pdf`（Typora / VS Code Markdown PDF 插件 / Pandoc）。

## 交付清单

`实验报告.pdf`、`README.md`、`include/`、`src/`、`Makefile`、`参考资料/dnsrelay.txt`、`.gitignore`
