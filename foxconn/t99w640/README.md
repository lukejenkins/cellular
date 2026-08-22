# Foxconn T99W640 (Dell DW5934e)

Qualcomm Snapdragon **X72 (SDX72)** 5G Release 17 M.2 module, PCIe-attached
via Qualcomm's MHI protocol. Sold by Dell as the **DW5934e**; the underlying
design is the Telit Cinterion **FN990B** family (Foxconn is the ODM). Two SIM
slots: one removable physical SIM plus one soldered eUICC.


## Documentation in this directory

**Access & mechanism**
- [Root ADB shell over MHI](./adb_root_shell.md) — unauthenticated root shell via the MHI `ADB` channel (T99W640 specifics; the driver/bridge live at repo level: [`/patches/linux/pcie-mhi/`](/patches/linux/pcie-mhi/), [`/tools/mhi-adb/`](/tools/mhi-adb/)).
- [foxnv partition dump over ADB](./foxnv_partition_dump.md) — pulling the Foxconn NV partition.
- [`atsh.py`](./atsh.py) — minimal raw-`os.open` AT shell for the non-TTY `/dev/wwan0at0` WWAN char device.

**Reference & data**
- [Vendor AT command reference](./vendor_at_commands.md) — the 40 `AT+...` vendor verbs, SDC gating, and observed responses.
- [Capability survey summary](./survey_summary.md) — sanitized, model-level bands / RATs / QMI+MBIM services / SIM architecture.
- [Firmware behavior notes](./firmware_notes.md) — transport/composition architecture, MHI pipe map, customer/personality model, FOXAP QMI service surface.
- [FCC unlock — state of the public art](./fcc_unlock.md) — the upstream libqmi FOXAP path and third-party tools (external work, linked and credited).

**Attribution**
- [Credits & related work](./CREDITS.md) — the OpenWrt community thread and other efforts this builds on.

> **Redaction note.** Everything here is model-level. Per-unit and subscriber
> identifiers (IMEI, IMSI, ICCID, eUICC EID, MSISDN, serial numbers) and any
> live-network / location data have been removed by hand; only shared SKU/board
> identifiers (e.g. `DP25-42843-47`) remain.

---

## 1. Identity & hardware

| Field | Value |
|-------|-------|
| ODM part number | Foxconn (Hon Hai) **T99W640** |
| OEM design | Telit Cinterion **FN990B** (sub-variants FN990B34 / FN990B40) |
| Reseller model | Dell **DW5934e** |
| Chipset | **Qualcomm Snapdragon X72 (SDX72)** — Release 17 5G |
| RAT | 5G NR Sub-6 + LTE |
| LTE bands | 1–43 + ext 46/48/66/67/68/70/71 |
| NR5G bands | SA/NSA bands 1–94, incl. n92/n93/n94 (Release 17 NTN bands) |
| 3GPP release | Release 17 |
| Form factor | M.2 |
| Primary bus | **PCIe** via MHI (USB endpoint disabled by default — `boot_hsusb_comp=none`) |
| PCI VID:PID (production, Foxconn) | `105b:e11d` |
| PCI VID:PID (engineering / Qualcomm reference) | `17cb:0309` |
| HW model / board | `DP25-42843-47` (M.2 edge pinout drawing `DP25-42843-43`) |
| HW revision | `V005` |
| SIM | **2 slots**: 1× removable physical + 1× soldered eUICC |
| eUICC vendor | Thales/Gemalto (identified by the eUICC's issuer range; per-unit EIDs are not published — see the redaction note above) |

The Dell/Foxconn product tag is "SDX72M2" and the silicon is X72. The Telit
FN990B family hardware design guide names the applied SoC as SDX72-0 (FN990B40 /
HW1.00) or SDX72-1 (FN990B34 / HW1.10), which agrees.

### Why it is X72, not X75 — the `sdx75/generic` board-string caveat

The on-modem `/etc/os-release` reads `OPENWRT_BOARD="sdx75/generic"`, and other
on-device platform strings read `soc0/machine=SDXPINNL`, `soc0/soc_id=609`. It is
tempting to read `sdx75` as evidence of X75 silicon — but that is a mistake, and
worth documenting so it is not repeated:

- Qualcomm names an entire modem **software build-family** `sdx_5` (i.e.
  `sdx75`), and applies that name across both X72 and X75 silicon. A `sdx75`
  board/build string is therefore a **software-target name, not a hardware part
  number**.
- `SDXPINN` ("Pinnacle") is the shared X70/X72/X75 **die-family** name — again
  not an X75-specific SKU.
- The hard SoC-identity fields that *would* name a specific part
  (`chipset_ver`, `product_name`, `pc_platform_ssid`, `fw_ver`) read **empty**
  on this unit. No on-device string names X75.
- The product SKU (Dell DW5934e / Foxconn T99W640) and the Telit hardware design
  guide both identify the silicon as **X72 (SDX72)**.

Bottom line: the `sdx75/*` strings are the platform/build base, not the silicon
part. This is an SDX72 module.

Firmware self-labels its DMS manufacturer string as `DELL` on production
firmware and `Qualcomm` on engineering-sample firmware.

---

## 2. MHI attachment

The module attaches to the host PCIe root port and speaks Qualcomm's MHI (Modem
Host Interface) protocol. Two host drivers can bind it, and they expose
**different MHI channel sets and different userspace device nodes**.

### In-tree `mhi_pci_generic` (default udev binding, kernel 6.12+)

With the Foxconn PCI IDs (`105b:e11d`) the kernel selects the `foxconn-dw5934e`
channel config:

| MHI channel | Userspace device | Purpose |
|---|---|---|
| `mhi0_DIAG` | `/dev/wwan0qcdm0` | Qualcomm DIAG (QCDM) — NV read/write, log capture |
| `mhi0_DUN` | `/dev/wwan0at0` | AT command port (raw WWAN char device, see note) |
| `mhi0_IP_HW0_MBIM` | (kernel, MBIM data path) | Hardware-accelerated IP + MBIM data channel |
| `mhi0_LOOPBACK` | (kernel diagnostic) | MHI protocol loopback test channel |
| `mhi0_MBIM` | `/dev/wwan0mbim0` | MBIM control — registration, bearer setup, vendor commands |

There is **no native QMI channel** under this driver — the `foxconn-dw5934e`
channel table does not request one, so QMI control flows through MBIM
(`qmicli --device-open-mbim --device-open-proxy -d /dev/wwan0mbim0`). The modem
firmware does expose a QMI channel; whether it appears depends on the host
driver's channel-config table.

> Note: `/dev/wwan0at0` is a kernel WWAN char device, **not a TTY** — terminal
> tools that call `tcsetattr` (tio/picocom/minicom/pyserial) fail because there
> is no UART backing. The correct access pattern is raw
> `os.open(O_RDWR | O_NONBLOCK)` + `select()`.

With the Qualcomm reference PCI IDs (`17cb:0309`, seen on engineering samples),
the in-tree driver falls through to a generic SDX7x reference channel set
(`DIAG, IPCR, IP_HW0, IP_SW0, MBIM, QMI`) that includes native QMI
(`/dev/wwan0qmi0`) but **no DUN/AT channel**.

### Out-of-tree Quectel `pcie_mhi.ko`

The OOT `pcie_mhi.ko` driver (loaded by manual `rmmod`/`insmod`) exposes a
different channel set that **includes ADB and native QMI**:

```
/dev/mhi_ADB  /dev/mhi_BHI  /dev/mhi_DIAG  /dev/mhi_DUN  /dev/mhi_LOOPBACK  /dev/mhi_QMI0
```

Trade-offs of the driver swap: the in-tree `/dev/wwan0*` nodes disappear
(`/dev/wwan0mbim0`, `/dev/wwan0at0` are in-tree only); native QMI moves to
`/dev/mhi_QMI0` (open with `qmicli --device-open-qmi`, no MBIM tunnel); the DUN
port moves to `/dev/mhi_DUN`. The swap is reversible on production firmware
(`rmmod pcie_mhi && modprobe mhi_pci_generic`). A co-resident Wi-Fi MHI device
(`ath12k`) is a separate `mhi` instance and is unaffected.

---

## 3. Post-PERST# boot-state taxonomy

After a PCIe reset (PERST#), the module can land in one of four observable
states. **Judge the state by the MHI execution environment (EE) and the
channel/USB surface — not by the PCI ID.**

### Why not the PCI ID

Under the OOT `pcie_mhi` driver the PCI ID stays `17cb:0309` in **both** PBL and
mission mode; the runtime `105b:e11d` only appears under the in-tree
`mhi_pci_generic` driver. A routine that waits for `105b:e11d` to confirm boot
under the OOT driver waits for a signal that never arrives. A retail unit that
self-boots to mission mode still presents `17cb:0309` under the OOT driver while
being fully booted.

### The four states

| # | State | EE / marker | USB + channels | EDL? | Meaning |
|---|-------|-------------|----------------|------|---------|
| 1 | **Mission (healthy)** | `AMSS` / `MISSION MODE` | all 6 `/dev/mhi_*` (`ADB BHI DIAG DUN LOOPBACK QMI0`) | no | Fully booted; app firmware latched. |
| 2 | **Clean Sahara EDL** | BHI image-request active | `QUSB_BULK` at `05c6:9008` (OEM stage `105b:e11d`) | yes | In EDL; Sahara/Firehose reachable. |
| 3 | **Stuck in PBL** | EE=0 / `ee:PBL` | only `/dev/mhi_BHI`, no data channels | no | PBL never loaded SBL (e.g. a cold-boot UFS-init race — UFS doesn't train, so PBL has no image to request). |
| 4 | **EE=5 / `MHI_EE_PTHRU` terminal** | EE=5 / `MHI_EE_PTHRU` | no `05c6:9008`, no `QUSB_BULK`, no tty/wwan/cdc-wdm; no AT/MBIM/QCDM/ADB | no | Boot got past SBL into pass-through but never latched AMSS, and is not in EDL. |

Notes on the driver-agnostic discriminator:

- **Stuck in PBL** ⇒ `ee:PBL` and only `/dev/mhi_BHI` present (no data channels).
- **Booted to mission** ⇒ `AMSS` / `MISSION MODE` with all six `/dev/mhi_*`
  channels.

State 4 is distinct from state 3: in state 3 SBL never loaded (EE=0); in state 4
SBL *did* load (EE advanced to 5) but the AMSS handoff stalled. In the one
reported state-4 case the silicon was intact (endpoint still enumerated,
fuses fine) — the failure was a boot handoff, not a dead endpoint. `MHI_EE_PTHRU`
is normally a healthy sub-3-second `PTHRU → AMSS` transient; a *terminal* PTHRU
is the unusual case. (State 4 is a single externally-reported observation, not
yet reproduced on-bench.)

At **EE=5/PTHRU every software EDL trigger is unreachable** (no ADB, no USB-AT,
no QMI node), so the only recovery path is a hardware `FORCE_USB_BOOT` strap to
force raw `05c6:9008` beneath the stall.

---

## 4. M.2 pin hazards (HM2U V7 enclosure)

The rework.network / Wireless Haven **HM2U V7** M.2-to-USB enclosure was
engineered against the Sierra EM91xx pinout, where pins 20 and 22 are a 2-bit
boot-transport strap. On the T99W640 (Qualcomm SDX7x-family M.2 reference
pinout — the X72 and X75 share the M.2 edge pinout), **pins 20 and 68 are modem
OUTPUTS**, not strap inputs. Driving them from the host creates contention with
the modem's internal output drivers.

| HM2U V7 jumper | M.2 pin | T99W640 signal | Safe to drive? |
|---|---:|---|---|
| MODLE | 22 | `MI2S_DATA0` / `UART_RX` (input) | yes — any position |
| **MODLE1** | **20** | **`QTM3_PON` (modem output)** | **no — leave OPEN / disconnected** |
| TEST | 25 | `DPR_BODY_SAR_3P3_N` (input) | yes — LOW triggers SAR (harmless) |
| **ANT** | **68** | **`NAV_GPIO_1_DR_SYNC` (modem output)** | **no — leave OPEN / disconnected** |

For the T99W640 in an HM2U V7, leave **MODLE1 and ANT OPEN**. (Correct jumpering
does not by itself enable boot in this enclosure — a separate suspected cause is
missing pull-ups on M.2 sideband pins 6/8/26/50/67.)

### COEX UART probe — negative result

The **COEX UART** (M.2 Key-B pin 62 = `COEX_UART_TXD`, pin 64 = `COEX_UART_RXD`)
was tapped on an engineering sample via an FTDI FT232R adapter, using a rig
independently confirmed working as an AT console on a SIMCom SIM8202G-M2. On the
T99W640 there was **no usable UART**:

| Test | Outcome |
|---|---|
| RX at power-on | 1–2 isolated `0x00` framing nulls per power cycle, nothing else |
| RX boot-console stream | silent at 9600 through 3M baud |
| TX active `AT` / `ATI` / CR probe | no echo or response at any of 9 rates |

The lone `0x00` per power-on is the module's TXD pin initializing
(floating → driven idle-high), which confirms the module-TXD → adapter-RX path is
electrically connected — but there is no boot-console and no AT console on the
COEX UART of this unit. Most likely the COEX UART is not the boot/debug console
on the SDX72 reference design (the dedicated debug UART, if any, is a different
pin pair). Do not assume the COEX UART is a viable recovery/console path on this
module.
