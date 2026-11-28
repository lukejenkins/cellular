# Qualcomm DIAG framing diagrams

Byte-layout diagrams for the Qualcomm DIAG containers and opcodes: the HDLC
byte stream, a single frame, the capture-file formats, and the opcode
envelopes that wrap log records, events and F3 debug messages. They go with
[`diag-encapsulations-and-oracles.md`](../diag-encapsulations-and-oracles.md),
whose sections link to the diagram that applies.

Each SVG is generated from a Kaitai Struct (`.ksy`) layout. The layouts are
tested against a working DIAG decoder, and the diagrams are drawn from them
by a deterministic renderer, so the same layout gives the same bytes on any
machine. Please don't edit the SVGs by hand.

## Reading a diagram

Each box is one type. Its columns are:

| column | meaning |
|---|---|
| `pos` | byte offset within the type. `N.b` is byte `N`, bit `b`. `…` means the offset depends on earlier data |
| `size` | bytes (`Nb` = bits). `to end` = the rest of the enclosing buffer. `N × expr` = a repeated field |
| `type` | `u1`/`u2le`/`u4le`/… integers, `bytes`, `strz` (NUL-terminated), a nested type, or `switch on …` |
| `id` | the field name |
| `when` | the condition, when the field is optional |

`switch` rows list their cases, and an arrow from each case points to the box
for its body type. `instances` are derived values, not bytes on the wire.
Purple boxes are enums.

## Index

| diagram | layout |
|---|---|
| [diag_hdlc_stream](diag_hdlc_stream.svg) | raw HDLC-framed byte stream (frame boundaries only) |
| [diag_hdlc_frame](diag_hdlc_frame.svg) | one unescaped, CRC-bearing frame (opcode dispatch) |
| [diag_dlf_file](diag_dlf_file.svg) | flat DLF capture file (deframed log records) |
| [diag_qmdl2_file](diag_qmdl2_file.svg) | QMDL2 capture (QSHRINK4 binding header + HDLC stream) |
| [diag_log_f](diag_log_f.svg) | 0x10 DIAG_LOG_F frame (binary log record carrier) |
| [diag_multi_radio_0x98](diag_multi_radio_0x98.svg) | 0x98 multi-radio routing wrapper |
| [diag_subsys_v2_0x80](diag_subsys_v2_0x80.svg) | 0x80 SUBSYS_CMD_VER_2_F (DIAG_SERV file-read chunk) |
| [diag_secure_log_0x9e](diag_secure_log_0x9e.svg) | 0x9E SECURE_LOG_F (cleartext envelope, keyed body) |
| [diag_event_report_0x60](diag_event_report_0x60.svg) | 0x60 EVENT_REPORT_F (the event stream) |
| [diag_ext_msg_0x79](diag_ext_msg_0x79.svg) | 0x79 EXT_MSG_F (plaintext F3 debug message) |
| [diag_qsr_terse_0x92](diag_qsr_terse_0x92.svg) | 0x92 QSR_EXT_MSG_TERSE_F (legacy hashed F3) |
| [diag_qsr4_terse_0x99](diag_qsr4_terse_0x99.svg) | 0x99 QSR4_EXT_MSG_TERSE_F (hashed F3) |
| [diag_qsh_trace_0x9d](diag_qsh_trace_0x9d.svg) | 0x9D QSH_TRACE_PAYLOAD_F (Snapdragon Health telemetry) |
| [diag_log_config_0x73](diag_log_config_0x73.svg) | 0x73 LOG_CONFIG_F (the log-mask arming handshake) |
| [diag_ext_msg_config_0x7d](diag_ext_msg_config_0x7d.svg) | 0x7D EXT_MSG_CONFIG_F (the F3 runtime-mask arming) |
| [diag_verno_0x00](diag_verno_0x00.svg) | 0x00 VERNO_F response (the anchor probe) |
| [diag_ver_0x1c](diag_ver_0x1c.svg) | 0x1C DIAG_VER_F response (the protocol version) |
| [diag_ts_0x1d](diag_ts_0x1d.svg) | 0x1D TS_F response (the modem timebase) |
| [diag_ext_build_id_0x7c](diag_ext_build_id_0x7c.svg) | 0x7C EXT_BUILD_ID_F response (the firmware id) |
| [diag_bad_cmd_0x13](diag_bad_cmd_0x13.svg) | 0x13 BAD_CMD_F (rejection, echoing the request) |
