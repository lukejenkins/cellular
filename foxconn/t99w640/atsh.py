#!/usr/bin/env python3
"""Interactive AT shell for /dev/wwan0at0 on the Foxconn T99W640 (Dell DW5934e).

Why this exists: /dev/wwan0at0 is a kernel WWAN char device (mhi_wwan_ctrl),
not a TTY, so termios-based tools (tio, picocom, minicom, pyserial) fail.
Use raw os.open + select and let the wwan core handle framing.

Note on state-gating: on this firmware the host-side MHI DUN channel
(/dev/wwan0at0) can stay silent even when the modem's AT processor is
responsive. If this script connects cleanly but every command times out,
the issue is transport/driver-side, not framing or termios. In that case
talk to the modem-side AT device directly (e.g. /dev/at_mdm0) from an
on-modem shell instead.

Usage:  sudo python3 atsh.py [DEVICE]
        DEVICE defaults to /dev/wwan0at0; pass another path to override.
        Ctrl-] to quit.
"""
import os
import select
import sys
import termios
import tty

# Default AT char device. Override by passing a path as argv[1]
# (e.g. /dev/wwan0at0, /dev/mhi_DUN, or a modem-side /dev/at_mdm0).
DEV = sys.argv[1] if len(sys.argv) > 1 else "/dev/wwan0at0"
fd = os.open(DEV, os.O_RDWR | os.O_NONBLOCK)

stdin_attrs = termios.tcgetattr(0)
tty.setcbreak(0)
try:
    print(f"[atsh] connected to {DEV} — Ctrl-] to quit", file=sys.stderr)
    line = b""
    while True:
        r, _, _ = select.select([0, fd], [], [], 0.1)
        if 0 in r:
            ch = os.read(0, 1)
            if ch == b"\x1d":
                break
            if ch in (b"\r", b"\n"):
                os.write(fd, line + b"\r")
                sys.stdout.write("\n")
                sys.stdout.flush()
                line = b""
            elif ch == b"\x7f":
                line = line[:-1]
                sys.stdout.write("\b \b")
                sys.stdout.flush()
            else:
                line += ch
                sys.stdout.write(ch.decode("latin-1", "replace"))
                sys.stdout.flush()
        if fd in r:
            try:
                buf = os.read(fd, 4096)
                if buf:
                    sys.stdout.write(buf.decode("latin-1", "replace"))
                    sys.stdout.flush()
            except BlockingIOError:
                pass
finally:
    termios.tcsetattr(0, termios.TCSADRAIN, stdin_attrs)
    os.close(fd)
