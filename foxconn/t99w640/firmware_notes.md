# T99W640 / DW5934e — Firmware behavior notes

Derived, verified facts about how the on-module firmware behaves: the
transport/composition architecture, the MHI pipe→service map, the USB
composition catalog, the customer/personality model, and the Foxconn "FOXAP"
QMI service surface.

> **Scope.** This page documents *observable behavior and protocol surface* —
> the kind of interop facts you need to talk to the modem. It intentionally
> contains **no decompiled vendor code**, no firmware disassembly, and no
> derivation of how any vendor/regulatory lock is enforced. Values below were
> established by reading the module's own OpenWrt rootfs scripts + SELinux
> file-contexts (clean-room, apps-side) and by observing the modem live;
> conclusions that required binary analysis are stated as facts only.

---

## 1. Transport architecture: PCIe/MHI vs USB gadget

The module boots in **one of two transport modes**, selected at boot:

- **PCIe / MHI (default).** The USB gadget is forced empty
  (`/etc/usb/boot_hsusb_comp = none`) and host interfaces are exposed as MHI
  channels (`/dev/mhi_*` on the host, from `/dev/mhi_pipe_N` on the module).
  This is the shipping DW5934e behavior.
- **USB gadget.** A configfs USB composition (`/sbin/usb/compositions/<HEX>`)
  is brought up. The SDX72 firmware's built-in USB-mode default is composition
  **`90D5`** (DIAG + ADB + MBIM + GNSS + DUN).

The choice is driven by a small set of persistent inputs:

| Input | Effect |
|---|---|
| `/dev/mhi_ctrl` present | MHI vs pure-USB path |
| `/data/debug_transport.conf` (`pcie` \| `usb`, default `pcie`) | the primary apps-side transport knob; persists, reboot to switch |
| `/etc/usb/boot_hsusb_comp` | `none` → MHI (no gadget); empty → `901D`; else → that composition PID |
| SoC = `sdxpinn` | selects the `90D5`-default branch |
| customer-id (vendor partition) | selects USB VID:PID identity + personality (see §4) |
| `AT+USBSWITCH` | the only AT command that rewrites the active composition PID |

## 2. MHI pipe → service map (PCIe mode)

In PCIe/MHI mode the interface set is fixed by the firmware below the apps
layer. Verified from SELinux `file_contexts` + per-service init scripts:

| Module node | Host-side | Service |
|---|---|---|
| `/dev/mhi_pipe_4`  | `/dev/mhi_DIAG` | Qualcomm DIAG |
| `/dev/mhi_pipe_12` | MBIM | MBIM control/data |
| `/dev/mhi_pipe_14` | QMI / rmnet control | QMI |
| `/dev/mhi_pipe_32` | AT | AT command port |

Under the OOT `pcie_mhi.ko` driver the channel set presented is
`ADB, BHI, DIAG, DUN, LOOPBACK, QMI0`; under the in-tree `mhi_pci_generic`
driver (Foxconn `105b:e11d` alias) it is `DIAG, DUN, IP_HW0_MBIM, LOOPBACK, MBIM`.

## 3. USB composition catalog

Each `/sbin/usb/compositions/<HEX>` script writes an `idVendor`/`idProduct` and
symlinks configfs functions. There are **61 compositions** in the firmware; the
high-value ones:

| PID | Interfaces | DIAG | AT | ADB |
|---|---|:--:|:--:|:--:|
| `901D` | DIAG + ADB | ✅ | | ✅ |
| `901F` | DIAG + ADB + DUN | ✅ | ✅ | ✅ |
| `9025` | DIAG+ADB+MODEM+NMEA+QMI_RMNET+MS | ✅ | ✅ | ✅ |
| `905B` | MBIM only | | | |
| `9085` | DIAG+ADB+MBIM+GNSS | ✅ | | ✅ |
| **`90D5`** | **DIAG+ADB+MBIM+GNSS+DUN** (SDX72 USB-mode default) | ✅ | ✅ | ✅ |
| `90E2` | MBIM + GNSS | | ✅ | |
| `90E5` | dual-DIAG (MSM+MDM)+QDSS+DUN+DPL+RMNET+ADB | ✅✅ | ✅ | ✅ |
| `4EE7` | ADB only (VID `18D1`) | | | ✅ |

DIAG is exposed in ~45 of the 61 compositions; the runtime host-MBIM switcher
(`mbimd`) cycles among `{9043, 905A, 905B, 9063, 9085, 90E2}`.

## 4. Customer / personality model

The module carries a **customer-id** byte in its `vendor` partition (readable
live via `AT+CUSTOMER?`, written by `AT+CUSTOMER=N`). It is seeded into shared
memory at boot and selects the module's OEM personality. Observed live value on
the DW5934e: **33 (Dell)**.

The customer-id drives, among other things, the **USB identity** and the
**ATI manufacturer/model strings**:

| customer-id | personality | USB VID:PID | ATI manufacturer | ATI model |
|---|---|---|---|---|
| 32 | base | `105b:e118` | `Qualcomm` | `DP25-42843-47` |
| **33** | **Dell** | **`105b:e11d`** | **`DELL`** | `DP25-42843-47` |
| 34 | Thales | (falls to default) | `Qualcomm` | `DP25-42843-47` |
| 35 / 36 | HP | `103c:8da9` / `103c:8e09` | `HP` | `Qualcomm(R) snapdragon(TM) X72` |
| 0 | Qualcomm-generic | (compiled-in default) | `Qualcomm` | `DP25-42843-47` |

⚠️ **Known hazard:** writing `customer=0` (Qualcomm-generic personality) drops
the Foxconn ADB/AT MHI composition and flips ATI Manufacturer `DELL → Qualcomm`
— i.e. it can cost you the ADB foothold. Treat a deliberate customer write as
destructive; community reports of it "changing FCC-lock behavior" are discussed
in the OpenWrt thread (see `CREDITS.md`) and are outside the scope of these
notes.

## 5. Foxconn "FOXAP" QMI service (wire id `0xE4` / 228)

The module runs a proprietary Foxconn QMI service, **service id `0xE4` (228)**,
with **21 messages** (max message length 2650 bytes). libqmi upstream exposes
one of these as `--fox-ap-set-fcc-authentication` (see MR!417 in `CREDITS.md`).
Notable messages and their roles (protocol surface only — no handler internals):

| msg_id | Role |
|---|---|
| `0x5556` / `0x5557` | state-cache write / read (backed by `/tmp/fx_*` + DPR fifo) |
| **`0x5562`** | **`qmi_fx_reboot_to_mode`** — reboot selector: `5`→bootloader, `6`→**EDL**, `8`→fxbootloader, `9`→normal reboot |
| `0x5564` / `0x5566` | worker-thread + QMI-SAP setup / teardown |
| `0x5565` | fxlog fifo writer |
| `0x5567` / `0x5568` | get / **set** UART debug-console enable |
| `0x5571` | FCC-lock set (the message libqmi's FOXAP FCC-auth call uses) |

### Security-relevant behavior
- The FOXAP service **accepts connections without an authentication gate**, and
  `qmi_fx_reboot_to_mode` (`0x5562`) has no per-message auth — so any process
  with QMI client access to service `0xE4` can reboot the module, **including a
  software-only EDL entry** (`0x5562` with body `[0x06]`). In practice QMI-client
  access already implies the root/ADB foothold documented elsewhere in this
  directory, so this is a local-access behavior, not a remote one — but it's the
  cleanest software EDL trigger on this module (no hardware `FORCE_USB_BOOT` strap
  needed).
- `0x5568` toggles the UART debug console — of interest for hardware bring-up.

## 6. `AT+DIAG_ENABLE` behavior

`AT+DIAG_ENABLE` is **not** a composition selector. It toggles an on-module
latch (`/data/diag_disable`, inverse polarity), flips the WWAN LED, and reboots;
it is gated by the module's SDC state. Whether DIAG is actually reachable is
decided by the transport (`debug_transport.conf`): PCIe → DIAG on MHI pipe 4;
USB → the `diag.diag` function in the active composition (present in the `90D5`
default).
