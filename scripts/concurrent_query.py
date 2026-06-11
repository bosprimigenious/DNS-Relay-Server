#!/usr/bin/env python3
"""Run concurrent DNS queries against the relay server."""

import argparse
import concurrent.futures
import socket
import struct
import time


def encode_name(name: str) -> bytes:
    out = bytearray()
    for label in name.strip(".").split("."):
        data = label.encode("ascii")
        if len(data) == 0 or len(data) > 63:
            raise ValueError("invalid label")
        out.append(len(data))
        out.extend(data)
    out.append(0)
    return bytes(out)


def build_query(query_id: int, qname: str) -> bytes:
    header = struct.pack("!HHHHHH", query_id, 0x0100, 1, 0, 0, 0)
    return header + encode_name(qname) + struct.pack("!HH", 1, 1)


def worker(server: str, port: int, qname: str, index: int, timeout: float) -> tuple[bool, str]:
    query_id = (index + 1) & 0xFFFF
    packet = build_query(query_id, qname)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(timeout)

    try:
        sock.sendto(packet, (server, port))
        data, _ = sock.recvfrom(512)
    except OSError as exc:
        sock.close()
        return False, str(exc)

    sock.close()
    if len(data) < 12:
        return False, "short-response"

    response_id, flags, _, _, _, _ = struct.unpack("!HHHHHH", data[:12])
    rcode = flags & 0xF
    if response_id != query_id:
        return False, f"id-mismatch:{response_id}!={query_id}"
    if rcode != 0:
        return False, f"rcode={rcode}"
    return True, "ok"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("server", nargs="?", default="127.0.0.1")
    parser.add_argument("port", nargs="?", type=int, default=5353)
    parser.add_argument("domain", nargs="?", default="baidu.com")
    parser.add_argument("--requests", type=int, default=100)
    parser.add_argument("--workers", type=int, default=50)
    parser.add_argument("--timeout", type=float, default=3.0)
    args = parser.parse_args()

    started = time.perf_counter()
    successes = 0
    failures: dict[str, int] = {}

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
        futures = [
            pool.submit(worker, args.server, args.port, args.domain, i, args.timeout)
            for i in range(args.requests)
        ]
        for future in concurrent.futures.as_completed(futures):
            ok, detail = future.result()
            if ok:
                successes += 1
            else:
                failures[detail] = failures.get(detail, 0) + 1

    elapsed_ms = (time.perf_counter() - started) * 1000.0
    print(
        "domain={domain} requests={requests} workers={workers} success={success} "
        "failed={failed} total_ms={elapsed:.2f}".format(
            domain=args.domain,
            requests=args.requests,
            workers=args.workers,
            success=successes,
            failed=args.requests - successes,
            elapsed=elapsed_ms,
        )
    )
    if failures:
        for detail, count in sorted(failures.items()):
            print(f"failure={detail} count={count}")

    return 0 if successes == args.requests else 1


if __name__ == "__main__":
    raise SystemExit(main())
