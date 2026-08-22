#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Probe /dev/mhi_ADB for existence + adbd responsiveness.

Three checks:
  1. device-node:  does /dev/mhi_ADB exist?
  2. readable:     can we open it O_RDWR non-blocking without ENODEV?
  3. adb-handshake: does the peer respond to an ADB CNXN frame?

Exit codes:
  0  all three checks passed  -> modem serves adbd on channel 36/37
  1  chardev missing          -> driver patch hasn't landed OR firmware
                                 doesn't advertise channel 36/37
  2  chardev present, not open-able
  3  chardev open, no adb response within 2s
                              -> likely fastboot-only channel (FB = fastboot)

Usage:  sudo ./mhi_adb_probe.py [DEVICE]
        default DEVICE = /dev/mhi_ADB
"""
import os
import struct
import sys
import selectors
import time

DEFAULT_DEV = "/dev/mhi_ADB"

# ADB v1 protocol CNXN frame.  A_CNXN = 0x4e584e43
# header layout (little-endian):  u32 command, u32 arg0, u32 arg1,
#                                 u32 data_length, u32 data_crc32, u32 magic
# magic = command XOR 0xFFFFFFFF. Payload is an identity banner.
A_CNXN = 0x4E584E43
ADB_VERSION = 0x01000000
MAX_PAYLOAD = 4096
BANNER = b"host::\0"

def build_cnxn() -> bytes:
    data = BANNER
    data_len = len(data)
    data_crc = sum(data) & 0xFFFFFFFF
    hdr = struct.pack(
        "<IIIIII",
        A_CNXN, ADB_VERSION, MAX_PAYLOAD, data_len, data_crc,
        A_CNXN ^ 0xFFFFFFFF,
    )
    return hdr + data

def main(dev: str) -> int:
    if not os.path.exists(dev):
        print(f"[FAIL] {dev} does not exist", file=sys.stderr)
        return 1

    try:
        fd = os.open(dev, os.O_RDWR | os.O_NONBLOCK)
    except OSError as exc:
        print(f"[FAIL] open({dev}) -> {exc}", file=sys.stderr)
        return 2

    try:
        frame = build_cnxn()
        try:
            os.write(fd, frame)
        except BlockingIOError:
            pass
        sel = selectors.DefaultSelector()
        sel.register(fd, selectors.EVENT_READ)
        deadline = time.monotonic() + 2.0
        got = b""
        while time.monotonic() < deadline:
            remaining = deadline - time.monotonic()
            events = sel.select(timeout=max(remaining, 0))
            if not events:
                continue
            try:
                chunk = os.read(fd, 4096)
            except BlockingIOError:
                continue
            if chunk:
                got += chunk
            if len(got) >= 24:
                break

        if len(got) < 24:
            print(f"[FAIL] no ADB response in 2s (got {len(got)} bytes)",
                  file=sys.stderr)
            return 3

        cmd = struct.unpack("<I", got[:4])[0]
        if cmd == A_CNXN:
            print("[PASS] ADB CNXN response received — this IS adbd")
            print(f"       raw response: {got[:64].hex()}")
            return 0
        print(f"[FAIL] got bytes but first u32 = 0x{cmd:08x}, not A_CNXN",
              file=sys.stderr)
        print(f"       raw response: {got[:64].hex()}", file=sys.stderr)
        return 3
    finally:
        os.close(fd)

if __name__ == "__main__":
    dev = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_DEV
    sys.exit(main(dev))
