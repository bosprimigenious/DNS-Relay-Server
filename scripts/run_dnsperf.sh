#!/usr/bin/env bash
# 兼容入口 → platform/linux/verify/run_dnsperf.sh
exec "$(dirname "$0")/../platform/linux/verify/run_dnsperf.sh" "$@"
