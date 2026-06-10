#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent / "diagrams"

DIAGRAMS = {
    "report-roadmap.svg": """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="760" height="220" viewBox="0 0 760 220">
  <rect width="760" height="220" fill="#F8FAFC"/>
  <text x="380" y="28" text-anchor="middle" font-family="Microsoft YaHei, SimHei, sans-serif" font-size="16" font-weight="bold" fill="#1E3A8A">报告阅读路线图</text>
  <rect x="24" y="48" width="150" height="130" rx="12" fill="#EFF6FF" stroke="#3B82F6" stroke-width="2"/>
  <text x="99" y="72" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="13" font-weight="bold" fill="#1D4ED8">需求</text>
  <text x="99" y="100" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="10" fill="#334155">图1-2 三种模式</text>
  <rect x="206" y="48" width="150" height="130" rx="12" fill="#F0FDF4" stroke="#22C55E" stroke-width="2"/>
  <text x="269" y="72" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="13" font-weight="bold" fill="#15803D">设计</text>
  <text x="269" y="100" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="10" fill="#334155">图3-7 架构流程</text>
  <rect x="402" y="48" width="150" height="130" rx="12" fill="#FEF3C7" stroke="#D97706" stroke-width="2"/>
  <text x="475" y="72" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="13" font-weight="bold" fill="#B45309">实现</text>
  <text x="475" y="100" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="10" fill="#334155">源码+dig对照</text>
  <rect x="594" y="48" width="150" height="130" rx="12" fill="#FEE2E2" stroke="#EF4444" stroke-width="2"/>
  <text x="669" y="72" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="13" font-weight="bold" fill="#B91C1C">测试</text>
  <text x="669" y="100" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="10" fill="#334155">14步终端实录</text>
  <text x="380" y="200" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="11" fill="#64748B">先看图 - 对照源码 - 用PNG验证RCODE</text>
</svg>
""",
    "rcode-map.svg": """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="720" height="300" viewBox="0 0 720 300">
  <rect width="720" height="300" fill="#F8FAFC"/>
  <text x="360" y="28" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="16" font-weight="bold" fill="#1E3A8A">RCODE 与业务分支对照</text>
  <rect x="24" y="52" width="155" height="100" rx="10" fill="#DCFCE7" stroke="#22C55E" stroke-width="2"/>
  <text x="99" y="78" text-anchor="middle" font-family="Consolas, monospace" font-size="14" font-weight="bold" fill="#15803D">RCODE 0</text>
  <text x="99" y="98" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="12" fill="#166534">NOERROR</text>
  <text x="99" y="130" text-anchor="middle" font-family="Consolas, monospace" font-size="9" fill="#64748B">bupt / dig 步骤11</text>
  <rect x="191" y="52" width="155" height="100" rx="10" fill="#FEE2E2" stroke="#EF4444" stroke-width="2"/>
  <text x="276" y="78" text-anchor="middle" font-family="Consolas, monospace" font-size="14" font-weight="bold" fill="#B91C1C">RCODE 3</text>
  <text x="276" y="98" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="12" fill="#DC2626">NXDOMAIN</text>
  <text x="276" y="130" text-anchor="middle" font-family="Consolas, monospace" font-size="9" fill="#64748B">008.cn 步骤4/12</text>
  <rect x="382" y="52" width="155" height="100" rx="10" fill="#FEF3C7" stroke="#D97706" stroke-width="2"/>
  <text x="477" y="78" text-anchor="middle" font-family="Consolas, monospace" font-size="14" font-weight="bold" fill="#B45309">RCODE 2</text>
  <text x="477" y="98" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="12" fill="#D97706">SERVFAIL</text>
  <text x="477" y="130" text-anchor="middle" font-family="Consolas, monospace" font-size="9" fill="#64748B">iptables 步骤14</text>
  <rect x="549" y="52" width="155" height="100" rx="10" fill="#EFF6FF" stroke="#3B82F6" stroke-width="2"/>
  <text x="626" y="78" text-anchor="middle" font-family="Consolas, monospace" font-size="14" font-weight="bold" fill="#1D4ED8">RCODE 1</text>
  <text x="626" y="98" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="12" fill="#1E40AF">FORMERR</text>
  <text x="626" y="130" text-anchor="middle" font-family="Consolas, monospace" font-size="9" fill="#64748B">解析失败</text>
  <rect x="24" y="168" width="210" height="88" rx="10" fill="#DCFCE7" stroke="#22C55E"/>
  <text x="135" y="210" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="11" fill="#15803D">绿色 本地解析</text>
  <rect x="254" y="168" width="210" height="88" rx="10" fill="#FEE2E2" stroke="#EF4444"/>
  <text x="355" y="210" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="11" fill="#B91C1C">红色 本地拦截</text>
  <rect x="484" y="168" width="220" height="88" rx="10" fill="#EFF6FF" stroke="#2563EB"/>
  <text x="600" y="210" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="11" fill="#1D4ED8">蓝色 上游中继</text>
</svg>
""",
    "verify-pipeline.svg": """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="720" height="160" viewBox="0 0 720 160">
  <rect width="720" height="160" fill="#F8FAFC"/>
  <text x="360" y="26" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="15" font-weight="bold" fill="#1E3A8A">自动化验证与截图流水线</text>
  <rect x="16" y="48" width="118" height="56" rx="8" fill="#2563EB"/>
  <text x="71" y="78" text-anchor="middle" font-family="Consolas, monospace" font-size="9" fill="#FFFFFF">run_verification.sh</text>
  <rect x="138" y="48" width="118" height="56" rx="8" fill="#FFFFFF" stroke="#94A3B8" stroke-width="2"/>
  <text x="199" y="78" text-anchor="middle" font-family="Consolas, monospace" font-size="9" fill="#334155">verification.log</text>
  <rect x="280" y="48" width="118" height="56" rx="8" fill="#FFFFFF" stroke="#94A3B8" stroke-width="2"/>
  <text x="339" y="78" text-anchor="middle" font-family="Consolas, monospace" font-size="9" fill="#334155">gen_terminal_</text>
  <text x="339" y="90" text-anchor="middle" font-family="Consolas, monospace" font-size="9" fill="#334155">screenshots.py</text>
  <rect x="422" y="48" width="118" height="56" rx="8" fill="#0c0c0c" stroke="#475569"/>
  <text x="479" y="78" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="10" fill="#4ec9b0">14 PNG</text>
  <rect x="564" y="48" width="118" height="56" rx="8" fill="#EFF6FF" stroke="#3B82F6" stroke-width="2"/>
  <text x="623" y="78" text-anchor="middle" font-family="Consolas, monospace" font-size="10" fill="#1D4ED8">typst PDF</text>
  <text x="360" y="132" text-anchor="middle" font-family="Consolas, monospace" font-size="10" fill="#64748B">verify_and_screenshot.ps1</text>
</svg>
""",
    "select-io.svg": """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="640" height="200" viewBox="0 0 640 200">
  <rect width="640" height="200" fill="#F8FAFC"/>
  <text x="320" y="26" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="15" font-weight="bold" fill="#1E3A8A">select 事件驱动 I/O</text>
  <rect x="40" y="52" width="100" height="44" rx="8" fill="#2563EB"/>
  <text x="90" y="78" text-anchor="middle" font-family="Consolas, monospace" font-size="11" fill="#FFFFFF">select 10ms</text>
  <rect x="300" y="52" width="90" height="44" rx="8" fill="#EFF6FF" stroke="#3B82F6"/>
  <text x="345" y="78" text-anchor="middle" font-family="Consolas, monospace" font-size="10" fill="#1D4ED8">continue</text>
  <rect x="430" y="52" width="100" height="44" rx="8" fill="#FFFFFF" stroke="#3B82F6" stroke-width="2"/>
  <text x="480" y="78" text-anchor="middle" font-family="Consolas, monospace" font-size="11" fill="#1E40AF">recvfrom</text>
  <rect x="540" y="52" width="90" height="44" rx="8" fill="#DCFCE7" stroke="#22C55E"/>
  <text x="585" y="78" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="10" fill="#15803D">sendto</text>
  <text x="320" y="158" text-anchor="middle" font-family="Microsoft YaHei, sans-serif" font-size="10" fill="#334155">超时 continue 省 CPU；有数据则收包处理并回到 select</text>
</svg>
""",
}

for name, content in DIAGRAMS.items():
    path = ROOT / name
    path.write_text(content, encoding="utf-8")
    print("wrote", path)
