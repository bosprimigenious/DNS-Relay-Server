#!/usr/bin/env bash
# WSL 内一键验证 + 截图 + PDF（fix-B 需 root）
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
sed -i 's/\r$//' scripts/run_verification.sh 2>/dev/null || true

if [ "$(id -u)" -ne 0 ]; then
  echo "Re-running as root for iptables (fix-B)..."
  exec sudo bash "$0"
fi

bash scripts/run_verification.sh
python3 scripts/gen_terminal_screenshots.py
if command -v typst >/dev/null 2>&1; then
  typst compile 实验报告.typ 实验报告.pdf
  echo "PDF: $ROOT/实验报告.pdf"
fi
echo "PNG: $ROOT/docs/screenshots/terminal-*.png"
