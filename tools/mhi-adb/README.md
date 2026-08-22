# `mhi-adb` — probe & bridge for ADB-over-MHI

Two small, dependency-free Python tools for using the ADB channel that
the [`pcie-mhi` patches](/patches/linux/pcie-mhi/) expose as
`/dev/mhi_ADB` on a PCIe/MHI Qualcomm modem (e.g. Quectel RM520N-GL-AP,
Foxconn T99W640 / Dell DW5934e).

Both are stdlib-only Python 3. Licensed **Apache-2.0** (see the SPDX
header in each file).

## Workflow: apply → verify → use

1. **Apply** the driver patches and load the patched `pcie_mhi.ko` so
   `/dev/mhi_ADB` appears — see [`/patches/linux/pcie-mhi/`](/patches/linux/pcie-mhi/).
2. **Verify** with the probe:

   ```console
   $ sudo ./mhi_adb_probe.py
   [PASS] ADB CNXN response received — this IS adbd
   ```

   Exit codes make the failure mode unambiguous:

   | Exit | Meaning |
   |---|---|
   | 0 | Node exists, opens, and the peer answers an ADB `CNXN` — the modem is serving adbd on channel 36/37 |
   | 1 | Chardev missing → the patch didn't land, or the firmware doesn't advertise ch 36/37 |
   | 2 | Chardev present but won't open |
   | 3 | Opens but no ADB reply in 2s → likely the *fastboot* half of the channel, not adbd |

3. **Use** the bridge — `adb` speaks TCP/USB/localhost-socket, never a
   chardev, so a relay is architecturally required:

   ```console
   $ sudo ./mhi_adb_bridge.py            # listens on 127.0.0.1:6555
   $ adb connect 127.0.0.1:6555
   $ adb -s 127.0.0.1:6555 shell id
   uid=0(root) ...
   ```

## Notes

- **Port 6555, not 5555.** `adb server` auto-enumerates 5554–5682 for
  Android emulators; anything on 5555 answering `CNXN` gets claimed as
  `emulator-5554`, which blocks an explicit `adb connect`. The bridge
  default avoids that range (override with `--port`).
- **Drain-on-open.** The OOT ADB channel queues a `CNXN` frame on every
  open; the first open after a modem boot also carries a stale one, so
  `adb` sees two back-to-back and marks the device `offline`. The bridge
  discards data queued at open time.
- **SSR-resilient.** If the modem's subsystem restarts mid-session, the
  bridge reopens the chardev (bounded backoff, `--ssr-budget`) instead
  of crashing; the `adb` client stays connected.
- **One client at a time.** The bridge serves a single `adb connect`.

## Optional: auto-start the bridge

For a hands-off setup, a `mhi-adb-bridge.service` systemd unit paired
with a udev rule (`KERNEL=="mhi_ADB"`, `SYSTEMD_WANTS+=...`, and
`BindsTo=dev-mhi_ADB.device` in the unit) can start the bridge whenever
`/dev/mhi_ADB` appears and stop it when the channel goes away. The
device appears asynchronously seconds after PCI coldplug, so prefer
udev activation over a static `After=`. This repo ships the tools, not
the unit — wire it to your own init as you see fit.
