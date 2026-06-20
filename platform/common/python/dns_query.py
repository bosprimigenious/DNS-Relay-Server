#!/usr/bin/env python3
"""向 DNS-Relay 发送最小 A 记录查询并打印响应摘要。"""
import socket
import struct
import sys


def encode_name(name: str) -> bytes:
    out = bytearray()
    for label in name.strip(".").split("."):
        b = label.encode("ascii")
        if len(b) == 0 or len(b) > 63:
            raise ValueError("invalid label")
        out.append(len(b))
        out.extend(b)
    out.append(0)
    return bytes(out)


def build_query(qname: str, qtype: int = 1) -> bytes:
    hdr = struct.pack("!HHHHHH", 0x1234, 0x0100, 1, 0, 0, 0)
    return hdr + encode_name(qname) + struct.pack("!HH", qtype, 1)


def query(server: str, qname: str, qtype: int = 1, port: int = 53) -> None:
    pkt = build_query(qname, qtype)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(5.0)
    s.sendto(pkt, (server, port))
    data, _ = s.recvfrom(512)
    if len(data) < 12:
        print("short response", len(data))
        return
    rid, flags, qd, an, ns, ar = struct.unpack("!HHHHHH", data[:12])
    rcode = flags & 0xF
    print(f"qname={qname} id=0x{rid:04x} rcode={rcode} ancount={an} len={len(data)}")


if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
    port = 53
    names = sys.argv[2:]
    if names and names[0].isdigit():
        port = int(names.pop(0))
    if not names:
        names = ["bupt", "008.cn", "baidu.com"]
    for name in names:
        try:
            query(host, name, port=port)
        except OSError as e:
            print(f"qname={name} error={e}")
