#!/usr/bin/env python3
"""兼容入口 → platform/common/python/dig_win.py"""
import subprocess
import sys
from pathlib import Path

target = Path(__file__).resolve().parent.parent / "platform/common/python/dig_win.py"
raise SystemExit(subprocess.call([sys.executable, str(target), *sys.argv[1:]]))
