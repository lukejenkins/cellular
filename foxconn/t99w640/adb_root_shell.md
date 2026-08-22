# Root ADB shell — Foxconn T99W640

The T99W640 (SDX72) serves an **unauthenticated root shell** over ADB —
no AT challenge, USB-composition switch, NV edit, EDL step, or unlock
key. ADB is exposed as an **MHI channel** (`/dev/mhi_ADB`), not a USB
endpoint, so access is a host-side procedure: load a driver that
registers the ADB channel, then bridge the chardev to a TCP socket for
the `adb` client.

The driver mechanics are **not specific to this modem** — the same
out-of-tree `pcie_mhi` patches, build steps, probe, and bridge cover the
T99W640 alongside other PCIe/MHI modems. They live at repo level:

> **➡ [`/patches/linux/pcie-mhi/`](/patches/linux/pcie-mhi/)** — the
> patches (incl. `0004`, which adds *this* modem's PCI ID), build/DKMS
> instructions, findings, and analysis.
> **➡ [`/tools/mhi-adb/`](/tools/mhi-adb/)** — `mhi_adb_probe.py` (verify)
> and `mhi_adb_bridge.py` (relay to `adb connect 127.0.0.1:6555`).

Follow those for the full apply → verify → use flow. What's below is
only what's **specific to the T99W640**.

## T99W640 specifics

- **PCIe-only card.** Default USB composition is
  `/etc/usb/boot_hsusb_comp = none` — USB is disabled at the modem, so
  no USB ADB interface exists. The MHI/PCIe path is the only route.
- **The patches are required.** The stock Quectel OOT driver does *not*
  expose `/dev/mhi_ADB`; patches `0001` + `0002` add the channel. This
  was confirmed on the T99W640 (the working sessions used the *patched*
  `pcie_mhi.ko`), matching the RM520N result in
  [`FINDINGS.md`](/patches/linux/pcie-mhi/FINDINGS.md).
- **PCI ID.** The retail unit (DF.001) enumerates as **`105b:e11d`**
  (Foxconn / Dell DW5934e) and needs patch `0004` to bind. An
  engineering sample enumerates as `17cb:0309` (already in the stock
  match table).
- **Control planes under the OOT driver.** QMI is native QMI on
  `/dev/mhi_QMI0` (`qmicli --device-open-qmi`, *not* `--device-open-mbim`);
  AT is `/dev/mhi_DUN`.
- **The root shell.** `adb -s 127.0.0.1:6555 shell id` → `uid=0(root)`;
  an OpenWrt-based `sdx75/generic aarch64` userspace on a 5.15-series
  kernel. `adb pull` / `adb push` operate against the on-modem filesystem.
- **Non-persistent scratch.** `/usrdata/local` does not survive reboot —
  use `/data` or `/persist` for anything you want to keep.
- **Reversible.** `sudo rmmod pcie_mhi && sudo modprobe mhi_pci_generic`
  restores the in-tree driver and the `/dev/wwan0*` nodes. Mind the
  cold-load-only hazard noted in the patches README before hot-swapping.

## Filesystem dump

With the root ADB shell, ADB-reachable partitions and
`/sys/devices/virtual/oem/sw/*` are accessible via `adb pull` and the
on-modem shell.
