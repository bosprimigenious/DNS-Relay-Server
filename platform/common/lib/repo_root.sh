#!/usr/bin/env bash
# Resolve repository root (contains Makefile and src/).

platform_repo_root() {
    local dir script_dir
    script_dir="$(cd "$(dirname "${BASH_SOURCE[1]:-${BASH_SOURCE[0]}}")" && pwd)"
    dir="$script_dir"
    while [[ "$dir" != "/" ]]; do
        if [[ -f "$dir/Makefile" && -d "$dir/src" ]]; then
            echo "$dir"
            return 0
        fi
        dir="$(dirname "$dir")"
    done
    return 1
}
