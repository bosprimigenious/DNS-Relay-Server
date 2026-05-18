#!/bin/sh
# DNS-Relay-Server 集成测试（需在 WSL/Linux 下以 root 绑定 53 端口）
set -e
cd "$(dirname "$0")/.."

make clean && make

if [ "$(id -u)" -ne 0 ]; then
    echo "请使用: sudo sh scripts/test_dns.sh"
    exit 1
fi

./dnsrelay &
PID=$!
trap 'kill "$PID" 2>/dev/null || true' EXIT
sleep 1

echo "========== 用例 1: bupt 本地解析 =========="
nslookup bupt 127.0.0.1 2>&1 || true
echo ""

echo "========== 用例 2: sina 本地解析 =========="
nslookup sina 127.0.0.1 2>&1 || true
echo ""

echo "========== 用例 3: 008.cn 拦截 =========="
nslookup 008.cn 127.0.0.1 2>&1 || true
echo ""

echo "========== 用例 4: baidu.com 中继 =========="
nslookup baidu.com 127.0.0.1 2>&1 || true
echo ""

echo "========== 用例 5: MX 查询 bupt =========="
nslookup -type=mx bupt 127.0.0.1 2>&1 || true
echo ""

echo "========== 测试完成 =========="
