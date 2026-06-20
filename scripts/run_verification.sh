#!/usr/bin/env bash
# 兼容入口 → platform/linux/verify/run_verification.sh
exec "$(dirname "$0")/../platform/linux/verify/run_verification.sh" "$@"
