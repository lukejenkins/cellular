# FINDINGS — OOT `pcie_mhi` ADB channel patch

**Verdict: POSITIVE.** The two vendored patches, applied to
`Quectel_Linux_PCIE_MHI_Driver_V1.3.6` and built as an out-of-tree
module, expose MHI channels 36/37 as `/dev/mhi_ADB` on RM520N-GL-AP
firmware when `AT+QPCIE="adb",1` is set. The channel carries a real
ADB v1 protocol, and the modem-side `adbd` hands out an unauthenticated
**root shell** over it.

## Confirmed on

- **Module:** RM520N-GL-AP, unit #2
- **Host:** Arch Linux, kernel `6.19.12-arch1-1`
- **Patched `.ko` SHA-256:** `0201e183a2c4c0ae12e02d850be19f4bec04c4a83dcdb803ed2fd3951de9b09b`
- **Date:** 2026-04-17

The same patches expose `/dev/mhi_ADB` on the Foxconn T99W640 / Dell
DW5934e (SDX72), which serves an unauthenticated root shell the same way
— the ADB channel there also comes from the *patched* driver, not stock.

## Evidence

### Chardev appears

```
# ls /dev/mhi_*
/dev/mhi_ADB  /dev/mhi_BHI  /dev/mhi_DIAG  /dev/mhi_DUN
/dev/mhi_LOOPBACK  /dev/mhi_QMI0
```

All six UCI chardevs enumerate after the patched driver reaches
MISSION MODE. `mhi_ADB` only appears because the patches land; the
pre-patch baseline produces five chardevs (no `mhi_ADB`).

### Probe succeeds

```
# sudo ./mhi_adb_probe.py
[PASS] ADB CNXN response received — this IS adbd
       raw response: 434e584e00000001001000003f00000028170000bcb1a7b1
```

Exit `0`. The response header decodes as a valid ADB v1 `CNXN` frame:
version `01000000`, max payload `0x1000`, banner length `0x3f`, valid
checksum and magic (`bcb1a7b1 = CNXN ^ 0xFFFFFFFF`).

### Bridge + `adb` client → root shell

```
# sudo ./mhi_adb_bridge.py   # defaults to TCP 127.0.0.1:6555
listening 127.0.0.1:6555, forwarding to /dev/mhi_ADB

$ adb connect 127.0.0.1:6555
connected to 127.0.0.1:6555
$ adb devices
127.0.0.1:6555  device
$ adb -s 127.0.0.1:6555 shell id
uid=0(root) gid=0(root) groups=1004,1007,1011,1015(sdcard),1028,3001,3002,3003(inet),3006
$ adb -s 127.0.0.1:6555 shell uname -a
Linux sdxlemur 5.4.180-perf #1 PREEMPT Fri May 26 09:26:39 UTC 2023 armv7l GNU/Linux
$ adb -s 127.0.0.1:6555 shell cat /etc/os-release
ID=qti-distro-nogplv3-perf
NAME="QTI Linux reference nogplv3 distro targeting performance builds."
VERSION="LE.UM.5.3.2.r1-06300-SDX65.0"
```

No authentication challenge. No `unauthorized` state. Root from the
first connect.

## Modem userspace summary

| Field | Value |
|---|---|
| Hostname | `sdxlemur` (SDX65-variant codename) |
| Kernel | Linux 5.4.180-perf, armv7l, built 2023-05-26 |
| Distro | `qti-distro-nogplv3-perf` |
| Distro version | `LE.UM.5.3.2.r1-06300-SDX65.0` |
| Top-level mounts of interest | `/firmware`, `/oemapp`, `/oemdata`, `/persist`, `/systemrw`, `/target` |
| `adbd` build | Android adbd ported to Linux — keeps Android UID/GID model (1015/sdcard, 3003/inet, etc.) but no `getprop`; `/etc/os-release` instead |
| Device banner | `device::ro.product.name=;ro.product.model=;ro.product.device=;` (product fields empty — not a stock Android image) |

## Why channels 36/37 work

Patch `0001` declares the channel in the OOT driver's `chan_cfg[]`
with name `"ADB"` (the channel numbers 36/37 are already reserved
symbolically in the header as `MHI_CLIENT_ADB_FB_OUT/IN` but the
vendor tree left them out of the data table).

Patch `0002` adds `{ .chan = "ADB", .driver_data = 0x4000 }` to
`mhi_uci_match_table[]` in `devices/mhi_uci.c`, which causes the UCI
layer to bind a character device to the channel at probe time. Both
patches are required — `0001` alone gets the channel declared but no
chardev; `0002` alone has no channel to match against.

The firmware side does not need any change beyond the AT-level toggle
`AT+QPCIE="adb",1` (persistent in NV), which tells the modem's
`adbd` init script to start. Without that, the patched driver still
creates `/dev/mhi_ADB`, but reads and writes go nowhere because adbd
isn't listening on the MHI endpoint.

## Operational notes

- **Cold-load only.** `rmmod pcie_mhi && insmod pcie_mhi.ko` while the
  modem is in M0/AMSS leaves the driver hung at "Waiting to enter READY
  state" — the host-issued `MHI_RESET` is a soft doorbell that the
  running firmware ignores. Reproduced twice. Treat the patched `.ko`
  as a boot-time-only module; reboot the host between load cycles, or
  reboot the modem (`AT+CFUN=1,1`) *before* `rmmod`.
- **Bridge port = 6555, not 5555.** `adb server` auto-enumerates ports
  5554–5682 at startup looking for Android emulators; anything on 5555
  that replies to `CNXN` gets registered as `emulator-5554`, which
  blocks an explicit `adb connect 127.0.0.1:5555` from going `online`.
  The bridge default avoids that range.
- **Per-client chardev reopen.** The bridge opens `/dev/mhi_ADB`
  fresh on each TCP client so adbd sees a clean MHI channel session
  (open/close map to `__mhi_prepare_channel` start/stop in the OOT
  driver). Steady-state throughput during an active `adb shell` is
  unaffected.

## Possible future enhancements

The bridge is architecturally required — the stock `adb` client speaks
only USB / TCP / localhost socket, never a chardev — but the three
manual steps (start bridge, `adb connect`, pass `-s`) can be collapsed
to zero with a little plumbing. None of this is needed for the result
to work; it's quality-of-life.

1. **systemd unit + udev rule — auto-start the bridge.** Install a
   `mhi-adb-bridge.service` and a udev rule like
   `ACTION=="add", KERNEL=="mhi_ADB", TAG+="systemd",
   ENV{SYSTEMD_WANTS}+="mhi-adb-bridge.service"`. Pair with
   `BindsTo=dev-mhi_ADB.device` in the unit file so systemd tears the
   bridge down if the driver unloads (no stale fd). Net: bridge is
   running whenever `/dev/mhi_ADB` exists, and stopped otherwise.

2. **Auto-connect on login.** Add `adb connect 127.0.0.1:6555
   >/dev/null 2>&1` to a shell profile or a systemd user unit.
   `adb server` caches the registration across adb commands, so this
   only needs to happen once per adb-server lifetime.

3. **Default target via `ANDROID_SERIAL`.** `export
   ANDROID_SERIAL=127.0.0.1:6555` in `~/.bashrc` (or equivalent).
   Every adb-based tool that honors the env — Android Studio, `adb`,
   `scrcpy`, `idb`, vendor flashers — will pick the bridge as the
   default device with no `-s` flag.

4. **Proper packaging of the patched OOT driver.** Rather than a
   one-off `make` that needs a manual rebuild on kernel upgrade, a
   DKMS module (or a distro package) auto-rebuilds on each kernel bump
   and lets the driver be managed like any other kernel module. Bonus:
   makes the patched driver portable to other hosts without
   hand-carrying the source tree.

5. **Upstream the two patches to Quectel.** Both are minimal (one
   line added to `chan_cfg[]`, one line added to
   `mhi_uci_match_table[]`). The channel constants
   `MHI_CLIENT_ADB_FB_OUT/IN` (36/37) are already declared in the
   header — the data-table omission looks like an oversight, not a
   deliberate lockout. Worth a support ticket.

6. **Controlled A/B baseline with ADB disabled.**
   For a clean write-up, run the probe on a cold boot with
   `AT+QPCIE="adb",0 && AT+CFUN=1,1` and the patched driver: expected
   behavior is `/dev/mhi_ADB` present but probe exit=3 (chardev opens,
   no `CNXN` response) — isolating "chardev existence" (driver patch)
   from "adb protocol presence" (firmware toggle).

7. **Fastboot side of `ADB_FB`.** The channel name
   `MHI_CLIENT_ADB_FB_OUT/IN` hints that the same channel pair is
   reused for fastboot, probably selected by firmware boot mode. If
   ever needed for reflash-over-PCIe, this would be a follow-up
   patch — likely a second UCI match entry and an AT / boot-mode
   toggle.

8. **Survey the `sdxlemur` modemroot.** `/persist`, `/systemrw`,
   `/oemdata`, `/oemapp`, `/firmware`, `/target`, `/build.prop` — all
   reachable via `adb pull` now. Worth a dedicated session to catalog
   NV, calibration, certs, OEM customizations, and any credentials or
   keys in the clear.

## Security / threat-model implications

This is a high-value result: the patched OOT driver plus the persistent
`AT+QPCIE="adb",1` NV bit give a local PCIe host `root` on the
modem's Linux userspace with no ADB authentication. Anyone with the
following can replicate it end-to-end on this firmware:

1. Physical PCIe access to the modem.
2. Ability to load an OOT kernel module on the host (i.e., root on the
   host, or a distro that ships the OOT driver packaged).
3. Knowledge of the AT toggle, which is plain-text documented in the
   Quectel AT manual.

There is no bootloader unlock, no SPC, no signed-image requirement,
and no adb key pairing in the path. That extends the root-shell
surface from USB (when the RNDIS composition is available) to
PCIe-only deployments — which is the RM520N-GL-AP's primary deployment
mode.
