# -*- coding: utf-8 -*-
from pathlib import Path

DIAGRAMS = Path(__file__).resolve().parent.parent / "diagrams"
DIAGRAMS.mkdir(exist_ok=True)

ARCH = """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="800" height="400" viewBox="0 0 800 400">
  <defs>
    <marker id="arrow" markerWidth="10" markerHeight="8" refX="9" refY="4" orient="auto">
      <path d="M0,0 L10,4 L0,8 Z" fill="#2563EB"/>
    </marker>
    <filter id="shadow" x="-4%" y="-4%" width="108%" height="108%">
      <feDropShadow dx="0" dy="2" stdDeviation="3" flood-color="#94A3B8" flood-opacity="0.35"/>
    </filter>
  </defs>
  <rect width="800" height="400" fill="#F8FAFC"/>
  <g filter="url(#shadow)">
    <rect x="30" y="155" width="120" height="90" rx="12" fill="#FFFFFF" stroke="#94A3B8" stroke-width="2"/>
    <text x="90" y="195" text-anchor="middle" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="15" font-weight="bold" fill="#1E293B">客户端</text>
    <text x="90" y="218" text-anchor="middle" font-family="Consolas, monospace" font-size="12" fill="#64748B">DNS 查询</text>
  </g>
  <g filter="url(#shadow)">
    <rect x="220" y="40" width="360" height="320" rx="16" fill="#FFFFFF" stroke="#2563EB" stroke-width="2.5"/>
    <rect x="220" y="40" width="360" height="44" rx="16" fill="#2563EB"/>
    <rect x="220" y="68" width="360" height="16" fill="#2563EB"/>
    <text x="400" y="70" text-anchor="middle" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="16" font-weight="bold" fill="#FFFFFF">DNS-Relay-Server</text>
    <text x="400" y="108" text-anchor="middle" font-family="Consolas, monospace" font-size="13" fill="#334155">main.c · select() 主循环</text>
    <rect x="250" y="125" width="300" height="36" rx="8" fill="#EFF6FF" stroke="#3B82F6" stroke-width="1.5"/>
    <text x="400" y="148" text-anchor="middle" font-family="Consolas, monospace" font-size="12" fill="#1D4ED8">dns_parse_query()</text>
    <line x1="400" y1="161" x2="400" y2="178" stroke="#2563EB" stroke-width="2" marker-end="url(#arrow)"/>
    <rect x="250" y="178" width="300" height="36" rx="8" fill="#EFF6FF" stroke="#3B82F6" stroke-width="1.5"/>
    <text x="400" y="201" text-anchor="middle" font-family="Consolas, monospace" font-size="12" fill="#1D4ED8">config_lookup() → 本地表</text>
    <rect x="245" y="235" width="95" height="52" rx="8" fill="#FEF2F2" stroke="#EF4444" stroke-width="1.5"/>
    <text x="292" y="258" text-anchor="middle" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="11" fill="#B91C1C">0.0.0.0</text>
    <text x="292" y="275" text-anchor="middle" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="11" font-weight="bold" fill="#DC2626">NXDOMAIN</text>
    <rect x="352" y="235" width="95" height="52" rx="8" fill="#F0FDF4" stroke="#22C55E" stroke-width="1.5"/>
    <text x="400" y="258" text-anchor="middle" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="11" fill="#15803D">真实 IPv4</text>
    <text x="400" y="275" text-anchor="middle" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="11" font-weight="bold" fill="#16A34A">A 记录</text>
    <rect x="460" y="235" width="95" height="52" rx="8" fill="#EFF6FF" stroke="#2563EB" stroke-width="1.5"/>
    <text x="507" y="262" text-anchor="middle" font-family="Consolas, monospace" font-size="10" fill="#1D4ED8">relay_to_upstream()</text>
    <line x1="400" y1="214" x2="292" y2="235" stroke="#94A3B8" stroke-width="1.5"/>
    <line x1="400" y1="214" x2="400" y2="235" stroke="#94A3B8" stroke-width="1.5"/>
    <line x1="400" y1="214" x2="507" y2="235" stroke="#94A3B8" stroke-width="1.5"/>
    <text x="400" y="318" text-anchor="middle" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="11" fill="#64748B">本地拦截 / 本地解析 / 上游中继</text>
  </g>
  <g filter="url(#shadow)">
    <rect x="650" y="155" width="120" height="90" rx="12" fill="#FFFFFF" stroke="#94A3B8" stroke-width="2"/>
    <text x="710" y="192" text-anchor="middle" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="13" font-weight="bold" fill="#1E293B">上游 DNS</text>
    <text x="710" y="215" text-anchor="middle" font-family="Consolas, monospace" font-size="11" fill="#64748B">114.114.114.114</text>
    <text x="710" y="232" text-anchor="middle" font-family="Consolas, monospace" font-size="11" fill="#64748B">:53</text>
  </g>
  <line x1="150" y1="200" x2="218" y2="200" stroke="#2563EB" stroke-width="2.5" marker-end="url(#arrow)"/>
  <text x="184" y="188" text-anchor="middle" font-family="Consolas, monospace" font-size="11" fill="#2563EB">UDP:53</text>
  <line x1="580" y1="261" x2="648" y2="210" stroke="#2563EB" stroke-width="2" marker-end="url(#arrow)"/>
</svg>
"""

DNS = """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="800" height="500" viewBox="0 0 800 500">
  <defs>
    <marker id="dn" markerWidth="8" markerHeight="6" refX="7" refY="3" orient="auto">
      <path d="M0,0 L8,3 L0,6 Z" fill="#94A3B8"/>
    </marker>
  </defs>
  <rect width="800" height="500" fill="#F8FAFC"/>
  <text x="400" y="28" text-anchor="middle" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="16" font-weight="bold" fill="#1E293B">DNS 报文结构（RFC 1035）</text>
  <rect x="40" y="45" width="720" height="130" rx="10" fill="#DBEAFE" stroke="#2563EB" stroke-width="2"/>
  <text x="60" y="68" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="14" font-weight="bold" fill="#1D4ED8">Header（12 字节）</text>
  <rect x="55" y="80" width="90" height="42" rx="4" fill="#FFFFFF" stroke="#3B82F6"/>
  <text x="100" y="98" text-anchor="middle" font-family="Consolas, monospace" font-size="11" fill="#1E293B">ID</text>
  <text x="100" y="114" text-anchor="middle" font-family="Consolas, monospace" font-size="10" fill="#64748B">16 bit</text>
  <rect x="155" y="80" width="310" height="42" rx="4" fill="#FFFFFF" stroke="#3B82F6"/>
  <text x="310" y="96" text-anchor="middle" font-family="Consolas, monospace" font-size="10" fill="#1E293B">FLAGS（16 bit）</text>
  <text x="310" y="112" text-anchor="middle" font-family="Consolas, monospace" font-size="9" fill="#64748B">QR | OPCODE | AA TC RD RA Z | RCODE</text>
  <rect x="475" y="80" width="68" height="42" rx="4" fill="#FFFFFF" stroke="#3B82F6"/>
  <text x="509" y="98" text-anchor="middle" font-family="Consolas, monospace" font-size="10">QDCOUNT</text>
  <rect x="553" y="80" width="68" height="42" rx="4" fill="#FFFFFF" stroke="#3B82F6"/>
  <text x="587" y="98" text-anchor="middle" font-family="Consolas, monospace" font-size="10">ANCOUNT</text>
  <rect x="631" y="80" width="58" height="42" rx="4" fill="#FFFFFF" stroke="#3B82F6"/>
  <text x="660" y="98" text-anchor="middle" font-family="Consolas, monospace" font-size="9">NSCOUNT</text>
  <rect x="699" y="80" width="50" height="42" rx="4" fill="#FFFFFF" stroke="#3B82F6"/>
  <text x="724" y="98" text-anchor="middle" font-family="Consolas, monospace" font-size="9">ARCOUNT</text>
  <path d="M400 175 L400 195" stroke="#94A3B8" stroke-width="2" marker-end="url(#dn)"/>
  <rect x="40" y="195" width="720" height="115" rx="10" fill="#D1FAE5" stroke="#059669" stroke-width="2"/>
  <text x="60" y="218" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="14" font-weight="bold" fill="#047857">Question Section</text>
  <rect x="55" y="230" width="200" height="40" rx="4" fill="#FFFFFF" stroke="#10B981"/>
  <text x="155" y="248" text-anchor="middle" font-family="Consolas, monospace" font-size="11">QNAME</text>
  <text x="155" y="264" text-anchor="middle" font-family="Consolas, monospace" font-size="10" fill="#64748B">标签 + 0x00</text>
  <rect x="270" y="230" width="100" height="40" rx="4" fill="#FFFFFF" stroke="#10B981"/>
  <text x="320" y="248" text-anchor="middle" font-family="Consolas, monospace" font-size="11">QTYPE</text>
  <rect x="385" y="230" width="100" height="40" rx="4" fill="#FFFFFF" stroke="#10B981"/>
  <text x="435" y="248" text-anchor="middle" font-family="Consolas, monospace" font-size="11">QCLASS</text>
  <path d="M400 310 L400 330" stroke="#94A3B8" stroke-width="2" marker-end="url(#dn)"/>
  <rect x="40" y="330" width="720" height="150" rx="10" fill="#FEF3C7" stroke="#D97706" stroke-width="2"/>
  <text x="60" y="353" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="14" font-weight="bold" fill="#B45309">Answer RR Section</text>
  <rect x="55" y="365" width="120" height="36" rx="4" fill="#FFFFFF" stroke="#F59E0B"/>
  <text x="115" y="388" text-anchor="middle" font-family="Consolas, monospace" font-size="10">NAME (ptr)</text>
  <rect x="185" y="365" width="55" height="36" rx="4" fill="#FFFFFF" stroke="#F59E0B"/>
  <text x="212" y="388" text-anchor="middle" font-family="Consolas, monospace" font-size="9">TYPE</text>
  <rect x="248" y="365" width="55" height="36" rx="4" fill="#FFFFFF" stroke="#F59E0B"/>
  <text x="275" y="388" text-anchor="middle" font-family="Consolas, monospace" font-size="9">CLASS</text>
  <rect x="311" y="365" width="55" height="36" rx="4" fill="#FFFFFF" stroke="#F59E0B"/>
  <text x="338" y="388" text-anchor="middle" font-family="Consolas, monospace" font-size="9">TTL</text>
  <rect x="374" y="365" width="70" height="36" rx="4" fill="#FFFFFF" stroke="#F59E0B"/>
  <text x="409" y="388" text-anchor="middle" font-family="Consolas, monospace" font-size="9">RDLENGTH</text>
  <rect x="454" y="365" width="290" height="36" rx="4" fill="#FFFFFF" stroke="#F59E0B"/>
  <text x="599" y="388" text-anchor="middle" font-family="Consolas, monospace" font-size="10">RDATA（IPv4 A 4 字节）</text>
  <text x="115" y="430" text-anchor="middle" font-family="Consolas, monospace" font-size="9" fill="#64748B">指针压缩 0xC0 0x0C → QNAME</text>
</svg>
"""

for name, content in [
    ("architecture.svg", ARCH),
    ("dnspacket.svg", DNS),
]:
    (DIAGRAMS / name).write_text(content, encoding="utf-8")
    print("wrote", name)

flowchart_path = DIAGRAMS / "flowchart.svg"
if flowchart_path.exists():
    print("skip flowchart.svg (hand-maintained)")
else:
    print("warning: flowchart.svg missing — create diagrams/flowchart.svg manually")
