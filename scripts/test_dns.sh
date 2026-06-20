#!/usr/bin/env bash
# 兼容入口 → platform/linux/verify/test_dns.sh
exec "$(dirname "$0")/../platform/linux/verify/test_dns.sh" "$@"
