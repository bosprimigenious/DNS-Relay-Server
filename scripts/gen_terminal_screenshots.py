#!/usr/bin/env python3
"""Render per-step verification logs as terminal PNGs."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VER = ROOT / "docs" / "verification"
OUT = ROOT / "docs" / "screenshots"

SECTION_MAP = {
    "1": "01-build",
    "2": "02-startup",
    "3": "03-nslookup-bupt",
    "4": "04-nslookup-008",
    "5": "05-nslookup-baidu",
    "6": "06-nslookup-test0",
    "7": "07-nslookup-test1",
    "8": "08-nslookup-sina",
    "9": "09-nslookup-mx",
    "10": "10-dns-query",
    "11": "11-dig-bupt",
    "12": "12-dig-block",
    "13": "13-dig-relay",
    "14": "14-fix-b",
}

DNSPERF_MAP = {
    "15-dnsperf-light": "12-dnsperf-light-summary.log",
    "16-dnsperf-stress": "12-dnsperf-stress-summary.log",
}

ALLOWED_STEMS = set(SECTION_MAP.values()) | set(DNSPERF_MAP.keys())

HEADER_RE = re.compile(r"Screenshot (\d+):\s*([^\n=]+)")
# PIL/Cascadia Mono 无法绘制 TAB(U+0009)，会显示为方框乱码
CONTROL_RE = re.compile(r"[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]")
TAB_RE = re.compile(r"\t+")

MONO_FONT_CANDIDATES = [
    "C:/Windows/Fonts/CascadiaMono.ttf",
    "C:/Windows/Fonts/CascadiaCode.ttf",
    "C:/Windows/Fonts/consola.ttf",
    "C:/Windows/Fonts/cour.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
]

# dig ANSWER 行：name TTL CLASS TYPE RDATA（TAB 对齐）
DNS_RR_RE = re.compile(r"^(\S+)\s+(\d+)\s+(\S+)\s+(\S+)\s+(.+)$")
# nslookup 字段行（含 Server，TAB 在冒号后常导致 PIL 方框乱码）
NSLOOKUP_FIELD_RE = re.compile(
    r"^(Server|Name|Address|Aliases):\s*(.*)$",
    re.IGNORECASE,
)
# 渲染前仅保留可打印 ASCII，彻底避免字体缺字/控制符
ASCII_ONLY_RE = re.compile(r"[^\x20-\x7e]")


def sanitize_text(text: str) -> str:
    """Expand tabs and strip control chars in whole log blobs."""
    text = text.replace("\r", "")
    text = TAB_RE.sub(lambda m: "    " * len(m.group(0)), text)
    text = CONTROL_RE.sub("", text)
    return text.replace("\ufffd", " ")


def normalize_line(line: str) -> str:
    """Expand tabs and strip control chars that fonts cannot render."""
    line = sanitize_text(line)
    return line.expandtabs(8)


def polish_line(line: str) -> str:
    """Normalize whitespace and reformat DNS/nslookup lines for PNG rendering."""
    line = normalize_line(line)
    stripped = line.strip()
    m = DNS_RR_RE.match(stripped)
    if m:
        name, ttl, klass, rtype, rdata = m.groups()
        return f"{name:<22} {ttl:>6}  {klass:<4} {rtype:<4} {rdata}"
    m = NSLOOKUP_FIELD_RE.match(stripped)
    if m:
        return f"{m.group(1)}: {m.group(2).strip()}"
    return line


def render_safe(line: str) -> str:
    return ASCII_ONLY_RE.sub(" ", polish_line(line))


def normalize_lines(text: str) -> list[str]:
    return [polish_line(line) for line in text.splitlines()]


def parse_sections(full_text: str) -> dict[str, str]:
    sections: dict[str, str] = {}
    matches = list(HEADER_RE.finditer(full_text))
    for i, m in enumerate(matches):
        key = SECTION_MAP.get(m.group(1))
        if not key:
            continue
        body_start = m.end()
        body_end = matches[i + 1].start() if i + 1 < len(matches) else len(full_text)
        body = re.sub(r"^=+\s*$", "", full_text[body_start:body_end], flags=re.MULTILINE).strip()
        sections[key] = body
    return sections


def read_log(path: Path) -> str:
    if not path.exists():
        return ""
    return sanitize_text(path.read_text(encoding="utf-8", errors="replace"))


def startup_section() -> str:
    stderr = read_log(VER / "02-server-startup.log").strip()
    stdout = read_log(VER / "02-server-stdout.log").strip()
    bind = "127.0.0.1"
    port = "15353"
    return (
        "Screenshot 2: server startup (stderr + stdout)\n"
        f"$ DNS_RELAY_BIND={bind} DNS_RELAY_PORT={port} ./dnsrelay\n"
        "--- stderr (config load) ---\n"
        f"{stderr}\n"
        "--- stdout (listen) ---\n"
        f"{stdout}"
    )


def load_sections() -> dict[str, str]:
    build_raw = read_log(VER / "01-build.log")
    full = read_log(VER / "03-full-verification.log")
    sections = parse_sections(build_raw)
    sections.update(parse_sections(full))
    m = HEADER_RE.search(build_raw)
    if m:
        sections["01-build"] = build_raw[m.end():].strip()
    sections["02-startup"] = startup_section()
    return {k: sections.get(k, "") for k in SECTION_MAP.values()}


def trim_dnsperf_noise(lines: list[str], max_timeouts: int = 3) -> list[str]:
    out: list[str] = []
    timeout_count = 0
    for line in lines:
        if "[Timeout]" in line or "unexpected (maybe timed out)" in line:
            if timeout_count < max_timeouts:
                out.append(line)
                timeout_count += 1
            elif timeout_count == max_timeouts:
                out.append("... (timeout lines omitted) ...")
                timeout_count += 1
            continue
        out.append(line)
    return out


def load_mono_font(size: int):
    from PIL import ImageFont

    for fp in MONO_FONT_CANDIDATES:
        if Path(fp).exists():
            return ImageFont.truetype(fp, size)
    return ImageFont.load_default()


def render_png(text: str, path: Path, width: int = 1280) -> None:
    from PIL import Image, ImageDraw

    lines = normalize_lines(text)
    if "Statistics:" in text:
        lines = trim_dnsperf_noise(lines)

    if not lines:
        raise ValueError(f"empty content for {path}")

    font_size = 13
    font = load_mono_font(font_size)
    lh = font_size + 4
    pad = 14

    max_px = 0
    safe_lines = [render_safe(line) for line in lines]
    for line in safe_lines:
        bbox = font.getbbox(line[:240])
        max_px = max(max_px, bbox[2] - bbox[0])

    # 最小宽度拉满，避免 PDF 里终端图过窄、与侧栏文字并排
    img_w = max(max_px + pad * 2, min(width, 1100))
    img_h = pad * 2 + len(safe_lines) * lh
    img = Image.new("RGB", (img_w, img_h), "#0c0c0c")
    draw = ImageDraw.Draw(img)
    y = pad
    for line in safe_lines:
        color = "#cccccc"
        if line.startswith("$"):
            color = "#4ec9b0"
        elif any(x in line for x in ("NXDOMAIN", "SERVFAIL", "error", "can't find")):
            color = "#f44747"
        elif "NOERROR" in line or "rcode=0" in line:
            color = "#4ec9b0"
        elif line.startswith(("gcc", "make", "rm ")):
            color = "#dcdcaa"
        elif line.startswith((";;", "; ")):
            color = "#9cdcfe"
        elif line.startswith("qname="):
            color = "#dcdcaa"
        elif "Statistics:" in line or "Queries per second" in line:
            color = "#dcdcaa"
        elif "Queries lost" in line or "lost:" in line:
            color = "#f44747"
        elif "Queries completed" in line:
            color = "#4ec9b0"
        draw.text((pad, y), line[:240], fill=color, font=font, spacing=0)
        y += lh
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path, "PNG")


def scrub_verification_logs() -> None:
    """Persist tab-free logs so re-runs and manual inspection stay clean."""
    for path in VER.glob("*.log"):
        raw = path.read_bytes()
        text = raw.decode("utf-8", errors="replace")
        clean = sanitize_text(text)
        if clean != text.replace("\r", ""):
            path.write_text(clean, encoding="utf-8", newline="\n")
            print(f"scrubbed tabs: {path.name}")


def main() -> None:
    try:
        from PIL import Image  # noqa: F401
    except ImportError:
        print("pip install pillow")
        raise SystemExit(1)

    scrub_verification_logs()
    sections = load_sections()
    for key, log_name in DNSPERF_MAP.items():
        log_path = VER / log_name
        if log_path.exists():
            sections[key] = read_log(log_path).strip()

    OUT.mkdir(parents=True, exist_ok=True)
    for key, content in sections.items():
        if not content.strip():
            print(f"skip empty: {key}")
            continue
        out = OUT / f"terminal-{key}.png"
        render_png(content, out)
        print(f"written {out} ({len(content.splitlines())} lines)")

    for old in OUT.glob("terminal-*.png"):
        stem = old.stem.replace("terminal-", "")
        if stem not in ALLOWED_STEMS:
            old.unlink()
            print(f"removed legacy {old}")


if __name__ == "__main__":
    main()
