#!/usr/bin/env bash
# 兼容入口 → platform/linux/verify/verify_and_screenshot.sh
exec "$(dirname "$0")/../platform/linux/verify/verify_and_screenshot.sh" "$@"
