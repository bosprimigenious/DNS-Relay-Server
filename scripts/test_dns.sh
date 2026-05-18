#!/bin/sh
# DNS-Relay-Server 集成测试（WSL/Linux，root）
set -e
cd "$(dirname "$0")/.."

make clean && make

if [ "$(id -u)" -ne 0 ]; then
    echo "请使用: sudo sh scripts/test_dns.sh"
    exit 1
fi

systemctl stop systemd-resolved 2>/dev/null || true
export DNS_RELAY_BIND="${DNS_RELAY_BIND:-127.0.0.1}"
export DNS_RELAY_PORT="${DNS_RELAY_PORT:-5353}"

./dnsrelay &
PID=$!
trap 'kill "$PID" 2>/dev/null || true' EXIT
sleep 2

OUT=docs/test-output.txt
mkdir -p docs

{
    echo "DNS_RELAY_BIND=$DNS_RELAY_BIND DNS_RELAY_PORT=$DNS_RELAY_PORT"
    echo ""
    python3 scripts/dns_query.py 127.0.0.1 "$DNS_RELAY_PORT" bupt 008.cn baidu.com
    echo ""
    echo "========== nslookup（若已安装 dnsutils）=========="
    if command -v nslookup >/dev/null 2>&1; then
        nslookup -port="$DNS_RELAY_PORT" bupt 127.0.0.1 2>&1 || true
        nslookup -port="$DNS_RELAY_PORT" 008.cn 127.0.0.1 2>&1 || true
        nslookup -port="$DNS_RELAY_PORT" baidu.com 127.0.0.1 2>&1 || true
    else
        echo "nslookup not installed"
    fi
} | tee "$OUT"
