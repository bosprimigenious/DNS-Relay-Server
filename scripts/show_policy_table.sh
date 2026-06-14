#!/usr/bin/env bash
# Print dnsrelay.txt policy table for demo (no server required).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FILE="${1:-$ROOT/参考资料/dnsrelay.txt}"

if [ ! -f "$FILE" ]; then
    echo "config not found: $FILE" >&2
    exit 1
fi

printf '\n'
printf '+------------------+---------------+------------+----------------+\n'
printf '| Domain           | IPv4 config   | A query    | AAAA query     |\n'
printf '+------------------+---------------+------------+----------------+\n'
printf '| baidu.com        | (not in file) | RELAY      | RELAY          |\n'

print_row() {
    local domain="$1"
    local ip="$2"
    local v6only="${3:-0}"

    if [ "$v6only" = "1" ]; then
        printf '| %-16s | ::              | %-10s | %-14s |\n' "$domain" "RELAY" "NXDOMAIN"
    elif [ "$ip" = "0.0.0.0" ]; then
        printf '| %-16s | %-15s | %-10s | %-14s |\n' "$domain" "$ip" "NXDOMAIN" "NXDOMAIN"
    else
        printf '| %-16s | %-15s | %-10s | %-14s |\n' "$domain" "$ip" "A record" "NOERROR(empty)"
    fi
}

for name in bupt 008.cn; do
    line="$(awk -v d="$name" '{
        sub(/\r$/, "");
        if ($1 != "" && $1 !~ /^#/ && $2 == d) { print; exit }
    }' "$FILE")"
    if [ -n "$line" ]; then
        ip="$(echo "$line" | awk '{print $1}')"
        if [ "$ip" = "::" ]; then
            print_row "$name" "$ip" 1
        else
            print_row "$name" "$ip" 0
        fi
    fi
done

shown=0
while IFS= read -r line; do
    line="${line%%$'\r'}"
    [ -z "$line" ] && continue
    [ "${line#\#}" != "$line" ] && continue
    ip="$(echo "$line" | awk '{print $1}')"
    domain="$(echo "$line" | awk '{print $2}')"
    case "$domain" in bupt|008.cn) continue ;; esac
    if [ "$ip" = "::" ]; then
        print_row "$domain" "$ip" 1
        shown=$((shown + 1))
    elif [ "$ip" = "0.0.0.0" ]; then
        print_row "$domain" "$ip" 0
        shown=$((shown + 1))
    fi
    [ "$shown" -ge 8 ] && break
done <"$FILE"

total="$(grep -vcE '^[[:space:]]*($|#)' "$FILE" || true)"
printf '+------------------+---------------+------------+----------------+\n'
printf '| 0.0.0.0 = full block (A+AAAA NXDOMAIN)                       |\n'
printf '| :: = IPv6-only block (A relay, AAAA NXDOMAIN)                 |\n'
printf '| other IPv4 = local A; AAAA empty NOERROR (fix-A)             |\n'
printf '| not in table = upstream relay                                |\n'
printf '+--------------------------------------------------------------+\n'
printf 'Config file: %s\n' "$FILE"
printf 'Total entries: %s (table shows course cases + samples)\n\n' "$total"
