#!/usr/bin/env bash
# 兼容入口 → platform/linux/verify/show_policy_table.sh
exec "$(dirname "$0")/../platform/linux/verify/show_policy_table.sh" "$@"
