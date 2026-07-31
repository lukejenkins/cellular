# RXM-G1 — AT commands

The RXM-G1's AT command set is unusually minimal for a modern 5G module. It
answers baseline 3GPP TS 27.007 commands and not much else — there is no
`AT+QCFG`/`AT+QENG`-style vendor extension layer (Quectel), no `AT#RFSTS`
(Telit), no `AT!GSTATUS` (Sierra Wireless). If you're used to those modems,
the biggest adjustment is that **detailed serving-cell radio data (NR-ARFCN,
PCI, per-band signal breakdown) simply isn't available over AT on this
module.** For that level of detail you need the modem's diagnostic (DIAG)
log stream, decoded with a tool such as
[diaggrok](https://github.com/lukejenkins/diaggrok).

Command discovery: `AT+CLAC` and the vendor `AT$QCCLAC` both enumerate
supported commands; `$QCCLAC` returns a larger list including some
undocumented `$QC*` and legacy CDMA-era commands left over from the
Qualcomm reference build that don't do anything useful on this firmware.

## Identity

```sh
ATI            # manufacturer / model / firmware, e.g. COMPAL / 334 / RXMG1.20.00.244_0C03
AT+CGMI        # manufacturer
AT+CGMM        # model
AT+CGMR        # firmware revision
AT+CGSN        # IMEI
```

## Radio state

The module has been observed to boot with the radio **off** by default:

```sh
AT+CFUN?       # 1 = RF on, 0 = RF off (no network camp is possible)
AT+CFUN=1      # turn the radio on
```

This also matters for GNSS: the GNSS engine shares the cellular RF
front-end and clocking, so with the radio off it will not see satellites —
something that's easy to misdiagnose as an antenna problem.

## Registration and serving cell (wardriving-relevant)

These are the commands that give you what a WiGLE-style cell observation
needs. Poll them at a short interval (a couple of seconds) while moving.

```sh
AT+COPS?               # current operator: +COPS: 0,0,"<operator name>",<AcT>
AT+C5GREG=2             # enable extended unsolicited registration reporting for 5G
AT+C5GREG?              # +C5GREG: 2,<stat>,"<TAC>","<NCI>",<AcT>  — NR5G-SA tracking area + cell identity
AT+CEREG=2               # enable extended unsolicited registration reporting for LTE
AT+CEREG?               # +CEREG: 2,<stat>,"<TAC>","<CI>",<AcT>    — LTE tracking area + cell identity
AT+CESQ                 # signal quality — includes NR ss-RSRP / ss-RSRQ / ss-SINR fields
AT+CSQ                  # legacy RSSI-only signal quality
```

What's notably **missing** compared to other modems: no PCI, no EARFCN or
NR-ARFCN, no per-neighbor-cell report. `AT+C5GREG?`/`AT+CEREG?` give you
tracking area + cell identity (enough to place the observation), and
`AT+CESQ` gives you signal strength, but the fine-grained radio parameters
that let you distinguish sectors on the same site, or see what the modem is
*not* camped on, aren't exposed here.

## PDP context / APN table

```sh
AT+CGDCONT?     # list configured PDP contexts
```

The default profile table on this firmware pre-provisions two APNs beyond
the usual data/IMS/emergency set — profile 38 (`v2x_ip`, IPv6) and profile
39 (`v2x_non_ip`) — reserved for possible future C-V2X use. They are not
active in any configuration observed so far; see the parent
[README](README.md#c-v2x-capability).

## What doesn't work

- No `AT+QENG`/`AT#RFSTS`/`AT!GSTATUS`-equivalent detailed serving-cell
  query — see above.
- No AT-level lever for the PCIe-vs-USB data-plane personality or the USB
  gadget composition — that's a boot-time board configuration, not something
  toggled from the AT interface (see
  [usb-composition.md](usb-composition.md)).
- Vendor-prefixed commands from other chipset vendors' ecosystems
  (SIMCom `AT+C*`/`AT+Q*` variants, etc.) return `ERROR` — this is a
  Qualcomm reference AT implementation, not a rebadged third-party stack.

## Notes

- Full observed command list was captured with an automated `AT+CLAC` /
  `AT$QCCLAC` enumeration sweep. `AT+COPS=?` (full network scan) is slow
  (can take minutes) and, as on most modems, should not be polled
  frequently.
- Always refer to Compal's own AT command reference where available — this
  page reflects what was observed responding on one firmware build and
  should not be treated as an exhaustive spec.
