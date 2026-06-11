#!/usr/bin/env bash
# dnsperf load test against dnsrelay (run inside WSL, not PowerShell).
# Light (default): -c 10 -l 10 — suitable for report screenshots.
# Stress (--stress): ~600k queries — shows sync relay limits (high timeout rate).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
VER="$ROOT/docs/verification"
mkdir -p "$VER"

MODE="${1:-light}"
if [[ "$MODE" == "--stress" ]]; then
  MODE="stress"
fi

if ! command -v dnsperf >/dev/null 2>&1; then
  echo "dnsperf not found. In WSL run: sudo apt install -y dnsperf"
  echo "(Enable sudo in Windows Settings > System > For developers if needed.)"
  exit 1
fi

if [[ ! -x ./dnsrelay ]]; then
  make -s
fi

QUERY_FILE="$VER/dnsrelay-queries.txt"
if [[ "$MODE" == "stress" ]]; then
  cat >"$QUERY_FILE" <<'EOF'
bupt A
sina A
test1 A
baidu.com A
EOF
else
  cat >"$QUERY_FILE" <<'EOF'
bupt A
sina A
test1 A
EOF
fi

BIND="${DNS_RELAY_BIND:-127.0.0.1}"
PORT="${DNS_RELAY_PORT:-15353}"
LOG="$VER/12-dnsperf-${MODE}.log"

if [[ "$MODE" == "stress" ]]; then
  CLIENTS="${DNSPERF_CLIENTS:-100}"
  TIMELIMIT="${DNSPERF_DURATION:-60}"
  MAXRUNS="${DNSPERF_MAXRUNS:-150000}"
  OUTSTANDING="${DNSPERF_OUTSTANDING:-500}"
  DNSPERF_CMD=(dnsperf -s "$BIND" -p "$PORT" -d "$QUERY_FILE"
    -c "$CLIENTS" -l "$TIMELIMIT" -n "$MAXRUNS" -q "$OUTSTANDING")
  TITLE="dnsperf stress (~600k queries, sync relay bottleneck)"
else
  CLIENTS="${DNSPERF_CLIENTS:-2}"
  TIMELIMIT="${DNSPERF_DURATION:-10}"
  DNSPERF_CMD=(dnsperf -s "$BIND" -p "$PORT" -d "$QUERY_FILE"
    -c "$CLIENTS" -l "$TIMELIMIT")
  TITLE="dnsperf light (local names only, -c ${CLIENTS} -l ${TIMELIMIT})"
fi

{
  echo "Screenshot 12: dnsperf ${MODE}"
  echo "============================================================"
  echo "$TITLE"
  echo "WSL: cd /mnt/c/projects/DNS-Relay-Server && bash scripts/run_dnsperf.sh ${MODE}"
  echo ""
  echo "Starting dnsrelay on ${BIND}:${PORT} ..."
} | tee "$LOG"

DNS_RELAY_BIND="$BIND" DNS_RELAY_PORT="$PORT" ./dnsrelay &
RELAY_PID=$!
sleep 0.5

cleanup() {
  kill "$RELAY_PID" 2>/dev/null || true
  wait "$RELAY_PID" 2>/dev/null || true
}
trap cleanup EXIT

CMD_PRINT="\$ dnsperf -s ${BIND} -p ${PORT} -d ${QUERY_FILE} -c ${CLIENTS} -l ${TIMELIMIT}"
if [[ "$MODE" == "stress" ]]; then
  CMD_PRINT="$CMD_PRINT -n ${MAXRUNS} -q ${OUTSTANDING}"
fi

{
  echo ""
  echo "$CMD_PRINT"
  echo "========== dnsperf =========="
} | tee -a "$LOG"

"${DNSPERF_CMD[@]}" 2>&1 | tee -a "$LOG"

{
  echo "============================"
  echo "[Status] Testing complete"
} | tee -a "$LOG"

SUMMARY_LOG="$VER/12-dnsperf-${MODE}-summary.log"
python3 - "$LOG" "$SUMMARY_LOG" <<'PY'
import re, sys
src, dst = sys.argv[1], sys.argv[2]
text = open(src, encoding="utf-8", errors="replace").read()
lines = text.splitlines()
out, timeouts, past_cmd = [], 0, False
for line in lines:
    if line.startswith("Screenshot ") or line.startswith("=") or line.startswith("dnsperf ") or line.startswith("WSL:"):
        out.append(line)
    elif line.startswith("Starting dnsrelay"):
        out.append(line)
    elif line.startswith("$ dnsperf"):
        out.append(line)
        past_cmd = True
    elif past_cmd and line.startswith("[Status]"):
        out.append(line)
    elif "[Timeout]" in line or "unexpected (maybe timed out)" in line:
        if timeouts < 2:
            out.append(line)
        elif timeouts == 2:
            out.append("... (timeout lines omitted) ...")
        timeouts += 1
    elif line.strip() == "Statistics:":
        idx = lines.index(line)
        out.append("")
        out.extend(lines[idx:])
        break
open(dst, "w", encoding="utf-8").write("\n".join(out).strip() + "\n")
PY

echo "Log: $LOG"
echo "Summary (for report PNG): $SUMMARY_LOG"
