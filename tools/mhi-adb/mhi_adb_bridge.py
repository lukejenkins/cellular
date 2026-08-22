#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Bidirectional proxy: /dev/mhi_ADB <-> TCP 127.0.0.1:6555.

Single-client. `adb connect 127.0.0.1:6555` from another terminal.

Ctrl-C to shut down. The daemon closes both endpoints on exit.
Use mhi_adb_probe.py to sanity-check the chardev first.

Modem SSR handling: when the AMSS subsystem restarts mid-relay
(e.g. on AT+USBSWITCH composition changes), the kernel's MHI channel
fd surfaces -ERESTARTSYS (errno 512) on read OR write, and may briefly
report ENODEV while the channel re-enumerates. The relay catches both
and reopens the fd after a short backoff rather than crashing. The
ADB client stays connected.
"""
import argparse
import errno
import os
import selectors
import socket
import sys
import time

CHUNK = 16384

# Exit code when the listen port is already bound — almost always a
# hand-started bridge already serving this device. A systemd unit
# (mhi-adb-bridge.service) can map this to SuccessExitStatus=3 so the
# device-activated service neither restart-spins nor fights the live bridge.
EXIT_PORT_IN_USE = 3

# OOT pcie_mhi's mhi_uci_read can return -ERESTARTSYS (errno 512) even on
# an O_NONBLOCK fd when a signal interrupts the wait path. Treat the same
# as EINTR/EAGAIN at the per-op level: ignore and re-poll. Observed on
# kernel 6.12.74.
_RETRY_ERRNOS = (errno.EAGAIN, errno.EINTR, 512)

# SSR-class errnos: the MHI channel went away (subsystem restart). Cannot
# resume by re-polling the same fd — must reopen. errno.EIO covers the
# usual write-after-disconnect path; ENODEV is the post-SSR window before
# the channel re-enumerates.
_SSR_ERRNOS = (errno.EIO, errno.ENODEV)

# Default budget for reopening the chardev after an SSR before giving up.
_DEFAULT_SSR_BUDGET_S = 10.0
_REOPEN_DELAY_S = 1.0


class _SSRError(Exception):
    """Internal signal: relay detected an SSR and needs an fd reopen."""


def serve(dev_path: str, host: str, port: int,
          ssr_budget_s: float = _DEFAULT_SSR_BUDGET_S) -> int:
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        srv.bind((host, port))
    except OSError as e:
        # An active listener on this port (SO_REUSEADDR only forgives TIME_WAIT,
        # not a live LISTEN) means another bridge is already serving the device.
        # Exit cleanly rather than crash-loop so we don't fight it.
        if e.errno == errno.EADDRINUSE:
            print(f"{host}:{port} already in use — another bridge is likely "
                  f"running; exiting (no-op).", file=sys.stderr)
            srv.close()
            return EXIT_PORT_IN_USE
        srv.close()
        raise
    srv.listen(1)
    print(f"listening {host}:{port}, forwarding to {dev_path}",
          file=sys.stderr)
    try:
        while True:
            sock, peer = srv.accept()
            fd = open_chardev(dev_path)
            drained = drain(fd)
            print(f"client: {peer} (fd={fd})  drained={drained}b",
                  file=sys.stderr)
            sock.setblocking(False)
            try:
                relay(fd, sock, dev_path, ssr_budget_s)
            finally:
                _safe_close_fd(fd)
                sock.close()
            print("client disconnected, accepting next", file=sys.stderr)
    finally:
        srv.close()


def open_chardev(dev_path: str) -> int:
    """Open the MHI ADB chardev with the same flags used per-client."""
    return os.open(dev_path, os.O_RDWR | os.O_NONBLOCK)


def reopen_after_ssr(dev_path: str, budget_s: float) -> int | None:
    """Reopen the chardev after an SSR, retrying until budget exhausted.

    Returns the new fd on success or None if the device never came
    back within budget_s (typically because the modem failed to
    finish its subsystem restart). Caller should exit cleanly so a
    watchdog can restart the daemon.
    """
    deadline = time.monotonic() + budget_s
    last_err: Exception | None = None
    while time.monotonic() < deadline:
        time.sleep(_REOPEN_DELAY_S)
        try:
            fd = open_chardev(dev_path)
        except OSError as e:
            last_err = e
            if e.errno in (errno.ENOENT, errno.ENODEV, errno.EBUSY):
                continue
            raise
        drained = drain(fd)
        print(f"SSR recovery: reopened {dev_path} fd={fd} drained={drained}b",
              file=sys.stderr)
        return fd
    print(f"SSR recovery: budget {budget_s}s exhausted; giving up (last_err={last_err!r})",
          file=sys.stderr)
    return None


def _safe_close_fd(fd: int) -> None:
    try:
        os.close(fd)
    except OSError:
        pass


def drain(fd: int, window_s: float = 0.15) -> int:
    """Discard any data the chardev has already queued at open time.
    On RM520N-GL-AP, the OOT pcie_mhi ADB channel queues a CNXN frame
    every time the channel is opened. After a fresh modem boot the FIRST
    open also still carries a stale CNXN from the previous session, so
    the client sees TWO CNXNs back-to-back. adb interprets the duplicate
    as a reset and parks the device as 'offline'. Drain solves it."""
    end = time.monotonic() + window_s
    total = 0
    while time.monotonic() < end:
        try:
            buf = os.read(fd, CHUNK)
        except OSError as e:
            if e.errno in _RETRY_ERRNOS:
                time.sleep(0.01)
                continue
            if e.errno in _SSR_ERRNOS:
                # SSR mid-drain is rare but possible; surface to caller
                # rather than retrying inside drain.
                raise
            raise
        if not buf:
            break
        total += len(buf)
        end = time.monotonic() + window_s  # extend window if data flowing
    return total


def relay(fd: int, sock: socket.socket, dev_path: str,
          ssr_budget_s: float = _DEFAULT_SSR_BUDGET_S) -> None:
    """Bidirectional copy between MHI chardev and ADB client socket.

    Survives modem SSR events: if the chardev fd surfaces an SSR-class
    errno on read or write, reopen the chardev (with backoff up to
    ssr_budget_s seconds) and rebuild the selector. Per-op transient
    errnos (EAGAIN / EINTR / 512) re-poll without reopening.
    """
    while True:
        sel = selectors.DefaultSelector()
        sel.register(fd, selectors.EVENT_READ, data="chardev")
        sel.register(sock.fileno(), selectors.EVENT_READ, data="sock")
        try:
            _relay_loop(fd, sock, sel)
            return
        except _SSRError:
            pass
        finally:
            sel.close()
        _safe_close_fd(fd)
        new_fd = reopen_after_ssr(dev_path, ssr_budget_s)
        if new_fd is None:
            return
        fd = new_fd


def _relay_loop(fd: int, sock: socket.socket,
                sel: selectors.BaseSelector) -> None:
    while True:
        for key, _ in sel.select():
            if key.data == "chardev":
                try:
                    buf = os.read(fd, CHUNK)
                except OSError as e:
                    if e.errno in _RETRY_ERRNOS:
                        continue
                    if e.errno in _SSR_ERRNOS:
                        raise _SSRError() from e
                    raise
                if not buf:
                    return
                try:
                    sock.sendall(buf)
                except OSError as e:
                    if e.errno in _RETRY_ERRNOS:
                        continue
                    raise
            else:
                try:
                    buf = sock.recv(CHUNK)
                except OSError as e:
                    if e.errno in _RETRY_ERRNOS:
                        continue
                    raise
                if not buf:
                    return
                try:
                    os.write(fd, buf)
                except OSError as e:
                    if e.errno in _RETRY_ERRNOS:
                        continue
                    if e.errno in _SSR_ERRNOS:
                        raise _SSRError() from e
                    raise


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--device", default="/dev/mhi_ADB")
    p.add_argument("--host", default="127.0.0.1")
    # 6555 (not 5555) to dodge adb server's emulator auto-discovery
    # range (5554-5682). On 5555 adb registers the bridge as
    # "emulator-5554" and marks an explicit `adb connect` entry offline.
    p.add_argument("--port", type=int, default=6555)
    p.add_argument("--ssr-budget", type=float, default=_DEFAULT_SSR_BUDGET_S,
                   help="seconds to wait for chardev reopen after SSR")
    args = p.parse_args()
    return serve(args.device, args.host, args.port, args.ssr_budget) or 0


if __name__ == "__main__":
    sys.exit(main())
