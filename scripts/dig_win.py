#!/usr/bin/env python3
"""dig 风格 A 记录查询（Windows 无 dig 时用于验收截图）。"""
import socket
import struct
import sys

RCODE = {0: "NOERROR", 1: "FORMERR", 2: "SERVFAIL", 3: "NXDOMAIN"}


def encode_name(name: str) -> bytes:
    out = bytearray()
    for label in name.strip(".").split("."):
        b = label.encode("ascii")
        out.append(len(b))
        out.extend(b)
    out.append(0)
    return bytes(out)


def skip_name(data: bytes, off: int) -> int:
    while off < len(data):
        ln = data[off]
        if ln == 0:
            return off + 1
        if ln & 0xC0 == 0xC0:
            return off + 2
        off += 1 + ln
    raise ValueError("bad name")


def parse_a_records(data: bytes) -> list[tuple[str, int, str]]:
    if len(data) < 12:
        return []
    _, flags, qd, an, _, _ = struct.unpack("!HHHHHH", data[:12])
    off = 12
    for _ in range(qd):
        off = skip_name(data, off)
        off += 4
    rows = []
    for _ in range(an):
        name_start = off
        off = skip_name(data, off)
        if off + 10 > len(data):
            break
        rtype, rclass, ttl, rdlen = struct.unpack("!HHIH", data[off : off + 10])
        off += 10
        rdata = data[off : off + rdlen]
        off += rdlen
        if rtype == 1 and rdlen == 4:
            ip = ".".join(str(b) for b in rdata)
            rows.append((name_start, ttl, ip))
    return rows, flags & 0xF


def main() -> None:
    if len(sys.argv) < 4:
        print("usage: dig_win.py <server> <port> <qname>", file=sys.stderr)
        sys.exit(1)
    host, port_s, qname = sys.argv[1], sys.argv[2], sys.argv[3]
    port = int(port_s)
    pkt = struct.pack("!HHHHHH", 0xABCD, 0x0100, 1, 0, 0, 0)
    pkt += encode_name(qname) + struct.pack("!HH", 1, 1)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)
    sock.sendto(pkt, (host, port))
    data, _ = sock.recvfrom(512)
    rows, rcode = parse_a_records(data)
    print(f"$ dig @{host} -p {port} {qname} A +noall +answer +comments")
    print(f";; ->>HEADER<<- status: {RCODE.get(rcode, rcode)}")
    if not rows:
        print(";; ANSWER SECTION: (empty)")
        return
    print(";; ANSWER SECTION:")
    for _, ttl, ip in rows:
        print(f"{qname}.    {ttl}    IN    A    {ip}")


if __name__ == "__main__":
    main()
