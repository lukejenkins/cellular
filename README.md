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

The four Python tools are on PyPI as of their **`2026.11.8`** release, and each one requires its siblings at that version or newer. `diagbarf` is a static C binary, so it ships as a GitHub release (latest **`v2026.10.31`**):

| Tool | Job | Latest release | Install | Written in |
|---|---|---|---|---|
| [`diaggulp`](https://github.com/lukejenkins/diaggulp) | capture | [`v2026.11.8`](https://github.com/lukejenkins/diaggulp/releases/tag/v2026.11.8) | `pip install diaggulp` | Python |
| [`diaggrok`](https://github.com/lukejenkins/diaggrok) | decode — 197 log-code parsers | [`v2026.11.8.post1`](https://github.com/lukejenkins/diaggrok/releases/tag/v2026.11.8.post1) | `pip install diaggrok` | Python, zero dependencies |
| [`diagmunge`](https://github.com/lukejenkins/diagmunge) | transport + capture formats | [`v2026.11.8`](https://github.com/lukejenkins/diagmunge/releases/tag/v2026.11.8) | `pip install diagmunge` | Python |
| [`diaggpsd`](https://github.com/lukejenkins/diaggpsd) | live GNSS → gpsd / RINEX | [`v2026.11.8`](https://github.com/lukejenkins/diaggpsd/releases/tag/v2026.11.8) | `pip install diaggpsd` | Python |
| [`diagbarf`](https://github.com/lukejenkins/diagbarf) | on-device DIAG egress | [`v2026.10.31`](https://github.com/lukejenkins/diagbarf/releases/tag/v2026.10.31) | build from the repo | C (static) |

- **[`diaggulp`](https://github.com/lukejenkins/diaggulp) — capture.** The program you actually run. Point it at a connected modem (over USB serial, TCP, or UDP) and it records the raw diagnostic log stream to a file, using very little host CPU. Most people start here. *Latest:* first PyPI release. Arming finishes in seconds on modems that acknowledge each logging-setup step while the step is still being written; it used to wait out a 4-second timeout per step. Stopping diaggulp while it is still arming the modem now disarms it too.
- **[`diaggrok`](https://github.com/lukejenkins/diaggrok) — decode.** The star of the show. It takes those raw log bytes and hands back named, typed fields — turning cryptic log codes and opaque payloads into readable data: **signal strength**, serving and neighbor cells, GNSS fixes, LTE/5G measurements. It now covers **197 log codes** across GNSS, LTE, 5G NR, 2G/3G, IMS, and SIM, validated on chipsets from the MDM9x07 generation through SDX72. It is pure Python with no dependencies. It won't guess: a record whose byte layout it hasn't actually reverse-engineered and verified is **left undecoded**, never filled in with plausible-looking but wrong values. *Latest:* first PyPI release, with one new parser (the 5G NR cell-acquisition timeline). A few record fields were renamed or re-typed where the old decode was wrong; the [changelog](https://github.com/lukejenkins/diaggrok/blob/main/CHANGELOG.md) lists every one.

Three more pieces make those work — and stand on their own:

- **[`diagmunge`](https://github.com/lukejenkins/diagmunge) — transport + formats.** The shared core the other tools lean on: moving diagnostic frames over serial/USB/TCP/UDP and converting between capture formats (HDLC ↔ DLF ↔ JSONL ↔ pcap). Useful by itself if you're building your own pipeline. *Latest:* first PyPI release. GSMTAP export handles LTE channel numbers of 16384 and up, and a gap in a network-streamed capture is warned about once, not once for every pass that reads the stream.
- **[`diaggpsd`](https://github.com/lukejenkins/diaggpsd) — live GNSS.** Decodes a modem's GNSS DIAG records and serves them as a gpsd-compatible feed on TCP 2947 (so `cgps`, `gpspipe`, JOSM, and anything built on `libgps` just work), as raw JSON, or as RINEX for post-processing. Works on modems that expose no NMEA port or kernel GPS driver. *Latest:* first PyPI release. It now installs as a single `diaggpsd` package instead of a dozen top-level modules.
- **[`diagbarf`](https://github.com/lukejenkins/diagbarf) — optional on-device egress.** A small static C daemon for the minority of setups where you can't reach the modem's diagnostic port from your host: it runs on the modem's own processor and fans the DIAG stream out to TCP/UDP sinks on the network. It has been validated on Wistron, Casa Systems, Fibocom, Inseego, and Quectel hardware, including devices with no `/dev/diag`. Most people never need it; it feeds the same pipeline. *Latest:* non-blocking UDP sinks and a `--stall-watchdog` for supervisors like systemd. It disarms the modem on stop by default, and can also arm the DIAG event plane.

Each tool does one thing well and is useful on its own — capture without decode, decode without capture, transport without either. Take just the part you need, or build on any single piece. They're small and permissively licensed on purpose: easy to share, easy to build on.

Install the Python tools from PyPI, e.g. `pip install "diaggulp[serial]" diaggpsd`. The leaf tools pull in `diagmunge` and `diaggrok` themselves. Every release is tagged, and a tag reads like the PyPI version (`diaggulp==2026.11.8` is tag `v2026.11.8`), so you can pin either one. Each repo's README lists its extras.

New to DIAG captures? The [DIAG encapsulations and oracles guide](diag-encapsulations-and-oracles.md) explains the formats these tools handle, layer by layer, and what to check when a capture won't decode.

Wondering what hardware to use? The [tested hardware list](TESTED_HARDWARE.md) names the devices confirmed useful for diag* work and the firmware versions tested on each.

### Companion tool: `qfenix`

[`qfenix`](https://github.com/lukejenkins/qfenix) is a fork of [iamromulan/qfenix](https://github.com/iamromulan/qfenix), itself an enhanced [linux-msm/qdl](https://github.com/linux-msm/qdl). It is a single-binary Swiss Army knife for Qualcomm modems: EDL/Firehose flashing, NV items, EFS browsing and backup, GPT partitions, and an AT console. The latest release, [`v2.5.10-qfenix-0.2.0`](https://github.com/lukejenkins/qfenix/releases/tag/v2.5.10-qfenix-0.2.0), adds:

- a wxWidgets GUI and raw sector read/write
- PCIe/MHI modem discovery on Linux
- more robust DIAG/EFS handling, including `--no-offline` for read-only EFS access without taking the modem offline
- MIPS support and statically linked Windows builds

qfenix is BSD-3-Clause licensed, separately from the `diag*` tools.

---

## Guides & how-tos

Task-oriented walkthroughs, drawn from the per-device notes below:

- [Root / ADB shell on a Foxconn T99W640](/foxconn/t99w640/adb_root_shell.md) — get an unauthenticated root shell on the modem's application processor over ADB-over-MHI.
- [Dump the `foxnv` partition (Foxconn T99W640)](/foxconn/t99w640/foxnv_partition_dump.md) — pull the modem's NV/config partition for offline analysis.
- [GNSS on a stripped modem (Orbic RC400L)](/orbic/rc400l/gnss-driver/QMI-LOC-GNSS-Driver.md) — a QMI LOC GNSS driver for MDM9207 devices whose stock GNSS stack was removed.
- [Block carrier remote management (Casa Systems CFW-3212)](/casasystems/cfw3212/guide_block_carrier_remote_mgmt.md) — stop a carrier from remotely managing a 5G FWA CPE you own.

---

## Kernel patches & host tools

Cross-vendor building blocks, not tied to one device:

- [ADB-over-MHI — root shell on a PCIe/MHI modem](/patches/linux/pcie-mhi/) — patches for Quectel's out-of-tree `pcie_mhi` driver that expose `/dev/mhi_ADB` (channels 36/37), plus a kernel 6.8–7.0 build port. Validated on Quectel RM520N-GL-AP and Foxconn T99W640 / Dell DW5934e.
- [`mhi-adb` tools](/tools/mhi-adb/) — `mhi_adb_probe.py` (verify adbd is answering on the channel) and `mhi_adb_bridge.py` (relay `/dev/mhi_ADB` to `adb connect 127.0.0.1:6555`).

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

- [Orbic RC400L](/orbic/rc400l/) — LTE Cat 4 mobile hotspot (Qualcomm MDM9207). QMI LOC GNSS driver for stripped MDM9207 devices.
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

- [Tested hardware](TESTED_HARDWARE.md) — Devices confirmed useful for diag* work (modules, hotspots, and gateways from MDM9200 through SDX72), with the firmware versions tested on each.
- [Qualcomm DIAG: Encapsulations, Compression Formats and Decode Oracles](diag-encapsulations-and-oracles.md) — Field guide to reading DIAG captures layer by layer: transports, file containers, opcodes and the `0x98` wrapper, QShrink-compressed debug prints and the `qdsp6m.qdb` database, versioned log payloads, a chipset-generation matrix, a catalogue of decode oracles, and a "this frame won't decode" triage checklist.
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
