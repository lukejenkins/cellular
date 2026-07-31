# Compal RXM-G1

5G module (Sub-6, NSA + SA) built on the Qualcomm SDX55 platform. Ships as a bare
Qualcomm "LE" (Linux Embedded) reference image with only light OEM branding —
closer to a Qualcomm reference design than a heavily customized carrier device.

| Field | Value |
|-------|-------|
| Chipset | Qualcomm SDX55 (internal codename `sdxprairie`) |
| Category | 5G Sub-6 GHz module / 5G CPE gateway |
| Form factor | M.2, also seen as a finished USB-C dongle (see Rebrands below) |
| OEM/ODM | Compal Electronics Inc. (CEI) — Compal designs and builds this one itself |
| Command prefix | 3GPP-standard only (no vendor `AT+Q*`/`AT^*`/`AT!*` extensions) |
| FCC ID (original module grant) | `GKRRXMG1` |

## Current understanding

This page reflects what's confirmed by hands-on testing against a running unit.
It is **not** a datasheet — treat vendor documentation as authoritative where the
two disagree.

- The module identifies over AT as `COMPAL`, model `334`, with a firmware
  string like `RXMG1.20.00.244_0C03`.
- Root ADB access is available with no unlock procedure. USB access, in the
  base composition, gives you DIAG, ADB, and an AT (DUN) port — no MBIM/RmNet
  data interface. See [usb-composition.md](usb-composition.md) for why, and
  how to change it.
- The AT command set is thin. Unlike modems from Quectel, Telit, or Sierra
  Wireless, there is no vendor-specific serving-cell query — no `AT+QENG`,
  no `AT#RFSTS`, no `AT!GSTATUS`. Only baseline 3GPP `AT+C*` commands answer.
  See [at_commands.md](at_commands.md).
- GNSS works over the shared cellular RF front-end once the radio is powered
  on (`AT+CFUN=1`) — the module ships with the radio off by default, which
  can look like a GNSS/antenna problem if you don't know that.
- The SDX55 platform and this firmware carry evidence of **possible future
  C-V2X functionality** (PC5 sidelink + dedicated V2X APN profiles) — see
  [C-V2X capability](#c-v2x-capability) below. It is not active in any
  configuration observed so far.

## USB gadget architecture

The module's USB personality is driven by Linux **configfs** (`gsi`), not the
older Android `android_usb` sysfs interface many USB-serial guides assume.
Several USB "compositions" (interface sets) are selectable at runtime and at
boot. Getting the right host-side serial ports bound reliably — without
losing your ADB connection — is the single most useful thing to get right on
this module. Full writeup: [usb-composition.md](usb-composition.md).

## Rebrands

Compal is the original designer and FCC grant holder for this module. Other
brands sell it, or a dongle built around it, under their own model names and
their own FCC grants (a "class II permissive change," C2PC, off Compal's
original grant):

| Brand | Model | Form | FCC ID |
|-------|-------|------|--------|
| Compal (CEI) | RXM-G1 | M.2 module | `GKRRXMG1` |
| Tri Cascade (TRITOM) | SG500M2-X | M.2 module | `2ACARSG500M2` |
| Tri Cascade | VOS 5G Dongle | USB-C dongle | `2ACARVOS5GC` |
| APAL | Tributo 5G Dongle | USB-C dongle | (rebrand of the VOS dongle) |

FCC filings for these grants are public — search the FCC ID at
[fccid.io](https://fccid.io/) or [device.report](https://device.report/) for
exhibits, internal photos, and test reports.

## Firmware

Firmware version strings follow `RXMG1.<train>.<build>_<branch>`, e.g.
`RXMG1.20.00.244_0C03`. Two marketing trains have been observed in the wild
(`20.00.x` and `27.00.x`), both built on the same SDX55 "LE" baseline
(`SDX55.LE.1.2.r1`). Branch suffixes (`_0Cnn`/`_0Enn`/`_0Rnn`) track the
rootfs/OEM personality, not the cellular modem firmware itself — builds
sharing a branch line have been observed to ship byte-identical modem images.

Firmware updates for the retail Tri Cascade VOS dongle are distributed as
Android-recovery-style OTA packages and a Windows MBIM firmware-update
bundle carrying Qualcomm's `QCMBFWUpdateDriver`.

## C-V2X capability

The SDX55 silicon and every RXM-G1 firmware build inspected so far carry
Cellular Vehicle-to-Everything (C-V2X) capability in the modem image: PC5
sidelink RF configuration code, FTM V2X test-mode support, and two
dedicated APN profiles pre-provisioned in the default PDP context table
(`v2x_ip`, `v2x_non_ip`). None of this is exposed or active at runtime in
any configuration tested — no V2X network interface appears, and the USB
gadget cannot instantiate a V2X data function in the stock configuration.
Treat it as **possible future C-V2X functionality** baked into the
platform rather than a feature you can turn on today.

## Contents

| Document | Description |
|----------|--------------|
| [usb-composition.md](usb-composition.md) | USB gadget compositions, how to pick one, and how to bind the right host serial driver to the right endpoints without breaking ADB |
| [at_commands.md](at_commands.md) | AT command basics: identity, radio state, registration, signal — and what's *not* available |
| [publicly_available_docs.md](publicly_available_docs.md) | Links to public reference material: FCC filings, community firmware notes |

## See also

- [Cellular Modem Scan Commands Reference](../../cell-scan-commands.md) — this
  module's wardriving-relevant AT commands are folded into the cross-vendor
  comparison there.
