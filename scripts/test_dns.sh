#!/bin/sh
# DNS-Relay-Server 集成测试（WSL/Linux）
set -e
cd "$(dirname "$0")/.."

make clean && make

export DNS_RELAY_BIND="${DNS_RELAY_BIND:-127.0.0.1}"
export DNS_RELAY_PORT="${DNS_RELAY_PORT:-5353}"

OUT=docs/test-output.txt
SERVER_LOG=docs/test-server.log
mkdir -p docs

./dnsrelay -b "$DNS_RELAY_BIND" -p "$DNS_RELAY_PORT" \
    -s 114.114.114.114 -f 参考资料/dnsrelay.txt -c 1024 -v \
    >"$SERVER_LOG" 2>&1 &
PID=$!
trap 'kill "$PID" 2>/dev/null || true' EXIT
sleep 2

{
    echo "bind=$DNS_RELAY_BIND port=$DNS_RELAY_PORT"
    echo ""
    echo "========== basic queries =========="
    python3 scripts/dns_query.py "$DNS_RELAY_BIND" "$DNS_RELAY_PORT" bupt 008.cn baidu.com
    echo ""
    echo "========== cache probe =========="
    python3 scripts/dns_query.py "$DNS_RELAY_BIND" "$DNS_RELAY_PORT" baidu.com baidu.com
    echo ""
    echo "========== qtype / qclass =========="
    python3 scripts/dns_query.py "$DNS_RELAY_BIND" "$DNS_RELAY_PORT" bupt:MX
    echo ""
    echo "========== nslookup =========="
    if command -v nslookup >/dev/null 2>&1; then
        nslookup -port="$DNS_RELAY_PORT" bupt "$DNS_RELAY_BIND" 2>&1 || true
        nslookup -port="$DNS_RELAY_PORT" 008.cn "$DNS_RELAY_BIND" 2>&1 || true
        nslookup -port="$DNS_RELAY_PORT" baidu.com "$DNS_RELAY_BIND" 2>&1 || true
    else
        echo "nslookup not installed"
    fi
    echo ""
    echo "========== server log =========="
    cat "$SERVER_LOG"
} | tee "$OUT"
