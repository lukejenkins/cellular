# OOT `pcie_mhi` — ADB-over-MHI channel patches + kernel port

Patches to Quectel's out-of-tree `pcie_mhi` driver that expose MHI
channels 36/37 (`MHI_CLIENT_ADB_FB_OUT/IN`) as `/dev/mhi_ADB`, plus a
port to build the driver on modern (6.8–7.0) kernels. Cross-cutting and
multi-vendor: validated on Quectel **RM520N-GL-AP** and Foxconn
**T99W640 / Dell DW5934e** (SDX72).

With the patched driver loaded and the firmware's `AT+QPCIE="adb",1`
toggle set, the modem's `adbd` hands out an **unauthenticated root
shell** over `/dev/mhi_ADB`. Full evidence in [`FINDINGS.md`](FINDINGS.md).

> **The patches are required.** The stock Quectel OOT driver declares
> the ADB channel constants in its header but leaves them out of the
> channel *data* tables, so `/dev/mhi_ADB` never appears on a stock
> build — you get five UCI chardevs, not six. Patches `0001` + `0002`
> add the missing entries. This holds on both the RM520N-GL-AP and the
> T99W640.

## The patches

| Patch | What it does | Needed for |
|---|---|---|
| `0001-add-adb-channel-declaration.patch` | Declares the `"ADB"` channel (36/37) in `chan_cfg[]` (`core/mhi_init.c`) | `/dev/mhi_ADB` |
| `0002-add-adb-uci-match.patch` | Binds the `"ADB"` channel to a UCI chardev in `mhi_uci_match_table[]` (`devices/mhi_uci.c`) | `/dev/mhi_ADB` |
| `0003-kernel-6.8-7.0-api-port.patch` | Pure kernel-glue port (`no_llseek`→`noop_llseek`, const `bus_type.match`, `hrtimer_init`→`hrtimer_setup`, `strlcpy`→`strscpy`) | building on kernel 6.8+ |
| `0004-add-foxconn-dw5934e-105b-e11d-pci-id.patch` | Adds the retail Foxconn T99W640 / Dell DW5934e PCI ID `105b:e11d` to the match table | binding *that* device |

`0001` + `0002` are the core result. `0003` is only needed on newer
kernels. `0004` is a **worked device example** — a single PCI ID; other
modems need their own ID added the same way (find yours with `lspci -nn`).

## The driver (bring your own)

These are patches against Quectel's driver, which is **GPL-2.0-only**
(copyright "The Linux Foundation"). This repo distributes the *patches*,
not the vendor source — obtain the driver release from Quectel and
verify it before patching:

| Release | Archive SHA-256 |
|---|---|
| `Quectel_Linux_PCIE_MHI_Driver_V1.3.8` (recommended) | `a92a7036076d21702e8d5663f7fb4351a90461e9f257562eba9765b81ea3cece` |
| `Quectel_Linux_PCIE_MHI_Driver_V1.3.6` (older; hunks also apply) | `4f5bc6fc55525b2a16eb8ce829d78266e4705a77667ff9bea53a954eab6b7a45` |

```sh
sha256sum Quectel_Linux_PCIE_MHI_Driver_V1.3.8.zip   # must match the table above
unzip -q Quectel_Linux_PCIE_MHI_Driver_V1.3.8.zip -d build   # unpacks to build/pcie_mhi/
```

## Build

```sh
cd build/pcie_mhi
P=/path/to/this/patches/linux/pcie-mhi
patch -p1 < "$P/0001-add-adb-channel-declaration.patch"
patch -p1 < "$P/0002-add-adb-uci-match.patch"
patch -p1 < "$P/0003-kernel-6.8-7.0-api-port.patch"   # only on kernel 6.8+
patch -p1 < "$P/0004-add-foxconn-dw5934e-105b-e11d-pci-id.patch"   # only for the T99W640/DW5934e
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules   # -> pcie_mhi.ko
```

(The unpacked tree is not a git repo — use `patch -p1`, not `git apply`.)

## Load it, and make it the MHI driver

The in-tree `mhi_pci_generic` will claim the device first; swap to the
patched OOT driver:

```sh
sudo rmmod mhi_pci_generic          # releases the device (removes /dev/wwan0*)
sudo insmod ./pcie_mhi.ko
ls /dev/mhi_*                        # expect mhi_ADB among the nodes
```

To make the OOT driver the persistent MHI driver across reboots, rather
than hand-swapping each boot:

- **Blacklist the in-tree driver** — drop a `modprobe.d` snippet:
  `blacklist mhi_pci_generic`.
- **Autoload the OOT module** — add `pcie_mhi` to `/etc/modules-load.d/`
  (install the built `.ko` where `depmod` can find it, then `depmod -a`).

Under the OOT driver the control planes move: MBIM/QMI is native QMI on
`/dev/mhi_QMI0` (`qmicli --device-open-qmi`, *not* `--device-open-mbim`);
AT is `/dev/mhi_DUN`.

## Persist across kernel upgrades with DKMS (optional)

Rather than rebuilding by hand on every kernel bump, register the
patched driver with DKMS so it auto-rebuilds. Sketch:

1. Stage the unpacked, patched source under
   `/usr/src/quectel-pcie-mhi-adb-1.3.8/`.
2. Add a `dkms.conf` there:

   ```
   PACKAGE_NAME="quectel-pcie-mhi-adb"
   PACKAGE_VERSION="1.3.8"
   BUILT_MODULE_NAME[0]="pcie_mhi"
   DEST_MODULE_LOCATION[0]="/updates/dkms"
   MAKE[0]="make -C ${kernel_source_dir} M=${dkms_tree}/${PACKAGE_NAME}/${PACKAGE_VERSION}/build modules"
   CLEAN="make -C ${kernel_source_dir} M=${dkms_tree}/${PACKAGE_NAME}/${PACKAGE_VERSION}/build clean"
   AUTOINSTALL="yes"
   ```

3. `sudo dkms add -m quectel-pcie-mhi-adb -v 1.3.8`, then
   `sudo dkms install -m quectel-pcie-mhi-adb -v 1.3.8`.

Apply the patches when you stage the source (step 1), so every DKMS
rebuild builds the patched driver. `dkms status quectel-pcie-mhi-adb`
should report `installed`, and the module rebuilds on each kernel bump.

## Enable adbd, verify, use

1. **Firmware toggle** (persistent in NV): `AT+QPCIE="adb",1`, then
   reboot the modem (`AT+CFUN=1,1`). Without it, `/dev/mhi_ADB` still
   appears but nothing answers on it — the modem's adbd isn't started.
2. **Verify + bridge** with the two tools in
   [`/tools/mhi-adb/`](/tools/mhi-adb/): `mhi_adb_probe.py` confirms
   adbd is answering, `mhi_adb_bridge.py` relays the chardev to
   `adb connect 127.0.0.1:6555`.

## Safety

- **Cold-load only.** `rmmod pcie_mhi && insmod` while the modem is in
  mission mode can strand the driver "Waiting to enter READY state" (the
  host `MHI_RESET` is a soft doorbell the running firmware ignores) or
  strand the modem in Sahara-listening mode. Treat the patched `.ko` as
  boot-time-only: reboot the host between load cycles, or reboot the
  modem (`AT+CFUN=1,1`) *before* `rmmod`.
- A pristine baseline is always reproducible by re-extracting the
  verified driver archive.

## Deeper reading

- [`FINDINGS.md`](FINDINGS.md) — the empirical yes/no, with evidence and a threat-model note.
- [`UPSTREAM-ANALYSIS.md`](UPSTREAM-ANALYSIS.md) — what the OOT driver actually contributes vs. the in-tree MHI stack, and what an in-tree ADB-over-MHI driver would cost (~300–500 LOC).
- [`USERSPACE-ANALYSIS.md`](USERSPACE-ANALYSIS.md) — the road not taken: a VFIO userspace MHI driver, and why the kernel path wins here.

## Licensing

- **`*.patch`** — **GPL-2.0-only**. They are derivative of the
  GPL-2.0-only `pcie_mhi` driver; the patched work stays GPL-2.0-only.
  Each patch carries an SPDX header.
- **`/tools/mhi-adb/*.py`** — **Apache-2.0** (original work; SPDX headers in each file).
- **The prose docs** in this folder are under the repository's
  [CC-BY-SA-4.0](/LICENSE.txt), like the rest of the notes here.
