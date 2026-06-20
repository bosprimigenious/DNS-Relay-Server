# Linux / WSL

## 编译

在项目根目录：

```bash
make clean && make    # 产物 ./dnsrelay
```

## 验收

```bash
bash platform/linux/verify/run_verification.sh
bash platform/linux/verify/verify_and_screenshot.sh   # 需 root（fix-B iptables）
bash platform/linux/verify/test_dns.sh
bash platform/linux/verify/run_dnsperf.sh light
```

兼容：`bash scripts/run_verification.sh` 会转发到本目录。

## 依赖

```bash
sudo apt install bind9-dnsutils    # dig / nslookup
sudo apt install -y dnsperf        # 可选压测
```
