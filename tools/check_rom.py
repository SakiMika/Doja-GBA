#!/usr/bin/env python3
"""Minimal build-artifact check for the Corpse Party GBA ROM (not an emulator test)."""

import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: python3 tools/check_rom.py <rom.gba>", file=sys.stderr)
        return 2

    path = Path(sys.argv[1])
    if not path.is_file():
        print(f"ROM not found: {path}", file=sys.stderr)
        return 1

    rom = path.read_bytes()
    if not 0xC0 <= len(rom) <= 32 * 1024 * 1024:
        print(f"Invalid ROM size: {len(rom):,} bytes", file=sys.stderr)
        return 1

    title = rom[0xA0:0xAC].rstrip(b"\x00 ")
    game_code = rom[0xAC:0xB0]
    checksum = (-0x19 - sum(rom[0xA0:0xBD])) & 0xFF
    if title != b"CORPSE PARTY" or game_code != b"BCP1":
        print(f"Unexpected GBA header: title={title!r}, code={game_code!r}", file=sys.stderr)
        return 1
    if rom[0xBD] != checksum:
        print("Invalid GBA header checksum", file=sys.stderr)
        return 1

    print(f"ROM header OK: {path} ({len(rom):,} bytes), title=CORPSE PARTY, code=BCP1")
    return 0


if __name__ == "__main__":
    sys.exit(main())
