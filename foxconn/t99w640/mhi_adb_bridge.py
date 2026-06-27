#!/usr/bin/env python3
"""Bidirectional proxy: /dev/mhi_ADB <-> TCP 127.0.0.1:6555. Single-client."""
import argparse, errno, os, selectors, socket, sys, time

CHUNK = 16384
_RETRY = (errno.EAGAIN, errno.EINTR, 512)   # 512 = -ERESTARTSYS from mhi_uci_read

def open_dev(p): return os.open(p, os.O_RDWR | os.O_NONBLOCK)

def drain(fd, window=0.15):
    """Discard CNXN frames queued at open time so adb does not see a reset."""
    end = time.monotonic() + window
    while time.monotonic() < end:
        try:
            buf = os.read(fd, CHUNK)
        except OSError as e:
            if e.errno in _RETRY:
                time.sleep(0.01); continue
            raise
        if not buf: break
        end = time.monotonic() + window

def relay(fd, sock):
    sel = selectors.DefaultSelector()
    sel.register(fd, selectors.EVENT_READ, "dev")
    sel.register(sock.fileno(), selectors.EVENT_READ, "sock")
    while True:
        for key, _ in sel.select():
            if key.data == "dev":
                try: buf = os.read(fd, CHUNK)
                except OSError as e:
                    if e.errno in _RETRY: continue
                    raise
                if not buf: return
                sock.sendall(buf)
            else:
                try: buf = sock.recv(CHUNK)
                except OSError as e:
                    if e.errno in _RETRY: continue
                    raise
                if not buf: return
                os.write(fd, buf)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--device", default="/dev/mhi_ADB")
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=6555)
    a = ap.parse_args()
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((a.host, a.port)); srv.listen(1)
    print(f"listening {a.host}:{a.port} -> {a.device}", file=sys.stderr)
    while True:
        sock, peer = srv.accept()
        fd = open_dev(a.device)
        drain(fd)
        print(f"client {peer}", file=sys.stderr)
        try: relay(fd, sock)
        finally:
            os.close(fd); sock.close()
        print("client gone; waiting for next", file=sys.stderr)

if __name__ == "__main__":
    sys.exit(main())
