#!/usr/bin/env bash
# Full course verification — one Screenshot section per step, logs for PNG generation.
# Run in WSL: bash scripts/run_verification.sh
# PowerShell: .\scripts\run_verification.ps1
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OUT="$ROOT/docs/verification"
PORT="${DNS_RELAY_PORT:-15353}"
BIND="${DNS_RELAY_BIND:-127.0.0.1}"

mkdir -p "$OUT"
pkill -f './dnsrelay' 2>/dev/null || true
sleep 1

# dig/nslookup 用 TAB 对齐字段；日志与 PNG 须展开为空格，否则 PIL 画成方框乱码
expand_tabs() {
  sed 's/\t/    /g'
}

run_cmd() {
  echo "$ $*"
  "$@" 2>&1 | expand_tabs || true
  echo ""
}

run_dig() {
  echo "$ dig $@"
  dig "$@" 2>&1 | expand_tabs || true
  echo ""
}

run_nslookup() {
  echo "$ nslookup -port=${PORT} $1 ${BIND}"
  nslookup -port="$PORT" "$1" "$BIND" 2>&1 | expand_tabs || true
  echo ""
}

{
  echo "Screenshot 1: build make clean && make"
  echo "$ make clean && make"
  make clean && make
} >"$OUT/01-build.log" 2>&1

DNS_RELAY_BIND="$BIND" DNS_RELAY_PORT="$PORT" stdbuf -oL ./dnsrelay \
  >"$OUT/02-server-stdout.log" 2>"$OUT/02-server-startup.log" &
SPID=$!
cleanup() {
  kill "$SPID" 2>/dev/null || true
  wait "$SPID" 2>/dev/null || true
}
trap cleanup EXIT
for _ in 1 2 3 4 5 6; do
  if [ -s "$OUT/02-server-stdout.log" ]; then
    break
  fi
  sleep 0.5
done

{
  echo "Screenshot 2: server startup (stderr + stdout)"
  echo "$ DNS_RELAY_BIND=$BIND DNS_RELAY_PORT=$PORT ./dnsrelay"
  echo "--- stderr (config load) ---"
  cat "$OUT/02-server-startup.log"
  echo "--- stdout (listen) ---"
  cat "$OUT/02-server-stdout.log"
  echo ""

  echo "Screenshot 3: course case1 nslookup bupt local A"
  run_nslookup bupt

  echo "Screenshot 4: course case2 nslookup 008.cn block NXDOMAIN"
  run_nslookup 008.cn

  echo "Screenshot 5: course case3 nslookup baidu.com upstream relay"
  run_nslookup baidu.com

  echo "Screenshot 6: config test0 block (0.0.0.0 test0)"
  run_nslookup test0

  echo "Screenshot 7: config test1 local (11.111.11.111 test1)"
  run_nslookup test1

  echo "Screenshot 8: nslookup sina second local record"
  run_nslookup sina

  echo "Screenshot 9: fix-A nslookup mx bupt empty NOERROR"
  echo "$ nslookup -port=${PORT} -type=mx bupt ${BIND}"
  nslookup -port="$PORT" -type=mx bupt "$BIND" 2>&1 | expand_tabs || true
  echo ""

  echo "Screenshot 10: dns_query.py protocol check"
  echo "$ python3 scripts/dns_query.py ${BIND} ${PORT} bupt 008.cn baidu.com test0 test1"
  python3 scripts/dns_query.py "$BIND" "$PORT" bupt 008.cn baidu.com test0 test1 2>&1 | expand_tabs || true
  echo ""

  echo "Screenshot 11: dig bupt A +noall +answer +comments"
  run_dig "@$BIND" -p "$PORT" bupt A +noall +answer +comments

  echo "Screenshot 12: dig 008.cn A +noall +answer +comments"
  run_dig "@$BIND" -p "$PORT" 008.cn A +noall +answer +comments

  echo "Screenshot 13: dig baidu.com A +noall +answer +comments"
  run_dig "@$BIND" -p "$PORT" baidu.com A +noall +answer +comments

  echo "Screenshot 14: fix-B SERVFAIL (iptables block 114.114.114.114)"
  echo "$ sudo iptables -A OUTPUT -d 114.114.114.114 -j DROP"
  echo "$ dig @${BIND} -p ${PORT} not-in-config-xyz123.com A +time=5 +comments"
  if command -v iptables >/dev/null 2>&1; then
    iptables -C OUTPUT -d 114.114.114.114 -j DROP 2>/dev/null \
      || iptables -A OUTPUT -d 114.114.114.114 -j DROP
    dig "@$BIND" -p "$PORT" not-in-config-xyz123.com A +time=5 +comments 2>&1 | expand_tabs || true
    iptables -D OUTPUT -d 114.114.114.114 -j DROP 2>/dev/null || true
    echo "(iptables rule removed)"
  else
    echo "iptables not available — see docs/verification/04-fixB-servfail-note.txt"
    dig "@$BIND" -p "$PORT" not-in-config-xyz123.com A +time=5 +comments 2>&1 | expand_tabs || true
  fi
  echo ""
} | tee "$OUT/03-full-verification.log"

echo "Done. Logs: $OUT/01-build.log $OUT/03-full-verification.log"
