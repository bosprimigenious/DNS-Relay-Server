#!/usr/bin/env python3
import socket

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.settimeout(2.0)
try:
    s.sendto(bytes(12), ("127.0.0.1", 53))
    data, addr = s.recvfrom(512)
    print("recv", len(data), "from", addr)
except OSError as e:
    print("error:", e)
