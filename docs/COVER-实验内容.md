# 课程设计封面 — 实验内容（更新版）

> 适用：北京邮电大学《计算机网络》DNS 中继服务器课程设计封面「实验内容」栏  
> 组别：2024211301 · 张恒基 / 尹浩铭 / 林旭东

---

## 实验内容（可直接粘贴）

本次课程设计在 **Windows 原生与 Linux/WSL 双平台** 下完成开发与验收：依据 RFC 1035 与课设任务书，采用 C11 与跨平台网络接口（POSIX Socket / Winsock，`net_compat` 统一适配）实现运行于 UDP 53（开发测试可用 15353）的 DNS 中继服务器；程序加载 dnsrelay.txt（208 条 IP–域名记录）对查询进行三路调度，在教室真实网络中完成本地拦截（0.0.0.0→NXDOMAIN）、本地解析（QTYPE=A 返 A 记录）、上游中继（转发至 114.114.114.114:53 并替换、还原 Transaction ID）三类功能，并与 Windows/Linux 下 nslookup、dig 完成互操作验证。报文严格按 RFC 1035 编解码（12 字节首部、Question/Answer、域名标签与指针压缩）；主交付采用 **同步中继** 模型，扩展实现 **异步双 socket + select() 10ms 非阻塞中继**，显著提升教室多人并发下的响应能力；针对 Windows nslookup 的 EDNS 与反向 DNS 查询，修正错误响应组包并增设 FAST 快速路径，消除客户端 2 秒 timeout 误报。另实现 TTL 缓存、ID 映射、CLI（-b/-p/-s/-f/-c/-v）及 fix-A、fix-B（上游故障 SERVFAIL）、NOTIMP 等边界处理；源码模块为 dns_protocol、config、id_map、dns_cache、main，Make/MinGW 编译零告警。测试含 **14 步跨平台自动化脚本**（PowerShell / Bash）与终端截图，必测 bupt、008.cn、baidu.com 三类用例，辅以 dig 协议对照与上游故障模拟；详见「实验报告和源程序」。

---

## 字数说明

- 原版约 390 字；本版约 430 字，结构一致、信息增量主要在双平台、异步扩展与 timeout 修复。
