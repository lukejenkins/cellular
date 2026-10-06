# Qualcomm DIAG: encapsulations, compression formats and decode oracles

> **Version 1.0 — 2026-10-06**
>
> Copyright © 2026 Luke Jenkins. Licensed under
> [CC-BY-SA-4.0](https://creativecommons.org/licenses/by-sa/4.0/) (full text in
> [LICENSE.txt](/LICENSE.txt)). You are free to share and adapt this guide for
> any purpose, including commercially, as long as you give appropriate credit,
> link to the license, note any changes you made, and release adapted versions
> under the same license.
>
> This is a living document. The latest version is at
> [github.com/lukejenkins/cellular](https://github.com/lukejenkins/cellular).

*A field guide for people learning to read Qualcomm DIAG captures: which
transport carried a byte, which container it was saved in, which wrapper it
sits inside, whether its text was compressed, and what outside reference — an
"oracle" — you need to make sense of it.*

DIAG is the diagnostic stream every Qualcomm cellular modem can emit: binary log
records, the firmware's own debug prints, event breadcrumbs, and a
command/response channel, all multiplexed over one serial-style link. Reading it
is mostly a framing problem, but the framing has more layers and more
generational variation than the usual "it's HDLC-encapsulated" summary suggests.
This guide concentrates on the parts that make captures *look* undecodable when
they are not.

It assumes you know what HDLC byte-stuffing is and roughly what a DIAG capture
is. Everywhere it can, it points at an open-source implementation — ours or
someone else's — that you can read or run.

**Contents**

1. [The layer model](#1-the-layer-model)
2. [Open-source tools from this effort](#2-open-source-tools-from-this-effort)
3. [Transports — how the bytes leave the modem](#3-transports--how-the-bytes-leave-the-modem)
4. [File containers — how the bytes are saved](#4-file-containers--how-the-bytes-are-saved)
5. [In-frame encapsulation — opcodes and wrappers](#5-in-frame-encapsulation--opcodes-and-wrappers)
6. [Compressed and hashed message formats](#6-compressed-and-hashed-message-formats)
7. [The QShrink database (`qdsp6m.qdb`) — format and where to get one](#7-the-qshrink-database-qdsp6mqdb--format-and-where-to-get-one)
8. [Log-record payloads — versions, and the "it's encrypted" trap](#8-log-record-payloads--versions-and-the-its-encrypted-trap)
9. [Chipset-generation matrix](#9-chipset-generation-matrix)
10. [Oracle catalogue — what each one answers and where to get it](#10-oracle-catalogue--what-each-one-answers-and-where-to-get-it)
11. [Triage checklist: "this frame won't decode"](#11-triage-checklist-this-frame-wont-decode)
12. [References](#12-references)

> **Where the numbers come from.** The measurements in this guide come from a
> corpus of roughly 2,200 captures from about 60 modem models, spanning
> MDM9x07 through SDX72, collected while building the tools in §2. Where the
> evidence is thin, the text says so. Percentages are per capture and depend
> on what each capture asked the modem to log, so read them as indicative —
> they describe captures, not silicon.

---

## 1. The layer model

A DIAG byte can pass through six layers between the modem and your parser. Most
"parse failures" are a reader applying the right decoder at the wrong layer.

```
 ┌──────────────────────────────────────────────────────────────────────────┐
 │ 6  Message content   log payload (versioned) │ F3 text / hash │ event id  │  ← oracles live here
 ├──────────────────────────────────────────────────────────────────────────┤
 │ 5  Inner wrapper     0x98 multi-radio → inner frame (0x10/0x79/0x99/0x9D) │
 ├──────────────────────────────────────────────────────────────────────────┤
 │ 4  Opcode            byte 0 of the deframed frame (0x10 LOG_F, 0x79 …)    │
 ├──────────────────────────────────────────────────────────────────────────┤
 │ 3  Framing           HDLC (7E-delimited, escaped, CRC-16/X-25) │ NHDLC    │
 ├──────────────────────────────────────────────────────────────────────────┤
 │ 2  Container         raw HDLC file │ flat DLF │ QMDL2 │ pcap of UDP relay │
 ├──────────────────────────────────────────────────────────────────────────┤
 │ 1  Transport         USB serial │ PCIe MHI │ /dev/diag │ diag-router/QRTR │
 │                      │ TCP/UDP relay                                       │
 └──────────────────────────────────────────────────────────────────────────┘
```

Two rules save the most time:

- **Identify the container by its content, never by its file extension.** Tools
  write raw HDLC under `.dlf`, "QMDL" files are almost always plain HDLC, and a
  flat DLF fed to an HDLC walker (or the reverse) produces *plausible garbage*
  rather than an error (§4).
- **Unwrap `0x98` every time.** Whether a modem wraps its log records depends on
  its internal routing path, not on its chipset generation (§5.2).

---

## 2. Open-source tools from this effort

Four small, separately usable projects came out of the work behind this guide.
They are referenced throughout, at the layer each one handles.

| Project | Layer | What it does |
|---|---|---|
| [diaggulp](https://github.com/lukejenkins/diaggulp) | 1 → 2 | Host-side capture. Arms the log mask (and optionally the F3 mask) and writes the raw HDLC stream to a file. Talks to a serial DM port, a TCP or UDP relay, or a pcap of one; can also write a live GSMTAP pcap for Wireshark. |
| [diagbarf](https://github.com/lukejenkins/diagbarf) | 1 | A single static binary that runs on a modem's own Linux (or on a host next to it), takes the one DIAG session the device allows, and fans it out to several network sinks at once. Its source modes map one-to-one onto the transports in §3. |
| [diagmunge](https://github.com/lukejenkins/diagmunge) | 1 – 3 | The transport and conversion library: a `DiagClient` for serial, raw file descriptors, TCP and UDP; the `DGE1` sequence tracker; HDLC → DLF conversion; and DIAG → GSMTAP pcap (`dlf_to_pcap`). |
| [diaggrok](https://github.com/lukejenkins/diaggrok) | 2 – 6 | The parsing library: content-based container detection, the HDLC walker (CRC check, `0x98` unwrapping, `0x80`/`0x9E`/binding-table parsers), and several hundred per-log-code parsers whose docstrings document each payload version's layout. `diagreplay` dumps a capture as JSON. |

None of these is required to follow the guide; every layout here is described
in enough detail to implement from scratch.

---

## 3. Transports — how the bytes leave the modem

| Transport | Framing on the wire | Typical devices | Implementations to read |
|---|---|---|---|
| USB serial "DM port" | HDLC + CRC-16/X-25 | almost every USB module | [diaggulp](https://github.com/lukejenkins/diaggulp), [diagmunge](https://github.com/lukejenkins/diagmunge), [QCSuper](https://github.com/P1sec/QCSuper), [ModemManager libqcdm](https://gitlab.freedesktop.org/mobile-broadband/ModemManager) |
| PCIe MHI (`/dev/mhi_DIAG`, `/dev/wwan0qcdm0`) | HDLC + CRC (same as USB) | M.2 PCIe modules (SDX55/62/72 class) | Linux `mhi` / `wwan` subsystem; diagbarf `hdlc-chardev` mode |
| Kernel `/dev/diag` (diagchar), on the modem's own Linux | `[u32 type][u32 count]` + N × `[u32 len][HDLC]` | hotspots and CPEs whose kernel has `CONFIG_DIAG_CHAR` | diagbarf `diag-dev` mode, [rayhunter](https://github.com/EFForg/rayhunter), [MobileInsight](https://github.com/mobile-insight/mobileinsight-core) |
| Userspace diag-router (abstract UNIX socket / QRTR) | `[u32 type]` + raw (un-HDLC'd) command; replies HDLC or raw | newer SDX6x CPEs whose kernel has no diagchar | [linux-msm/diag](https://github.com/linux-msm/diag) (the router itself); diagbarf `socket-log` mode via the vendor's `diag_socket_log` |
| TCP/UDP relay | raw HDLC byte stream, or HDLC chunks behind a small header | Ethernet CPEs, field relays | [diagbarf](https://github.com/lukejenkins/diagbarf) (sender), [diaggulp](https://github.com/lukejenkins/diaggulp) / [diagmunge](https://github.com/lukejenkins/diagmunge) (receiver) |
| NHDLC ("non-HDLC") | `7E 01 <u16 len> <data> 7E`, unescaped, no CRC | defined by the router; **not yet seen** in a real capture | [linux-msm/diag](https://github.com/linux-msm/diag) `router/diag.h` |

### 3.1 USB serial DM port

- **Framing:** take the payload, append a little-endian CRC-16, escape
  `7E → 7D 5E` and `7D → 7D 5D`, then end with a `7E`. Frames usually do **not**
  begin with a `7E`, so a raw capture often opens directly on `10 00 …`.
- **CRC:** reflected polynomial `0x8408`, initial value `0xFFFF`, final XOR
  `0xFFFF` (catalogued as CRC-16/X-25). Known answer:
  `crc("123456789") == 0x906E`. "CRC-16-CCITT" names several incompatible
  variants; the wrong one rejects every real frame, which looks exactly like a
  corrupt capture.
- **Finding the port:** it is "usually interface 0", but not reliably. A Sierra
  Wireless EM9190 exposes it on interface 4. On Qualcomm Linux USB-gadget compositions,
  protocol `0x30` is the **IPC** function — it acknowledges one write and then
  goes silent — and DIAG is the vendor-specific `ff/ff/ff` interface. **Send a
  `0x00 VERNO_F` request to confirm; don't pick the port from its descriptor
  alone.**
- **Quirks:**
  - Telit SDX20 modules hold each response until the host writes *again*
    ("flush on OUT"), so a request/response handshake needs a follow-up write.
    diaggulp handles this with `--telit-quirk`.
  - Some firmware filters the USB DM port down to NV commands only; one
    RM520N-GL build answered `0x26` (NV read) and nothing else.
  - Some USB DIAG functions are simply dead (a Fibocom FM101-GL never answers
    even `VERNO`).

### 3.2 PCIe MHI

The same CRC-protected HDLC as USB, delivered through a character device that
is **not a tty** (`tcgetattr` fails), so open it as a plain file descriptor.
diagbarf's `hdlc-chardev` mode does exactly that, on a host next to the modem.

- **The 8 KiB replay seam.** A widely circulated out-of-tree Quectel
  `pcie_mhi` driver uses 8,192-byte receive buffers. A DIAG packet longer than
  that arrives as `wire[0:8192] + wire[0:T-8192] + next_frame` — the first part
  is *replayed* instead of continued. On one SDX72 module this destroyed 99.4%
  of long frames. The in-kernel `mhi_pci_generic` / `wwan` path uses 32 KiB
  buffers and is unaffected. CRC failures concentrated on long frames are the
  tell.
- Expect `ERESTARTSYS` or `EAGAIN` while the MHI channel settles after a reset.

### 3.3 Kernel `/dev/diag` (on the modem)

On-device daemons — Qualcomm's own `diag_mdlog`, rayhunter, MobileInsight, and
diagbarf in `diag-dev` mode — read DIAG from the kernel's diagchar driver.

- **Mode switch:** `ioctl(fd, DIAG_IOCTL_SWITCH_LOGGING = 7, …)` with mode
  `USB = 1`, `MEMORY_DEVICE = 2` or `NO_LOGGING = 3`.
  `DIAG_IOCTL_REMOTE_DEV = 32` tells you whether writes need an extra field.
- **The argument struct grew across kernel versions, and the kernel checks its
  exact size.** This is the most common porting failure:

  | Form | Layout | Seen on |
  |---|---|---|
  | bare `int` | `ioctl(fd, 7, mode)` | older MDM9x07-era kernels |
  | 12 bytes | `{u32 req_mode, u32 peripheral_mask, u8 mode_param}` (padded) | many hotspots; some TP-Link builds want `mask=0, mode_param=1` |
  | 20 bytes ("V2") | varies by vendor tree — **not verified here** | reported for SDX62 hotspots |
  | 24 bytes | `{u32 req_mode, u32 peripheral_mask, u32 pd_mask, u16 mode_param, u8 diag_id, u8 reserved, i32 peripheral, i32 device_mask}` | SDX55 on kernel 4.14 (e.g. Inseego M2000/M2100) — this is what diagbarf's `diag-dev` mode sends |

  Try the sizes in turn and read `EINVAL` as "wrong size", not "no DIAG".
- **Reads** return `[u32 data_type][u32 num_entries]` followed by
  `num_entries × [u32 len][bytes]`. Keep data type `0x20`
  (`USER_SPACE_DATA_TYPE`) and skip the mask-update and control types. **One
  entry can hold more than one HDLC frame**, so split on `7E` inside each entry.
- **Writes** are `[u32 0x20][HDLC request]`, plus an `i32` field when
  `REMOTE_DEV > 0`.
- **Only one memory-device reader at a time.** A second one gets `EINVAL`, and
  two readers sharing a session corrupt each other's frames. That limitation is
  the reason diagbarf exists: one reader, many network sinks.

### 3.4 diag-router, abstract sockets and QRTR

Some newer SDX6x CPE kernels ship without diagchar at all. DIAG is then served by
the userspace router from [linux-msm/diag](https://github.com/linux-msm/diag) (or
a vendor fork of it).

- Clients connect to an abstract `AF_UNIX SOCK_SEQPACKET` socket and exchange
  `[u32 type]` messages; commands go as type `1` (raw data), **without HDLC**.
- Vendors often ship a `diag_socket_log` helper that connects to the router and
  re-serves the stream over TCP; diagbarf's default `socket-log` mode drives it.
- The router's logging-mode message carries the same
  `diag_logging_mode_param_t` as §3.3, plus an extra `req_mode = 7` (PCIe/MHI).
- **QRTR:** DIAG is QRTR service `4097` (`0x1001`). Command access (NV reads,
  for instance) works over the modem's command port; *log streaming* over raw
  QRTR uses a separate control/data channel set and is considerably more work.
- The router has a handful of client slots; clients that exit uncleanly can use
  them all up until the next reboot.

### 3.5 TCP and UDP relays

- **TCP** usually carries the raw HDLC byte stream, identical to what the modem
  emitted. Vendor relays and diagbarf keep the CRC; some consumers strip it
  without checking. Verify it rather than assuming either way.
- **UDP** datagrams are *chunks* of the HDLC stream, not whole frames, so they
  need a sequence number to detect loss and reordering. diagbarf's `DGE1` header
  is `"DGE1" | u8 ver | u8 flags | u32 LE seq | u16 LE len`, followed by the
  chunk. diagmunge's `Dge1SeqTracker` resequences them; diaggulp exposes it as
  `--transport udp-listen` (live) or `--transport pcap` (from a `tcpdump`), with
  `--udp-reorder-window`. A `DGE1` stream carries no stream id, so two senders on
  one port look like a gap of hundreds of millions of packets.
- **Some relays lose data silently.** A relay that forwards only `0x10` log
  records (with dummy CRCs) drops every F3 print, every `0x98` frame and every
  command response. One vendor socket-log path was measured dropping the entire
  `0x9D` plane that a direct serial capture of the same modem carried.

---

## 4. File containers — how the bytes are saved

| Container | Layout | Keeps F3 / events / commands? | Written by |
|---|---|---|---|
| Raw HDLC (`.hdlc`, `.bin`, "`.qmdl`", sometimes "`.dlf`") | the byte stream exactly as framed | **yes** | diaggulp, rayhunter, most `diag_mdlog` builds |
| Flat DLF | `u16 rec_len \| u16 log_code \| u64 ts64 \| payload` per record, no file header | **no — log records only** | QXDM-style exports, diagmunge `hdlc_to_dlf` |
| QMDL2 (v2) | `u32 header_length` (counts itself) + a binding table, then plain HDLC to the end | yes | `diag_mdlog -u` / `--qmdl2_v2` |
| pcap/pcapng of a UDP relay | link layer + UDP + relay header + HDLC chunk | yes, if the relay forwarded it | `tcpdump` on the relay port; read with diaggulp `--transport pcap` |
| ISF / HDF / `.mi2log` | vendor and MobileInsight formats | — | not covered here |

**Telling them apart by content.** diaggrok's `dlf.detect_format` returns one of
`dlf`, `hdlc`, `qmdl2-v2`, `pcap` or `unknown`, using these tests:

- **QMDL2:** a small `u32` at offset 0, *and* a CRC-valid DIAG frame starting at
  exactly that offset (`header_length`, not `header_length + 4`). Of 165 files
  named `.qmdl`/`.qmdl2` in our corpus, only **one** actually had this
  prologue; the rest were plain HDLC. The often-repeated "`10 5f 02` sub-record
  framing" inside QMDL2 is a phantom — a byte coincidence counted on data that
  had not been deframed (mostly inside `0x9D` payloads). Deframed properly, that
  region resolves into ordinary frames, 99.99% CRC-valid.
- **Flat DLF:** the first several records walk cleanly as
  `<u16 len><u16 code><u64 ts>` with known log codes, and the chain ends exactly
  at the end of the file.
- **Raw HDLC:** dense in `7E` bytes — but so are ELF and tar files, so insist on
  at least one CRC-valid frame before believing it.
- Expect junk at the front: zero-filled lead-ins, captures that start
  mid-frame, and short non-HDLC preambles all occur in real files.

**The DLF/HDLC trap.** A DLF record header and a deframed `0x10` header are both
12 bytes wide, with `log_code` and `ts64` at *different* offsets. Feed either
reader the other's bytes and it parses without complaint, producing wrong log
codes. Converting to DLF also **discards F3 prints, events, `0x80`, `0x9E` and
every command response** — keep the raw HDLC.

**Compressed capture files** (`.zst`, `.gz`, `.xz`) are only a storage concern,
but some decoders produce nothing at all on compressed input, so decompress
first.

---

## 5. In-frame encapsulation — opcodes and wrappers

After deframing (unescape, check and strip the CRC), byte 0 is the opcode.

| Opcode | Name | Role | Needs an oracle? |
|---|---|---|---|
| `0x10` | `DIAG_LOG_F` | binary log record (§8) | a parser for that log code |
| `0x98` | `DIAG_MULTI_RADIO_CMD_F` (header name `DIAG_CMD_EXT_F`) | routing wrapper around another frame (§5.2) | whatever the inner frame needs |
| `0x79` | `DIAG_EXT_MSG_F` | F3 debug print, **plain text** | no |
| `0x92` | `DIAG_QSR_EXT_MSG_TERSE_F` | F3, legacy QShrink hash | an index table from the modem image (§6.2) |
| `0x99` | `DIAG_QSR4_EXT_MSG_TERSE_F` | F3, QShrink 4 token | the exact build's `qdsp6m.qdb` |
| `0x9D` | `DIAG_QSH_TRACE_PAYLOAD_F` | QSH trace print, tokenised | the exact build's `qdsp6m.qdb` (Qtrace section) |
| `0x7E` | `DIAG_EXT_MSG_TERSE_F` | terse F3 | presumably a database; **never seen** |
| `0x60` | `DIAG_EVENT_REPORT_F` | event stream (§6.6) | an event-id name table |
| `0x80` | `DIAG_SUBSYS_CMD_VER_2_F` | here: the qdb **file transfer** (§5.3) | none — it *delivers* an oracle |
| `0x9E` | `DIAG_SECURE_LOG_F` | encrypted log (§5.4) | the key is not available |
| `0x4B` | `DIAG_SUBSYS_CMD_F` | subsystem command/response | depends on `(subsys, cmd)` |
| `0x00`, `0x1C`, `0x1D`, `0x7C` | identity: VERNO, DIAG_VER, TS, EXT_BUILD_ID | `0x7C` returns the build string you need to choose a qdb | no |
| `0x13` | `BAD_CMD_F` | refusal; the body echoes the refused request | no |
| `0x73`, `0x7D` | log mask, F3 mask | requests and responses look alike; only context tells them apart | no |

> `0x9C DIAG_MSG_SMALL_F` appears in vendor headers but has **never** been seen
> as a CRC-valid frame; the one apparent hit was the tail of a truncated `0x9E`.

For the command plane, the open-source router in
[linux-msm/diag](https://github.com/linux-msm/diag) and QCSuper's
[DIAG protocol write-up](https://github.com/P1sec/QCSuper/blob/master/docs/The%20Diag%20protocol.md)
are the best public references.

### 5.1 Reading the opcode before unescaping

No opcode equals `0x7D`, so byte 0 of a still-escaped frame *is* the opcode —
with one exception, `0x7E`, which has to travel as `7D 5E`. Counting opcodes on
raw bytes is cheap; counting anything deeper is not, because every escape
shifts the offsets after it.

### 5.2 `0x98` — the multi-radio wrapper

```
98 | u8 radio_id | 2 B pad | u32 tx_mask | inner frame (opcode first, no CRC of its own)
```

The outer CRC covers everything. The inner frame is a complete DIAG frame;
the inner opcodes seen in practice are `0x10`, `0x79`, `0x99` and `0x9D`.
diaggrok's HDLC walker unwraps it automatically (`hdlc.iter_log_records`).

**Who wraps, and how much.** Share of log records that arrived wrapped, by
family (depends on what was logged, so indicative):

| Family | Wrapped share of log records |
|---|---|
| MDM9x07, MDM9x30/35, MDM9600, MDM9250, MDM9150 | **0%** — never seen |
| MDM9640, SDX50M, SDX12, MDM9x50 (Sierra Wireless) | under 1% |
| SDX20 | ~2–5% typical (one outlier at 38%) |
| SDX24 | 7–59% |
| SDX55 | 15–95% (median ~32%) |
| SDX62 | 8–81% (median ~24%) |
| SDX65 | ~80–85% |
| SDX72 | 27% on one module; 100% on another's `diag_mdlog` capture |

So:

- "Newer chipset means everything is wrapped" is **false**. Captures with no
  bare `0x10` at all exist on SDX24, SDX55, SDX62 and SDX72 alike; most
  captures from SDX20 on are *mixed*.
- On SDX20 the split follows the producer: LTE layer-2 records (RLC/MAC/PDCP)
  arrive wrapped while GNSS, PHY and RRC records arrive bare, **in the same
  capture**.
- A parser that ignores `0x98` reports the capture as quiet or empty rather than
  unsupported. A debug-print extractor that looks only at top-level opcodes
  silently loses the wrapped F3.
- Several community decoders handle `0x98` partly or not at all; a pcap with
  headers but no packets from an SDX5x/6x capture is the classic symptom.

### 5.3 `0x80` — not a wrapper: a file transfer

On firmware from the QShrink 4 era (SDX20 onward), `0x80` subsystem `0x12`
(DIAG_SERV) carries a four-message protocol that streams the build's
`qdsp6m.qdb` — the database that decodes `0x99` and `0x9D` — to the host:

| `record_type` | Meaning | Key fields |
|---|---|---|
| 0 | FILE_LIST | `u16` count (big-endian), then N × (16-byte GUID + `u32` size) |
| 1 | OPEN | GUID, handle |
| 2 | READ | handle, `u32` offset, `u16` length, data (the request lists length before offset) |
| 3 | CLOSE | — |

The message counter's bit 15 is a *more data follows* flag, so the final chunk
legitimately breaks the "+1" sequence. Byte 8 is sometimes `0x10`, which tempts
readers to unwrap it like `0x98`; doing so produces a plausible log code over
bytes of a database file. **Don't.** diaggrok parses the header with
`hdlc.parse_subsys_v2_header`.

Why it matters: the FILE_LIST/OPEN names the build's qdb GUID **even when the
modem then refuses the transfer** (size 0), so it tells you which database you
need. `diag_mdlog -u` triggers the transfer on-device and saves the result as
`<guid>.qdb`. Leave these bytes out of any "how much of the telemetry did I
decode" figure — they are not telemetry.

### 5.4 `0x9E` — secure log

```
[1] version  [2] type flags  [12:16] u32 monotonic sequence  [24:] body
body: [0:4] nonce  [5] vendor tag  [6:8] per-build tag  [8:] ciphertext (~8.0 bits/byte)
```

Seen on SDX62, SDX65 and SDX72 modules from Quectel, Sierra Wireless and Foxconn. The
header decodes (diaggrok: `hdlc.parse_secure_log_envelope`); the body is
encrypted with keys that are not available — accounts differ on whether they
live in Arm TrustZone or are fused per device, and neither is confirmed. The
sequence number still gives you **loss detection** for a payload nobody can
read. This is the only DIAG plane in this guide that is genuinely "encrypted —
can't decode by design".

### 5.5 `0x4B` frames that matter for decoding

- **subsys `0x12`, cmd `0x0222`** — the `diag_id → protection-domain name`
  table, `[u8 ver][u8 n]` then `n × [u8 diag_id][u8 name_len incl. NUL][name]`.
  It is the in-band twin of the QMDL2 prologue, and it tells you which processor
  domain a tokenised message came from. It is a *response* to the five-byte
  request `4B 12 22 02 01`; the version byte is required, and the four-byte form
  is silently ignored. SDX55 and SDX62 answer it; SDX20 and MDM9607 refuse it.
  Every device measured so far reports `1 → APPS` and `2 → mdm/modem/root_pd`,
  but read it rather than assume it (diaggrok: `hdlc.parse_qshrink4_binding`).
- **subsys `0x12`, cmd `0x0218`** — HDLC_DISABLE, which switches a session to
  NHDLC framing (§3).
- **subsys `0x44`, cmd `0x9001`** — the QSH trace mask set that turns `0x9D` on.

---

## 6. Compressed and hashed message formats

The firmware's debug-print ("F3") stream exists in several encodings. The key
idea: **QShrink does not compress the payload — it replaces the format string
with a number** that only a database can turn back into text. The structure
(timestamp, arguments, identifier) always decodes; only the words need the
database.

| Format | Identifier | Oracle | Chipset era (measured) |
|---|---|---|---|
| `0x79` plain text | none — format string and filename travel in the frame | none | every generation |
| `0x92` legacy QShrink | `u32` hash | an index table in the modem image (file/line/argument count); format text not shipped | MDM9x07, MDM9x30/35/40, MDM9205, MSM8909 |
| `0x99` QShrink 4 | `u32` per-build token | that exact build's `qdsp6m.qdb` (`<Content>` section) | MDM9x50 / SDX20 onward (also MDM9205, MDM9150, QCM2290) |
| `0x9D` / log `0x1FE8` QSH trace | `u32` token | the same `qdsp6m.qdb`, `<MtraceContent>` section | SDX24 onward |
| `0x7E` terse | — | unknown | not seen |
| `0x60` events | 13-bit event id | an event-id name table | every generation (must be switched on) |

F3 is off until you subscribe to it with the `0x7D` mask, separately from the
log mask. diaggulp arms it with `--ext-msg-f3`, and `--ext-msg-f3-ss` limits it
to chosen subsystems — worth doing, because an unrestricted F3 capture on an
SDX6x part runs to tens of megabytes a minute.

### 6.1 `0x79` DIAG_EXT_MSG_F — decodes itself

```
[0] 0x79  [1] ts_type  [2] num_args  [3] drop_cnt  [4:12] u64 ts
[12:14] line  [14:16] ss_id  [16:20] ss_mask  [20:] num_args × u32, fmt\0, file\0
```

Works on any build with no outside data. A build can emit `0x79` and `0x99`
side by side, so a tool that renders only `0x79` makes a mixed build look
plain-text-only.

### 6.2 `0x92` legacy QShrink

```
[0] 0x92  [1] 0x00  [2:4] num_args  [4] chipset marker  [5:12] u56 ts (= u64 >> 8)
[12:14] line  [14:16] ss_id  [16:20] ss_mask  [20:24] hash  [24:] num_args × u32
```

The frame length is always `24 + 4·num_args`. (One public description puts a
`u64` timestamp at `[4:12]`; measurement shows a marker byte at `[4]` and a
56-bit timestamp after it.)

The lookup data comes in two halves. The **index** (`hash → file, line,
argument count`) is compiled into the modem firmware image itself, so it ships on
every device — but pulling it out of the image is firmware work beyond the scope
of this guide. The **format strings** are not shipped in production firmware at
all. Without the strings you still get a source file, a line number and the
arguments — a stable identity you can count, correlate and diff. The index
barely changes between point releases of one product family, unlike the
QShrink 4 database.

### 6.3 `0x99` QShrink 4

```
[0] 0x99  [1] ts_type  [2] num_size_args (low nibble = count, high = width)
[3] drop_cnt  [4:12] u64 ts  [12:16] u32 token  [16:18] u16 (unknown)  [18:] args
```

The frame checks itself: `len(args) == count × width`. Line, subsystem and
filename are not on the wire — they come from the database row.

- **The "hash" is not a hash of the string.** It is a token handed out in
  sequence for each build (neighbouring print sites get neighbouring values; the
  top byte is a segment tag). Only about 7% of tokens survive from one build to
  the next, and a database from a sibling build or a different carrier variant
  resolves only the shared core (under 9% in one measured case). **You need the
  exact build's database.**
- **The frame carries no GUID.** Match a capture to a database by build string
  (from `0x7C`, or `AT+GMR` and friends), by the GUID announced in `0x80` or a
  QMDL2 prologue, or by trying candidate databases and keeping the one that
  resolves the capture's tokens — a correct match resolves close to 100%, a
  wrong one close to 0%.
- Some vendors ship a second, same-build `qdsp6m_modem_oem.qdb` that has to be
  layered under the main one.
- To render `0x99` today with open-source tools, [SCAT](https://github.com/fgsect/scat)
  can load a `qdsp6m.qdb`.

### 6.4 `0x9D` QSH trace (and log `0x1FE8`)

```
[0] 0x9D  [1] subtype (0x45 text; others exist)  [2:4] subsystem / sub-stream bits
[4:6] —  [6:8] client handle  [8:12] u32 ts  [12:16] u32 token  [16:] u32 args
```

The same token scheme as `0x99`, resolved against the **Qtrace** section of the
same `qdsp6m.qdb` — there is no separate QSH database. It has to be switched on
(subsys `0x44` cmd `0x9001`, with subsystems in ascending order); MDM9607-class
parts refuse the command. Some firmware strips whole subsystems' strings out of
the database, so how much resolves varies by build (about 24% on one RM520N-GL
build, 96–99% on others).

### 6.5 `0x9C` and `0x7E`

Both are listed in vendor headers; neither appears in a CRC-checked census of
about 2,000 captures, at the top level or inside `0x98`. If you think you have
one, check its CRC first.

### 6.6 `0x60` events

```
0x60 | u16 len | items…   item: u16 id_raw | ts (8 B, or 2 B if bit 15) | payload
id_raw: bit 15 = short timestamp, bits 14:13 = payload size (0/1/2 B inline,
        3 = u8 length prefix), bits 12:0 = event id
```

Bit 15 is not part of the id: `0x6C84` and `0xEC84` are both event 3204. The
stream is switched on with the two bytes `60 01`. Event **names** come from
`event_defs.h`-style tables found in public Android vendor source trees (ids up
to about 2670) and from community lists; most ids above that range, common on
SDX55 and later, have no public name. Flat DLF cannot hold events at all.

---

## 7. The QShrink database (`qdsp6m.qdb`) — format and where to get one

### 7.1 Format

```
[0:4]   magic "\x7fQDB"
[4:20]  16-byte GUID (RFC-4122 byte order)
[20:64] zero padding
[64:]   zlib stream (78 DA …) of a text table
```

Decompressed, it is a text file with a banner and sections:

```
# Hash File Format : <hash>:<ss_mask>:<ssid>:<line>:<file>:<string>      → <Content>        (0x99)
# Qtrace Format    : <hash>:<line>:<level>:<client>:<file>:<tag>:<string> → <MtraceContent>  (0x9D)
                                                                          <QtraceStrContent> (argument names)
```

Split each row on the first *n* colons only — format strings contain colons. A
modem database is typically 4.5–8 MB holding 240,000–440,000 rows. A text
variant, `msg_hash_<guid>.qsr4`, carries the same table uncompressed; it is
common on MDM9205/BG95-class modules and turns up on some vendor download sites.
`qdsp6a.qdb` (the audio DSP's) has been an empty stub on every device examined.

### 7.2 The GUID has two byte orders

| Layout | Where you see it |
|---|---|
| Mixed-endian (first three groups byte-reversed) | on the DIAG wire (`0x80` FILE_LIST/OPEN) and in a QMDL2 prologue |
| RFC-4122 | in the file header, in the `<guid>.qdb` filename `diag_mdlog -u` writes, and in `diag_qsr4_guid_list.xml` |

Example: wire `d17b0980-…` is file `80097bd1-…`. Normalise before comparing, or
nothing will match. Also beware that OEM overlay databases have been seen
**reusing one GUID across builds with different contents** — identify files by
a content hash, not by GUID.

### 7.3 Where to get one

| Source | How | Notes |
|---|---|---|
| The modem's own filesystem | with a shell or ADB on the module, copy `/firmware/image/qdsp6m.qdb`; per-chipset subdirectories include `…/image/sdx55/`, `…/image/olympic/` (SDX6x), `…/image/pinnacles/` and `…/image/modem_oem/` (SDX7x); Telit keeps one per firmware slot | the easiest source if you have shell access |
| A firmware update package | first check whether it contains one at all: `grep -c $'\x7fQDB' <package>`. Many vendors ship a `NON-HLOS.ubi` volume; extract it with [ubi_reader](https://github.com/onekey-sec/ubi_reader) (`ubireader_extract_files`). Some wrap a squashfs inside UBI (`ubireader_extract_images`, then `unsquashfs`) | no magic in the package usually means the database isn't in it |
| Over DIAG | the `0x80` DIAG_SERV transfer (§5.3) — `diag_mdlog -u` on the device, or a FILE_LIST/OPEN/READ client on the host | confirmed byte-for-byte on SDX24, SDX55, SDX62, SDX65 and SDX72 modules; some builds refuse (they announce size 0) |
| Vendor text `.qsr4` | some vendors publish `msg_hash_<guid>.qsr4` alongside firmware | same table, uncompressed |
| Vendor tool | [Quectel QLog](https://github.com/quectel-official/QLog) pulls the database from a live Quectel modem over DIAG | Apache-2.0, but its NOTICE restricts use to Quectel customers |

**Builds known to ship no database at all:** several Sierra Wireless SWIX55C/SWIX65C
builds (the firmware's strings name the path, but the file isn't there), one
RM520N-GL release train (none in 15 packages, and the DIAG transfer is refused),
and a Quectel RG520N-based CPE build whose packaging step for it is disabled.
For those builds `0x99` and `0x9D` stay structure-only — token identity plus
arguments — unless a matching database turns up.

---

## 8. Log-record payloads — versions, and the "it's encrypted" trap

### 8.1 Payload versions

A `0x10` log record is `log_code | ts64 | payload`. For most log codes **byte 0
of the payload is a version number, and different versions are different
layouts** — not extensions of one layout. A parser that insists on one version
"fails" on most modern firmware, and loosening the check is *worse*: it decodes
garbage. The fix is to dispatch on the version.

Three worked examples; diaggrok's parsers for each (`diag_0xb114.py`,
`diag_0xb063.py`, `diag_0xb064.py`) document every version's layout in their
docstrings:

| Code | Versions seen | Notes |
|---|---|---|
| `0xB114` LTE LL1 serving-cell frame timing | `0x01` (MDM9x07/9x15), `0x2B` (MDM9x30/35), `0x65` (MDM9250), `0x7A` (SDX20), `0x8D` (SDX24), `0xA1` (SDX55+) | 12- or 16-byte header; record size 3, 28, 44, 8, 48 or 48 bytes; `0xA1` moves the frame/subframe number into every record. Across all our captures only about 5% of records are version `0x01`. |
| `0xB063` LTE MAC downlink transport block | `0x01` (MDM9x00–SDX24), `0x31` (SDX55), `0x32` (SDX55/SDX6x) | `0x01` is the classic MAC subpacket container (subpacket id `0x07`); `0x31` and `0x32` are a flat list of transport-block entries, and `0x31` puts a 532-byte cell-statistics block in front of the entries. |
| `0xB064` LTE MAC uplink transport block | `0x01` only (subpacket versions 1/2/3 inside) | subpacket id `0x08`, in the same container as `0xB063` version `0x01`. |

The `0xB061`–`0xB064` MAC family shares an outer container,
`u8 version | u8 num_subpackets | 2 B | subpackets`, each subpacket starting
`u8 id | u8 ver | u16 size`. Id `0x06` is a RACH attempt, `0x07` a downlink
transport block, `0x08` an uplink one. A parser that decodes only `0x06` and
keeps the others as raw bytes has the framing *right* and the body simply
undecoded — that is not a mis-parse.

### 8.2 "Encrypted NAS" — usually not

LTE NAS messages appear on several log codes, and those codes contain different
things:

| Code(s) | Contents |
|---|---|
| `0xB0E2` / `0xB0E3` (ESM in/out), `0xB0EC` / `0xB0ED` (EMM in/out) — the "plain" codes | the NAS message **after** the modem has removed the security header and deciphered it. The first octet's security-header nibble is `0` (plain) or `12` (service request) — never a ciphered container. |
| `0xB0E1` (ESM, security-protected, outgoing) | the protected outer message, logged in the same tick as its plain `0xB0E3` twin and exactly 6 octets longer (security header, MAC, sequence number). Ciphered unless the network selected the null cipher EEA0. |
| `0xB0E0` | not seen on any build measured. |

So **ciphertext should not turn up on the plain codes**. If a tool reports
"encrypted NAS", ask for the log code and the first octet of the NAS message:

- on `0xB0E2/E3/EC/ED` it is almost certainly a service request (`0xC_`) or an
  integrity-protected-only message that was misclassified;
- on `0xB0E1` it is real ciphertext — and its plaintext twin is on `0xB0E3`;
- if what was flagged is a whole DIAG frame rather than a NAS message, it is
  probably `0x9E` (§5.4).

diaggrok's NAS parsers (`diag_0xb0ec.py`, `diag_0xb0e1.py` and their shared
helpers) document these rules, and the firmware's own F3 prints corroborate them
— the modem logs the security-header type of each message it processes.

---

## 9. Chipset-generation matrix

Which formats appear in captures from each family (CRC-checked where possible;
`—` = not seen). Families group modules by modem platform; vendors differ within
a family.

| Family | Example modules | `0x98` | `0x79` | `0x92` | `0x99` | `0x9D` | `0x9E` | `0x80` qdb transfer | `0x60` |
|---|---|---|---|---|---|---|---|---|---|
| MDM9x07 (9207/9607) | Quectel EG25-G, EG95; SIMCom SIM7600; Orbic RC400L | — | ✓ | ✓ | — | — | — | refuses | ✓ |
| MDM9x30/35/40 | Sierra Wireless EM7455/MC7455, AC791L; Quectel EP06 | rare | ✓ | ✓ | — | — | — | — | ✓ |
| MDM9x50 / SDX20 | Sierra Wireless EM7565/EM7511/MC7411; Telit LM960; Quectel EG12/EG18 | ✓ (layer 2 only) | ✓ | — | ✓ | — | — | — | ✓ |
| MDM9150 / MDM9250 (automotive, C-V2X) | WNC and Kapsch units | MDM9150 partly | ✓ | — | ✓ | — | — | — | rare |
| SDX24 | Quectel EM120R/EM160R | ✓ | ✓ | — | ✓ | ✓ | — | ✓ | ✓ |
| SDX55 | Quectel RM500Q; Sierra Wireless EM9190; Telit FN980; Foxconn T99W175; Inseego M2000/M2100 | ✓ | ✓ | — | ✓ | ✓ | — | ✓ | ✓ |
| SDX62 | Quectel RM520N-GL/RG520N; Sierra Wireless EM9291; Orbic R562L5 | ✓ | ✓ | — | ✓ | ✓ | ✓ | ✓ (some refuse) | ✓ |
| SDX65 | Inseego M3100; Foxconn T99W373 | ✓ (mostly wrapped) | ✓ | — | ✓ | ✓ | — | ✓ | ✓ |
| SDX72 / SDX75 | Foxconn T99W640; Quectel RG650V | ✓ | ✓ | — | ✓ | ✓ | ✓ | ✓ | ✓ |

Reading the matrix:

- **`0x92` → `0x99` is the generational boundary** for debug prints: legacy
  QShrink up to the MDM9x40 era, QShrink 4 from MDM9x50/SDX20 on. Plain-text
  `0x79` coexists with both.
- **`0x9D` starts at SDX24; `0x9E` at SDX62** (absent on SDX55 and on the one
  SDX65 model measured).
- **Whether `/dev/diag` exists is the vendor's kernel choice**, not the
  chipset's. SDX55 hotspots typically have it; several SDX62/SDX65 CPEs and
  hotspots (including one Orbic SDX62 unit) do not, and serve DIAG only through
  the userspace router — pick diagbarf's `diag-dev` or `socket-log` mode
  accordingly.
- **`0x60` presence reflects arming**, not capability: most host tools never
  switch events on.

---

## 10. Oracle catalogue — what each one answers and where to get it

An "oracle" here is any independent source of truth used to decode a field or to
check a decode. None is authoritative on its own: when an oracle and a parser
disagree, that is something to investigate, not a verdict.

### 10.1 Databases and tables

| Oracle | Unlocks | Applies to | Where to get it |
|---|---|---|---|
| `qdsp6m.qdb` (exact build) | `0x99` text, `0x9D`/`0x1FE8` text | MDM9x50/SDX20 → SDX75 | §7.3 |
| Legacy QShrink index | `0x92` → source file, line, argument count | MDM9x07 → MDM9x40 | compiled into the modem firmware image; extracting it is beyond this guide |
| Event-id names | `0x60` event names | all | `event_defs.h`-style headers in public Android vendor source trees; community item lists |
| Log-code names | log code → name | all | `log_codes.h`-style headers; [osmo-qcdiag](https://gitea.osmocom.org/phone-side/osmo-qcdiag) headers; community lists |
| Opcode and subsystem constants | the command plane | all | [linux-msm/diag](https://github.com/linux-msm/diag) (BSD-3), [osmo-qcdiag](https://gitea.osmocom.org/phone-side/osmo-qcdiag), [QCSuper's protocol notes](https://github.com/P1sec/QCSuper/blob/master/docs/The%20Diag%20protocol.md) |
| `0x9E` key | secure-log body | SDX62+ | not available |

### 10.2 Other decoders — run them and compare

Running a second, independently written decoder over the same capture is the
cheapest check there is. Treat these as **output-only** references: several are
GPL, so if you are writing a permissively licensed decoder (diaggrok is
Apache-2.0), compare outputs rather than reading or porting their source.

| Tool | Gives you | Strong on | Weak on | Get it |
|---|---|---|---|---|
| [QCSuper](https://github.com/P1sec/QCSuper) (GPL-3.0) | DIAG → GSMTAP pcap for Wireshark | LTE RRC (`0xB0C0`), NAS (`0xB0E2/E3/EC/ED`), IP traffic (`0x11EB`), 2G/3G signalling | ~13 field-decoded codes; NR RRC lands as a GSMTAP type stock Wireshark won't dissect; nothing for vendor-internal codes; produces nothing on compressed input | `pip install qcsuper` |
| [SCAT](https://github.com/fgsect/scat) (GPL-2.0+) | text and GSMTAP; layer 2 (PDCP/RLC/MAC) with `-L` | the widest field coverage (~80 codes, including ML1 measurements, NR beams, MAC `0xB061–B064`, LTE MIB); renders QShrink 4 given a qdb | frames few records on some SDX20/SDX55 captures, so treat its counts as a floor; slow | `pip install signalcat` (**not** the unrelated `scat` package) |
| [rayhunter](https://github.com/EFForg/rayhunter) `rayhunter-check` (GPL-3.0) | QMDL → GSMTAP (RRC + NAS), plus IMSI-catcher heuristics | RRC/NAS on hotspot captures | parses MAC/ML1/LL1 internally without exposing fields; picks its reader by file extension (`.qmdl`); weak on `0x98`-heavy captures | build from source (`cargo build --bin rayhunter-check`) |
| [MobileInsight](https://github.com/mobile-insight/mobileinsight-core) | Python analyzers, ~117 codes | broad LTE/NR coverage | involved build; not on PyPI | from source |
| [uecapabilityparser](https://github.com/handymenny/uecapabilityparser) (MIT) | UE-capability and carrier-aggregation combination tables (the engine behind smartphonecombo.it) | OTA `UECapabilityInformation` hex from `0xB0C0`/`0xB821` (`-t H`) | its capture-file modes depend on SCAT's pcap output | GitHub releases (needs Java) |
| [Quectel QLog](https://github.com/quectel-official/QLog) | vendor capture plus QShrink 4 rendering | Quectel modems, live | needs the modem attached; licence restricted to Quectel customers | GitHub |
| [diagmunge](https://github.com/lukejenkins/diagmunge) `dlf_to_pcap` (Apache-2.0) | DIAG → GSMTAP pcap, with NR signalling as Wireshark Exported-PDU | a permissively licensed second opinion next to QCSuper's pcap | signalling codes only | `pip install "diagmunge[munge] @ git+https://github.com/lukejenkins/diagmunge@main"` |
| [osmo-qcdiag](https://gitea.osmocom.org/phone-side/osmo-qcdiag), [diag-parser](https://github.com/moiji-mobile/diag-parser), [SnoopSnitch](https://github.com/srlabs/snoopsnitch), [libqcdm](https://gitlab.freedesktop.org/mobile-broadband/ModemManager) | older community decoders and constant tables | 2G/3G, the command plane | little LTE/NR field coverage | upstream repositories |

### 10.3 Truth already inside the capture

| Oracle | Answers | Caveats |
|---|---|---|
| **F3 debug prints** (`0x79`, rendered `0x99`/`0x92`, and F3 inside `0x98`) | the firmware authors' own label, unit and scaling for a quantity (`"rsrp = %d"`), printed alongside the binary record | needs the right database for `0x99`; must be armed; builds differ in what they print, so check every F3 format and more than one capture |
| **`0x60` events** | procedure milestones and small payloads (an EMM message type, an attach-reject cause) | rarely armed; public names only up to about id 2670 |
| **RRC/NAS over-the-air messages** (`0xB0C0`, `0xB821`, `0xB0Ex`) decoded against 3GPP ASN.1 | ground truth for configuration fields that other codes copy (RACH parameters against SIB2; Msg3 against the uplink CCCH message) | a capture of a modem sitting idle carries little signalling — capture across handovers, attaches and other transitions |
| **Clock regression** | whether a counter is a radio frame (10 ms), subframe (1 ms) or slot (0.5 ms at 30 kHz) | needs many records |
| **Accounting identities** | e.g. per-subframe timing adjustments that must add up to the change reported in the next record | only where such an identity exists |

### 10.4 External references and truth sources

| Oracle | Answers | Where to get it |
|---|---|---|
| 3GPP ASN.1 (TS 36.331 LTE RRC, 38.331 NR RRC, 24.301/24.501 NAS) | message structure for over-the-air decodes | [3gpp.org](https://www.3gpp.org/specifications) (the ASN.1 is embedded in the specification text) |
| Wireshark / tshark with GSMTAP | an independent dissection of any pcap a decoder emits | [wireshark.org](https://www.wireshark.org/); GSMTAP header definitions in [libosmocore](https://gitea.osmocom.org/osmocom/libosmocore) |
| [pycrate](https://github.com/pycrate-org/pycrate) (LGPL-2.1+) | reference ASN.1 and NAS decoding | PyPI |
| GNSS reference receiver with RTCM MSM7 | truth for GNSS measurement and position codes | any RTK-capable receiver; [RTKLIB](https://github.com/rtklibexplorer/RTKLIB), [pyrtcm](https://github.com/semuconsulting/pyrtcm), gpsd |
| AT / QMI / MBIM polling alongside DIAG | the vendor's own summary values (RSRP, PCI, band…) to line up against raw fields | the modem itself; AT values are usually filtered at layer 3, so expect small offsets |
| Cell databases | (PCI, EARFCN) → (MCC, MNC, cell id, TAC) sanity checks | [OpenCelliD](https://opencellid.org/), [WiGLE](https://wigle.net/) |

---

## 11. Triage checklist: "this frame won't decode"

Work down the layers before concluding that anything is encrypted or
proprietary.

1. **Container.** Is it really HDLC? Find a CRC-valid frame. Look for a QMDL2
   prologue (a CRC-valid frame at the offset named by the leading `u32`). Ignore
   the file extension. (`diaggrok.dlf.detect_format` does all of this.)
2. **Transport damage.** CRC failures concentrated on long frames → the MHI
   8 KiB replay seam (§3.2). Gaps in UDP captures → check the relay's sequence
   numbers. F3 or `0x9D` missing entirely → a lossy relay or a DLF conversion.
3. **Opcodes.** Count opcodes on CRC-valid frames only — counts taken from raw
   bytes overstate rare opcodes by orders of magnitude. Any `0x13` tells you
   what the modem refused.
4. **Wrapper.** Unwrap `0x98` and count again. Don't unwrap `0x80`.
5. **Message format.** `0x99`/`0x9D` → you need the exact build's database (get
   its GUID from `0x80`, a QMDL2 prologue or the build string). `0x92` → you get
   file and line identity, not text. `0x9E` → stop: header only.
6. **Log payload.** Read byte 0 as a version and look it up. An unknown version
   is a new layout, not a corrupt record — a parser that rejects it is behaving
   correctly.
7. **"Encrypted"?** Ask for the log code and the first octet (§8.2). On the
   plain NAS codes it is almost never ciphertext. A whole frame reported as
   "encrypted" is usually `0x9E`.
8. **Check the answer against an oracle** from §10 — ideally two that don't
   share a code path, such as an F3 print *and* an over-the-air message.

---

## 12. References

**Tools from this effort** (Apache-2.0)

- [diaggulp](https://github.com/lukejenkins/diaggulp) — host-side raw HDLC
  capture over serial, TCP, UDP or pcap, with F3 arming and live GSMTAP output.
- [diagbarf](https://github.com/lukejenkins/diagbarf) — on-device DIAG fan-out
  relay (`diag-dev`, `socket-log` and `hdlc-chardev` sources; TCP raw HDLC and
  UDP `DGE1` sinks).
- [diagmunge](https://github.com/lukejenkins/diagmunge) — transport client,
  `DGE1` resequencing, HDLC → DLF, DIAG → GSMTAP pcap.
- [diaggrok](https://github.com/lukejenkins/diaggrok) — container detection, the
  HDLC walker with `0x98` unwrapping, and per-log-code parsers with documented
  per-version layouts.

**Other open-source implementations and documentation**

- [linux-msm/diag](https://github.com/linux-msm/diag) — Qualcomm's open-source
  userspace DIAG router (BSD-3): transport, HDLC/NHDLC framing, control channel,
  QRTR. It routes DIAG; it does not decode payloads.
- [QCSuper](https://github.com/P1sec/QCSuper) and its
  [DIAG protocol write-up](https://github.com/P1sec/QCSuper/blob/master/docs/The%20Diag%20protocol.md).
- [SCAT](https://github.com/fgsect/scat) — Signalling Collection and Analysis Tool.
- [rayhunter](https://github.com/EFForg/rayhunter) — EFF's on-device DIAG
  analyzer; a readable reference for `/dev/diag` ioctls and read framing.
- [MobileInsight](https://github.com/mobile-insight/mobileinsight-core).
- [osmo-qcdiag](https://gitea.osmocom.org/phone-side/osmo-qcdiag) and the
  [Osmocom Quectel DIAG wiki](https://osmocom.org/projects/quectel-modems/wiki/Diag).
- [ModemManager libqcdm](https://gitlab.freedesktop.org/mobile-broadband/ModemManager).
- [Quectel QLog](https://github.com/quectel-official/QLog).
- [uecapabilityparser](https://github.com/handymenny/uecapabilityparser).
- [ubi_reader](https://github.com/onekey-sec/ubi_reader) — for unpacking the
  UBI volumes in firmware update packages.

**Specifications**

- 3GPP TS 36.331 (LTE RRC), 38.331 (NR RRC), 24.301 (LTE NAS), 24.501 (5G NAS),
  33.401 (LTE security) — [3gpp.org](https://www.3gpp.org/specifications).
- RFC 1662 (PPP in HDLC-like framing) — the byte-stuffing DIAG borrows.
