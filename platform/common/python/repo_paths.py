"""Locate DNS-Relay-Server repository root from any script under platform/."""

from __future__ import annotations

from pathlib import Path


def repo_root(start: Path | None = None) -> Path:
    current = (start or Path(__file__)).resolve()
    for candidate in [current, *current.parents]:
        if (candidate / "Makefile").is_file() and (candidate / "src").is_dir():
            return candidate
    raise RuntimeError("DNS-Relay-Server repo root not found (expected Makefile + src/)")
