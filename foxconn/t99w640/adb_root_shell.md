# Root ADB shell — Foxconn T99W640

## Summary

ADB requires no enablement on the modem. `adbd` runs as root in the
default firmware. There is no AT challenge-response, USB composition
switch, NV edit, EDL step, or unlock key. ADB is exposed as an MHI
channel (`/dev/mhi_ADB`), not a USB endpoint. Access is a host-side
procedure: load a driver that registers the ADB channel, then bridge the
character device to a TCP socket for the `adb` client.

## Background

- The card is PCIe-only. Default USB composition is
  `/etc/usb/boot_hsusb_comp = none` (USB disabled at the modem). No USB
  ADB interface exists.
- ADB is an MHI channel. The mainline in-tree `mhi_pci_generic` driver
  does not register the ADB channel in its channel table for this device;
  only `/dev/wwan0*` (MBIM/AT/DIAG) appear. The Quectel out-of-tree
  `pcie_mhi.ko` driver registers the ADB channel.

## Requirements

- Card installed in a Linux host with a working PCIe-MHI stack (verified
  on kernels 6.12 and 6.19). Windows hosts cannot reach `/dev/mhi_ADB`.
- Out-of-tree module build toolchain: `make`, kernel headers for
  `$(uname -r)`.
- `adb` client.
- Root on the host.

## Procedure

### 1. Build the Quectel OOT MHI driver

Source: Quectel `quectel_MHI` / "5G-Modem" (public). The driver targets
Qualcomm SDX-family silicon; the T99W640 (SDX72) MHI channel layout is
compatible.

```bash
# in quectel_MHI/src
make KVER=$(uname -r)   # produces pcie_mhi.ko
```

Rebuild against the new `$(uname -r)` if the host kernel changes.

### 2. Swap to the OOT driver

```bash
sudo rmmod mhi_pci_generic        # removes /dev/wwan0* — see Side effects
sudo insmod ./pcie_mhi.ko
ls /dev/mhi_*
# Expected: mhi_ADB  mhi_BHI  mhi_DIAG  mhi_DUN  mhi_LOOPBACK  mhi_QMI0
```

`/dev/mhi_ADB` present confirms the channel is registered. No modem-side
change is required.

### 3. Bridge the ADB chardev to TCP

`adb` uses a socket transport, not a character device. `mhi_adb_bridge.py`
forwards `/dev/mhi_ADB` to `127.0.0.1:6555`. Two implementation details
it accounts for:

- Drain-on-open: the OOT ADB channel queues a `CNXN` frame on every
  open. The first open after a modem boot also carries a stale `CNXN`
  from the prior session. Two consecutive `CNXN` frames cause `adb` to
  treat the device as reset and mark it `offline`. The relay discards
  data queued at open time.
- Port 6555: on 5555 the adb server's emulator auto-discovery range
  (5554–5682) claims the bridge as `emulator-5554` and marks the
  explicit `adb connect` entry offline.

Run detached:

```bash
setsid python3 mhi_adb_bridge.py < /dev/null > /tmp/mhi_adb_bridge.log 2>&1 &
```

### 4. Connect

```bash
adb connect 127.0.0.1:6555
adb -s 127.0.0.1:6555 shell
# uid=0(root)  OpenWrt 22.03.5  sdx75/generic  aarch64  kernel 5.15.x
```

`adb pull` / `adb push` operate against the on-modem filesystem.

## Side effects

- Loading `pcie_mhi.ko` removes `/dev/wwan0mbim0` and `/dev/wwan0at0`.
  Under the OOT driver:
  - MBIM/QMI control: `/dev/mhi_QMI0` with `qmicli --device-open-qmi`
    (native QMI, not tunneled through MBIM; do not pass
    `--device-open-mbim`).
  - AT: `/dev/mhi_DUN`.
- Reversible: `sudo rmmod pcie_mhi && sudo modprobe mhi_pci_generic`
  restores the in-tree driver and `/dev/wwan0*` devices.
- The bridge serves one `adb connect` at a time.
- `/usrdata/local` is non-persistent; data pushed there does not survive
  reboot. Use `/data` or `/persist` for persistent tooling.

## Filesystem dump

With the root ADB shell, ADB-reachable partitions and
`/sys/devices/virtual/oem/sw/*` are accessible via `adb pull` and the
on-modem shell.
