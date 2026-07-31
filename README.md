# cellular

Notes, tools, and reverse-engineering findings for working with cellular modems — the cards and modules inside phones, hotspots, routers, and IoT gear.

The centerpiece is a small family of open tools, the **`diag*` toolkit**, for **capturing and decoding the raw diagnostic logs a modem emits**. Put a modem into diagnostic mode and it streams a firehose of its own internal signaling — every cell it sees and how strong each signal is, every GNSS fix, every LTE/5G measurement — as opaque binary log records. These tools capture that stream and turn it into named, typed, readable fields.

The rest of this repo is a growing collection of per-device notes, guides, and cross-vendor references built up along the way.

---

## The `diag*` toolkit

Getting usable data out of a modem's diagnostic interface is a short pipeline. Two tools do the main job:

```
  connected modem              capture                decode
  (USB or network)  ──▶  diaggulp  ──▶  raw log file  ──▶  diaggrok  ──▶  named fields
                                                                          → your analysis
```

- **[`diaggulp`](https://github.com/lukejenkins/diaggulp) — capture.** The program you actually run. Point it at a connected modem (over USB or the network) and it records the raw diagnostic log stream to a file. Most people start here.
- **[`diaggrok`](https://github.com/lukejenkins/diaggrok) — decode.** The star of the show. It takes those raw log bytes and hands back named, typed fields — turning cryptic log codes and opaque payloads into readable data: **signal strength**, serving and neighbor cells, GNSS fixes, LTE/5G measurements. It won't guess: a record whose byte layout it hasn't actually reverse-engineered and verified is **left undecoded**, never filled in with plausible-looking but wrong values.

Two more pieces make those work — and stand on their own:

- **[`diagmunge`](https://github.com/lukejenkins/diagmunge) — transport + formats.** The shared core the other tools lean on: moving diagnostic frames over serial/USB/TCP/UDP and converting between capture formats. Useful by itself if you're building your own pipeline.
- **[`diagbarf`](https://github.com/lukejenkins/diagbarf) — optional on-device egress.** A small helper for the minority of setups where you can't reach the modem's diagnostic port from your host. Most people never need it; it feeds the same pipeline.

Each tool does one thing well and is useful on its own — capture without decode, decode without capture, transport without either. Take just the part you need, or build on any single piece. They're small and permissively licensed on purpose: easy to share, easy to build on.

---

## Guides & how-tos

Task-oriented walkthroughs, drawn from the per-device notes below:

- [Root / ADB shell on a Foxconn T99W640](/foxconn/t99w640/adb_root_shell.md) — get an authenticated root shell on the modem's application processor.
- [Dump the `foxnv` partition (Foxconn T99W640)](/foxconn/t99w640/foxnv_partition_dump.md) — pull the modem's NV/config partition for offline analysis.
- [GNSS on a stripped modem (Orbic RC400L)](/orbic/rc400l/gnss-driver/QMI-LOC-GNSS-Driver.md) — a QMI LOC GNSS driver for MDM9207 devices whose stock GNSS stack was removed.
- [Block carrier remote management (Casa Systems CFW-3212)](/casasystems/cfw3212/guide_block_carrier_remote_mgmt.md) — stop a carrier from remotely managing a 5G FWA CPE you own.

---

## Modules & devices

Per-module notes: identity, AT commands, firmware captures, and quirks.

### Casa Systems

- [Casa Systems CFW-3212](/casasystems/cfw3212/) — 5G FWA CPE (Qualcomm SDX62 / Quectel RG520N-NA OpenCPU). Root unlock tool and carrier remote management blocking guide.
  - [Flash Layout](/casasystems/cfw3212/flash_layout.md)
  - [Block Carrier Remote Management](/casasystems/cfw3212/guide_block_carrier_remote_mgmt.md)
  - [Web UI Screenshots](/casasystems/cfw3212/screenshots/)

### Compal

- [Compal RXM-G1](/compal/rxm-g1/) — 5G Sub-6 module / CPE gateway (Qualcomm SDX55). USB gadget composition guide (configfs, safe `option` driver binding), AT command reference, C-V2X capability notes.
  - [USB Composition](/compal/rxm-g1/usb-composition.md)
  - [AT Commands](/compal/rxm-g1/at_commands.md)
  - [Publicly Available Docs](/compal/rxm-g1/publicly_available_docs.md)

### Foxconn

- [Foxconn T99W640](/foxconn/t99w640/) — 5G module. Root ADB shell and `foxnv` partition dump guides.
  - [Root ADB Shell](/foxconn/t99w640/adb_root_shell.md)
  - [`foxnv` Partition Dump](/foxconn/t99w640/foxnv_partition_dump.md)

### Orbic

- [Orbic RC400L](/orbic/rc400l/) — LTE Cat 4 MiFi hotspot (Qualcomm MDM9207). QMI LOC GNSS driver for stripped MDM9207 devices.
  - [QMI LOC GNSS Driver](/orbic/rc400l/gnss-driver/QMI-LOC-GNSS-Driver.md)
  - [Build Instructions](/orbic/rc400l/gnss-driver/BUILD.md)
  - [References](/orbic/rc400l/gnss-driver/REFERENCES.md)

### Quectel

- [Quectel Overview](/quectel/)
- [Quectel BG95-M3](/quectel/bg95m3/) — LPWA module: NB-IoT / LTE Cat M1 (Qualcomm MDM9205). AT command docs and firmware captures.
  - [AT Commands](/quectel/bg95m3/at_commands.md)
  - [Publicly Available Docs](/quectel/bg95m3/publicly_available_docs.md)
- [Quectel EC2x / EG2x (EG25-G)](/quectel/eg25g/) — LTE Cat 4 module (Qualcomm MDM9207). AT command docs, firmware captures, scanning commands.
  - [AT Commands](/quectel/eg25g/at_commands.md)
  - [Publicly Available Docs](/quectel/eg25g/publicly_available_docs.md)
- [Quectel RM502Q](/quectel/rm502q/) — 5G Sub-6 module (Qualcomm SDX55). AT command docs.
  - [AT Commands](/quectel/rm502q/at_commands.md)
  - [Publicly Available Docs](/quectel/rm502q/publicly_available_docs.md)

### Telit

- [Telit Overview](/telit/)
- [Telit LM960](/telit/lm960/) — LTE Cat 18 module (Qualcomm SDX20). AT command docs and firmware captures.
  - [AT Commands](/telit/lm960/at_commands.md)
  - [Publicly Available Docs](/telit/lm960/publicly_available_docs.md)

---

## References

- [Cellular Modem Scan Commands Reference](cell-scan-commands.md) — Cross-vendor comparison of AT commands that return cell tower observations. Covers Fibocom, Quectel, Sierra Wireless, SIMCom, and Telit modems with data field matrices, WiGLE submission compatibility, and scan strategy recommendations.

### Elsewhere

**Information**

- [iamromulan/cellular-modem-wiki](https://github.com/iamromulan/cellular-modem-wiki)

**Tools**

- [QCSuper](https://github.com/P1sec/QCSuper) — capture raw 2G/3G/4G/5G radio frames from Qualcomm-based phones and modems.
- [SCAT: Signaling Collection and Analysis Tool](https://github.com/fgsect/scat)
- [MobileInsight](https://github.com/mobile-insight)

**Blogs**

- [markhoutz.com](https://markhoutz.com/)

**Privacy**

- [EFForg/rayhunter](https://github.com/EFForg/rayhunter) — worth watching; lots of extra logging that may be useful for PCI scanning/mapping ([discussion](https://github.com/EFForg/rayhunter/discussions/447)).
- [MarlinDetection/Marlin](https://github.com/MarlinDetection/Marlin)

**Unsorted**

- [Modifying USB mode on the Orbic RC400](https://mjg59.dreamwidth.org/61725.html) — blog post.
- [Osmocom Quectel EC25 wiki](https://www.osmocom.org/projects/quectel-modems/wiki/EC25)
- [the-modem-distro](https://github.com/the-modem-distro)

---

## Quests

- ~~Figure out a way to get geolocation data on the Rayhunter ([context](https://github.com/EFForg/rayhunter/issues/20#issuecomment-2762008339)).~~ Done — see the [Orbic RC400L GNSS driver](/orbic/rc400l/gnss-driver/QMI-LOC-GNSS-Driver.md).

---

## Prior art & thanks

QCSuper, SCAT, and osmo-qcdiag blazed this trail — they reverse-engineered and documented the DIAG protocol in the open over many years, and they're the reason tools like these can exist. This toolkit makes a different set of tradeoffs (small composable pieces, a permissive license), not a judgment on theirs — and it's built to compose with that ecosystem: the capture tools emit the same HDLC/DLF stream those tools already read.

---

## License

The `diag*` tools are released under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0) — a permissive license that makes them easy to share, embed, and build on. See each tool's repository for its own `LICENSE` and `NOTICE`.

The notes, guides, and references in this repository are the author's own work, shared for the community.
