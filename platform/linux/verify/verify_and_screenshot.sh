#!/usr/bin/env bash
# WSL 内一键验证 + 截图 + PDF（fix-B 需 root）
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
VERIFY="$ROOT/platform/linux/verify"
PY="$ROOT/platform/common/python"
cd "$ROOT"
sed -i 's/\r$//' "$VERIFY/run_verification.sh" 2>/dev/null || true

if [ "$(id -u)" -ne 0 ]; then
  echo "Re-running as root for iptables (fix-B)..."
  exec sudo bash "$0"
fi

bash "$VERIFY/run_verification.sh"
python3 "$PY/gen_terminal_screenshots.py"
if command -v typst >/dev/null 2>&1; then
  make report
else
  echo "typst not found; skip PDF. Install typst or run: make report"
fi
echo "PNG: $ROOT/docs/screenshots/terminal-*.png"
