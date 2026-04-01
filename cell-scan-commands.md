# Cellular Modem Scan Commands Reference

Copyright 2026 Luke Jenkins. Licensed under
[CC-BY-SA-4.0](https://creativecommons.org/licenses/by-sa/4.0/).

Source: <https://github.com/lukejenkins/cellular>

## Purpose

USB cellular modems can report detailed information about visible cell towers
through vendor-specific AT commands, but each vendor uses different commands,
different response formats, and returns different data fields. This document
provides a cross-vendor comparison so you can answer:

- **What commands do I run on this modem?** — see the data tables and scan
  strategy recommendations
- **What data will I get back?** — see the per-command field matrices
- **Can I submit this to WiGLE?** — see the WiGLE Complete column
- **What does this field mean?** — see the terminology glossary

The document is organized by observation speed: **fast commands** return in
under a second and are suitable for continuous polling, while **slow commands**
perform RF band scans that take 30 seconds to several minutes. Vendors
covered: Fibocom, Quectel, Sierra Wireless, SIMCom, and Telit.

## Column Legend

| Symbol | Meaning |
|--------|---------|
| Y | Field is directly returned |
| D | Field is derivable from other returned fields |
| - | Field is not available |
| W | Column header suffix — field is required for WiGLE cell tower submissions |

**WiGLE Complete** column: whether a single command returns all fields needed
for a [WiGLE](https://wigle.net) cell tower CSV submission.
- **Y** = all WiGLE-required fields returned by this command alone
- **P** = partial — needs a second fast command on the same modem to fill gaps
- **N** = missing WiGLE-required fields that cannot be obtained from a companion command

Columns marked with **W** in the header (e.g., `MCC W`) are the specific
fields WiGLE requires: RAT, MCC, MNC, CID, TAC, EARFCN, and RSRP. GPS
position and timestamp are also required but come from the GPS source, not
the modem.
| ? | Untested / unverified |

---

## Fast Serving Cell Commands

These commands query the modem's current serving cell and return immediately
(sub-second). Suitable for continuous polling at 2-5 second intervals.

| Vendor | Model | Command | WiGLE Complete | RAT W | MCC W | MNC W | CID W | TAC W | PCI | EARFCN W | Band | RSRP W | RSRQ | RSSI | SINR | DL BW | UL BW | Duplex | Operator | DRX | TX Pwr | srxlev | CQI | MIMO | CA | Source |
|--------|-------|---------|:-----:|:---:|:---:|:---:|:---:|:---:|:---:|:------:|:----:|:----:|:----:|:----:|:----:|:-----:|:-----:|:------:|:--------:|:---:|:------:|:------:|:---:|:----:|:--:|--------|
| Fibocom | L850-GL/L860-GL | `AT+XCELLINFO?` | Y | Y | Y | Y | Y | Y | Y | Y | - | Y | Y | - | - | - | - | - | - | - | - | - | - | - | - | V2.0.2 §10.1.12 |
| Quectel | BG95-M3/BG77/BG600L | `AT+QCSQ` | N | Y | - | - | - | - | - | - | - | Y | Y | Y | Y | - | - | - | - | - | - | - | - | - | - | V2.0 §4.7 |
| Quectel | EC2x/EG2x/EG9x/EM05/EP06 | `AT+QENG="servingcell"` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | Y | - | - | - | V2.1 §6.18 |
| Quectel | EG12/EM12/EG18 | `AT+QENG="servingcell"` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | Y | Y | - | - | V1.1 §6.18 |
| Quectel | RG520N/RM520N/RM530N (LTE) | `AT+QENG="servingcell"` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | Y | Y | - | - | V1.1 §5 |
| Quectel | RG520N/RM520N/RM530N (NR5G-SA) | `AT+QENG="servingcell"` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | Y | Y | - | Y | - | - | - | Y | - | - | - | V1.1 §5 |
| Quectel | RG520N/RM520N/RM530N (ENDC) | `AT+QENG="servingcell"` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | Y | Y | - | - | V1.1 §5 |
| Quectel | RM500Q/RM502Q (LTE) | `AT+QENG="servingcell"` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | Y | - | - | - | V1.2 §5 |
| Quectel | RM500Q/RM502Q (NR5G-SA) | `AT+QENG="servingcell"` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | Y | Y | - | Y | - | - | - | Y | - | - | - | V1.2 §5 |
| Sierra | EM74xx/MC74xx/EM7511/MC7411 | `AT!GSTATUS?` | P | Y | - | - | Y | Y | - | D | Y | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | Y | r4 |
| Sierra | EM91 (EM9190/EM9191/EM7690) | `AT!GSTATUS?` | P | Y | - | - | Y | Y | - | D | Y | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | Y | r14 |
| Sierra | EM91 (EM9190/EM9191/EM7690) | `AT!NRINFO?` | P | Y | Y | Y | Y | - | - | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | Y | - | - | Y | - | r14 |
| Sierra | EM92 (EM9291/EM9293) | `AT!GSTATUS?` | P | Y | - | - | Y | Y | - | D | Y | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | Y | r14 |
| Sierra | EM92 (EM9291/EM9293) | `AT!NRINFO?` | P | Y | - | - | Y | - | - | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | Y | - | - | Y | - | r14 |
| SIMCom | SIM7070/SIM7080/SIM7090 | `AT+CPSI?` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | V1.08 §4.2.14 |
| SIMCom | SIM7500/SIM7600 | `AT+CPSI?` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | V3.00 §4.2.14 |
| SIMCom | SIM82XX/SIM83XX (LTE) | `AT+CPSI?` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | V1.02 §4 |
| SIMCom | SIM82XX/SIM83XX (NR5G-SA) | `AT+CPSI?` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | Y | - | - | - | - | - | - | - | - | - | - | V1.02 §4 |
| SIMCom | SIM82XX/SIM83XX (ENDC) | `AT+CPSI?` | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | V1.02 §4 |
| Telit | FN980m (ENDC) | `AT#RFSTS` | Y | Y | Y | Y | Y | Y | - | Y | Y | Y | Y | Y | - | - | - | Y | Y | Y | Y | - | - | - | - | Rev.5 |
| Telit | FN980m (ENDC) | `AT#SERVINFO` | Y | Y | Y | Y | Y | Y | - | Y | - | Y | Y | Y | - | - | - | - | Y | Y | - | - | - | - | - | Rev.5 |
| Telit | FN980m (LTE) | `AT#RFSTS` | Y | Y | Y | Y | Y | Y | - | Y | Y | Y | Y | Y | - | - | - | Y | Y | Y | Y | - | - | - | - | Rev.5 |
| Telit | FN980m (LTE) | `AT#SERVINFO` | Y | Y | Y | Y | Y | Y | - | Y | - | Y | - | Y | - | - | - | - | Y | Y | - | - | - | - | - | Rev.5 |
| Telit | FN980m | `AT#LTEDS` | P | - | Y | Y | - | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | - | - | Y | Y | - | Y | - | - | Rev.5 |
| Telit | LM960 | `AT#RFSTS` | Y | Y | Y | Y | Y | Y | - | Y | Y | Y | Y | Y | - | - | - | Y | Y | Y | Y | - | - | - | - | Rev.8 §5.6.1.26 |
| Telit | LM960 | `AT#SERVINFO` | Y | Y | Y | Y | Y | Y | Y | Y | - | Y | - | Y | - | - | - | - | Y | Y | - | - | - | - | - | Rev.8 §5.6.1.27 |

**Notes:**
- Telit LM960 `AT#RFSTS` — band field is `ABND` (Active Band, 1-63 per 3GPP TS 36.101). Value 255 on some firmware means "not available."
- Telit FN980m `AT#RFSTS` (ENDC) — appends NR fields: NR channel, NR RSRP, NR RSSI, NR RSRQ, NR band, NR BW, NR TX power.
- Telit FN980m `AT#SERVINFO` (ENDC) — appends: NR channel, NR RSSI, NR RSRP, NR RSRQ. NR SA mode returns only 5 NR-specific fields.
- Telit FN980m `AT#LTEDS` — returns 26 fields total including MCS, modulation, BLER — the most detailed single-command status on the FN980m.
- Sierra EM74xx/MC74xx `AT!GSTATUS?` — also reports carrier aggregation SCC1-4 with per-SCC band, RSSI, and RSRP.
- Sierra EM91 `AT!NRINFO?` — reports MCC-MNC in 5G SA mode only. Reports per-antenna RSSI (RxM, RxD, RxM1, RxD1 for sub-6; Rx0, Rx1 for mmW). Cell ID is PCI (0-1007).
- Sierra EM92 `AT!NRINFO?` — Cell ID is 64-bit Global Cell ID (hex/decimal), not PCI. Does not report MCC-MNC. Otherwise same format as EM91.
- Sierra EM91/EM92 `AT!LTEINFO?` — r14 adds CA SCell section (EARFCN, PCI, band, MIMO layers, RSRP, RSSI, SINR) and WCDMA neighbor reporting.
- Sierra EM91/EM92 `AT!NRPCI` — returns 5G NR Physical Cell IDs for PCC + all SCCs (added in r14 Rev.10).
- Quectel RM500Q/RG520N `AT+QENG="servingcell"` — response format varies by RAT (LTE vs NR5G-SA vs NR5G-NSA each have different field layouts).
- Quectel EG12/EM12/EG18 `AT+QENG="servingcell"` — adds CQI and TX power fields compared to the EC2x family.
- Quectel RG520N/RM520N/RM530N `AT+QENG="servingcell"` (ENDC) — returns two `+QENG:` blocks: LTE serving cell followed by NR5G-NSA secondary cell. RM530N-GL adds mmW (FR2) band support but uses the same AT command interface as the sub-6 models.
- Quectel BG95-M3/BG77/BG600L `AT+QCSQ` — NB-IoT/Cat-M modules. Returns signal metrics only (RSSI, RSRP, SINR, RSRQ). No cell identity, no EARFCN. **Not WiGLE-capable** — these LPWA modules lack `AT+QENG` entirely.
- SIMCom SIM7080G `AT+CPSI?` — NB-IoT/Cat-M module. Returns full serving cell identity including TAC, CellID, PCI, EARFCN, RSRP. WiGLE-complete despite being an LPWA module.
- SIMCom SIM82XX/SIM83XX `AT+CPSI?` — 5G module (SIM8260, SIM8262, SIM8230, etc.). In ENDC mode returns two `+CPSI:` lines: LTE anchor + NR5G-NSA secondary. NR5G-SA format omits RSSI, DL/UL BW. No neighbor cell command is available on this family.
- eNB ID and Sector ID are derivable from CID on all modems: `eNB_ID = CID >> 8`, `Sector_ID = CID & 0xFF`.

---

## Fast Neighbor Cell Commands

These commands return cells the modem can detect but is not camped on.
Sub-second response time.

| Vendor | Model | Command | WiGLE Complete | EARFCN W | PCI | RSRP W | RSRQ | RSSI | SINR | MCC W | MNC W | CID W | TAC W | Band | Pathloss | CQI | Reselection Priority | Source |
|--------|-------|---------|:-----:|:------:|:---:|:----:|:----:|:----:|:----:|:---:|:---:|:---:|:---:|:----:|:--------:|:---:|:--------------------:|--------|
| Fibocom | L850-GL/L860-GL | `AT+XCELLINFO?` (neighbor) | N | Y | Y | Y | Y | - | - | - | - | - | - | - | - | - | - | V2.0.2 §10.1.12 |
| Fibocom | L850-GL/L860-GL | `AT+XMCI=1` (neighbor) | Y | Y | Y | Y | Y | - | Y | Y | Y | Y | Y | D | Y | Y | - | V2.0.2 §10.1.24 |
| Quectel | EC2x/EG2x/EG9x/EM05/EP06 | `AT+QENG="neighbourcell"` | N | Y | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | V2.1 §6.18 |
| Quectel | EG12/EM12/EG18 | `AT+QENG="neighbourcell"` | N | Y | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | V1.1 §6.18 |
| Quectel | RG520N/RM520N/RM530N | `AT+QENG="neighbourcell"` | N | Y | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | V1.1 §5 |
| Quectel | RM500Q/RM502Q | `AT+QENG="neighbourcell"` | N | Y | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | V1.2 §5 |
| Sierra | EM74xx/MC74xx/EM7511/MC7411 | `AT!LTEINFO?` (intra-freq) | Y | Y | Y | Y | Y | Y | - | Y | Y | Y | Y | Y | - | - | Y | r4 |
| Sierra | EM74xx/MC74xx/EM7511/MC7411 | `AT!LTEINFO?` (inter-freq) | Y | Y | Y | Y | Y | Y | - | Y | Y | Y | Y | Y | - | - | Y | r4 |
| Sierra | EM91/EM92 | `AT!LTEINFO?` (intra-freq) | Y | Y | Y | Y | Y | Y | - | Y | Y | Y | Y | Y | - | - | Y | r14 |
| Sierra | EM91/EM92 | `AT!LTEINFO?` (inter-freq) | Y | Y | Y | Y | Y | Y | - | Y | Y | Y | Y | Y | - | - | Y | r14 |
| SIMCom | SIM7070/SIM7080/SIM7090 | `AT+CENG` (neighbor) | N | Y | Y | Y | Y | - | Y | - | - | - | - | - | - | - | - | V1.08 §4 |
| Telit | LM960 | `AT#MONI` (mode 1, intra-freq) | N | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | - | Rev.8 §5.6.1.24 |
| Telit | LM960 | `AT#MONI` (mode 2, inter-freq) | N | Y | Y | Y | Y | Y | - | - | - | - | - | - | - | - | - | Rev.8 §5.6.1.24 |

**Notes:**
- Quectel EC2x/EG2x/EG9x/EM05 `AT+QENG="neighbourcell"` — returns empty when the modem is in limited service or not registered.
- Telit LM960 `AT#MONI` — requires setting the mode first (`AT#MONI=1`, then `AT#MONI` to query). `Id` field is PCI in hex for neighbor cells.
- Telit LM960 `AT#MONI=7` — returns all cell sets in a single table-formatted response (serving + all neighbors).
- Telit LM960 `AT#MONI` modes 3 and 4 — return WCDMA and GSM inter-RAT neighbors respectively.
- Sierra EM74xx/MC74xx `AT!LTEINFO?` — the most complete neighbor command; returns full identity (MCC/MNC/CID/TAC) for all neighbor types including inter-RAT (GSM, WCDMA, CDMA).
- Fibocom L850-GL/L860-GL `AT+XMCI=1` — differentiates cell types by a TYPE field: 4=LTE serving, 5=LTE neighbor.
- SIMCom SIM7080G `AT+CENG` — engineering mode neighbor reporting. Enable with `AT+CENG=1,1`. Neighbor entries return EARFCN, PCI, RSRP, RSSI, RSRQ, SINR but lack MCC/MNC/CID/TAC.

---

## Fast Signal-Only and Identity-Only Commands

These return signal metrics or cell identity without full serving cell detail.
Not sufficient for cell tower tracking on their own.

### Signal Quality

| Vendor | Model | Command | RAT | RSSI | RSRP | RSRQ | SINR | RSCP | Ec/Io | RSSNR | Source |
|--------|-------|---------|:---:|:----:|:----:|:----:|:----:|:----:|:-----:|:-----:|--------|
| Fibocom | L850-GL/L860-GL | `AT+XCESQ?` | - | - | Y | Y | - | Y | Y | Y | V2.0.2 |
| Quectel | EC2x/EG2x/EG9x/EM05 | `AT+QCSQ` | Y | Y | Y | Y | Y | - | - | - | V2.1 §6.4 |
| Quectel | EC2x/EG2x/EG9x/EM05 (WCDMA) | `AT+QCSQ` | Y | Y | - | - | - | Y | Y | - | V2.1 §6.4 |

### Network Information

| Vendor | Model | Command | RAT | Operator | Band | Channel | Source |
|--------|-------|---------|:---:|:--------:|:----:|:-------:|--------|
| Quectel | EC2x/EG2x/EG9x/EM05 | `AT+QNWINFO` | Y | Y | Y | Y | V2.1 §6 |
| Quectel | RM500Q/RM502Q | `AT+QNWINFO` | Y | Y | Y | Y | V1.2 §5 |

### Cell Identity

| Vendor | Model | Command | RAT | MCC | MNC | NCI | gNodeB ID | PCI | Source |
|--------|-------|---------|:---:|:---:|:---:|:---:|:---------:|:---:|--------|
| Quectel | RM500Q/RM502Q | `AT+QNWCFG="nr5g_cell_id"` | NR | Y | Y | Y | Y | Y | V1.2 §5 |

**Note:** Quectel RM500Q/RM502Q `AT+QNWCFG="nr5g_cell_id"` — NR only. Returns composite NCGI (MCC+MNC+NCI); NCI contains gNodeB_ID + Cell ID.

### Carrier Aggregation

| Vendor | Model | Command | Band | PCI | EARFCN | DL BW | UL BW | MIMO | RSRP | Source |
|--------|-------|---------|:----:|:---:|:------:|:-----:|:-----:|:----:|:----:|--------|
| Fibocom | L850-GL/L860-GL | `AT+GTCAINFO?` | Y | Y | Y | Y | Y | Y | Y | L8 Family |

### Cell Camping Information

| Vendor | Model | Command | RAT | MCC | MNC | CID | TAC | LAC | Band | RAC | Network Capabilities | Source |
|--------|-------|---------|:---:|:---:|:---:|:---:|:---:|:---:|:----:|:---:|:--------------------:|--------|
| Fibocom | L850-GL/L860-GL | `AT+XCCINFO?` | Y | Y | Y | Y | Y | Y | Y | Y | Y | V3.2.3 |

**Note:** Fibocom L850-GL/L860-GL `AT+XCCINFO?` — not a measurement command; returns cell camping/registration state with GPRS/EDGE/HSDPA/HSUPA availability flags.

### 3GPP Standard Commands

| Vendor | Model | Command | WiGLE Complete | Registration State | TAC/LAC | CID | RAT | MCC | MNC | EARFCN | RSRP | PCI | Source |
|--------|-------|---------|:--------------:|:------------------:|:-------:|:---:|:---:|:---:|:---:|:------:|:----:|:---:|--------|
| All | All LTE | `AT+CEREG?` | N | Y | Y | Y | Y | - | - | - | - | - | 3GPP TS 27.007 |
| All | All 2G/3G | `AT+CREG?` | N | Y | Y | Y | - | - | - | - | - | - | 3GPP TS 27.007 |
| All | All | `AT+COPS?` | N | - | - | - | - | Y | Y | - | - | - | 3GPP TS 27.007 |
| All | All | `AT+COPS=?` | N | - | - | - | Y | Y | Y | - | - | - | 3GPP TS 27.007 |
| All | All | `AT+CSQ` | N | - | - | - | - | - | - | - | RSSI only | - | 3GPP TS 27.007 |

No combination of 3GPP-standard commands is WiGLE complete. Even combining
all five commands above, **EARFCN and RSRP are unavailable** — these fields
are only returned by vendor-specific extensions (e.g., Quectel `AT+QENG`,
Telit `AT#RFSTS`, Sierra `AT!GSTATUS?`). `AT+CSQ` returns RSSI, not RSRP.

**Note:** `AT+COPS=?` is slow (30s-5min) and blocks the modem. Quectel EG25-G `AT+COPS=?` has been observed to hang the modem, requiring a power cycle.

---

## Slow Commands (30 seconds to minutes)

These commands trigger an active RF scan across configured frequency bands.
They block the serial port for the duration of the scan and are only suitable
for stationary surveys or infrequent polling.

| Vendor | Model | Command | WiGLE Complete | Duration | RATs | RAT W | MCC W | MNC W | CID W | TAC W | PCI | Freq W | Band | BW | RSRP W | RSRQ | RSSI | SCS | srxlev | squal | Cell Status | Source |
|--------|-------|---------|:-----:|----------|------|:---:|:---:|:---:|:---:|:---:|:---:|:----:|:----:|:--:|:----:|:----:|:----:|:---:|:------:|:-----:|:-----------:|--------|
| Quectel | EC2x/EG2x/EG9x/EM05 | `AT+QOPS` | Y | 1-5 min | 2G+3G+LTE | Y | Y | Y | Y | Y | Y | Y | - | - | Y | Y | - | - | - | - | - | V2.1 §6.16 |
| Quectel | RM500Q/RM502Q | `AT+QSCAN=1,1` | Y | ~2 min | LTE | Y | Y | Y | Y | Y | Y | Y | Y | - | Y | Y | - | - | Y | Y | - | V1.2 §5 |
| Quectel | RM500Q/RM502Q | `AT+QSCAN=2,1` | Y | ~2 min | NR | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | - | Y | Y | - | - | V1.2 §5 |
| Quectel | RM500Q/RM502Q | `AT+QSCAN=3,1` | Y | ~2 min | LTE+NR | Y | Y | Y | Y | Y | Y | Y | Y | * | Y | Y | - | * | Y | * | - | V1.2 §5 |
| Telit | LM960 | `AT#CSURV` (neighbor) | N | (same) | (same) | Y | - | - | - | - | Y | Y | D | - | Y | Y | Y | - | - | - | Y | Rev.8 §5.6.9.1 |
| Telit | LM960 | `AT#CSURV` (serving) | Y | 30s-3 min | 2G+3G+LTE | Y | Y | Y | Y | Y | Y | Y | D | Y | Y | Y | Y | - | - | - | Y | Rev.8 §5.6.9.1 |
| Telit | LM960 | `AT#CSURVC` (neighbor) | N | (same) | (same) | D | - | - | - | - | Y | Y | D | - | Y | Y | Y | - | - | - | Y | Rev.8 §5.6.9.2 |
| Telit | LM960 | `AT#CSURVC` (serving) | Y | 30s-3 min | 2G+3G+LTE | D | Y | Y | Y | Y | Y | Y | D | Y | Y | Y | Y | - | - | - | Y | Rev.8 §5.6.9.2 |

`*` = present for NR cells only (AT+QSCAN=3,1 returns both LTE and NR cells with different field sets)

**Notes:**
- Quectel EC2x/EG2x/EG9x/EM05 `AT+QOPS` — requires idle state (no active CS/PS connections) and `AT+QCFG="NWSCANMODE",0` (AUTO mode). Results grouped by operator; 4G entries have PCI/TAC/RSRP/RSRQ, 3G have PSC/LAC/CI/RSCP/Ec/No, 2G have BSIC/rxlev.
- Telit LM960 `AT#CSURVC` vs `AT#CSURV` — CSURVC returns numeric-only format; CSURV includes text labels. Same data content.
- Telit LM960 `AT#CSURVC` / `AT#CSURV` — LTE scan only works when camped on an LTE cell ("partly implemented" per manual).
- Telit LM960 `AT#CSURVC` — MCC/MNC: the manual states hexadecimal, but observed output is decimal.
- Telit LM960 `AT#CSURVC` — distinguishes serving cells (11 fields) from neighbor cells (6 fields) by field count.
- Telit LE910Cx `AT#CSURVB`, `AT#CSURVBC`, `AT#CSURVF`, etc. — additional survey format variants documented in *LE910Cx AT Commands Reference Guide* Rev.18.

---

## Scan Strategy Recommendations

### Wardrive (mobile survey)

Poll fast commands at short intervals. Skip slow scans — they block the serial
port and miss cells as you move.

| Vendor | Model | Serving (every 2s) | Neighbors (every 5s) |
|--------|-------|--------------------|----------------------|
| Fibocom | L850-GL/L860-GL | `AT+XCELLINFO?` | `AT+XMCI=1` |
| Quectel | EC2x/EG2x/EG9x/EM05/EP06 | `AT+QENG="servingcell"` | `AT+QENG="neighbourcell"` |
| Quectel | EG12/EM12/EG18 | `AT+QENG="servingcell"` | `AT+QENG="neighbourcell"` |
| Quectel | RG520N/RM520N/RM530N | `AT+QENG="servingcell"` | `AT+QENG="neighbourcell"` |
| Quectel | RM500Q/RM502Q | `AT+QENG="servingcell"` | `AT+QENG="neighbourcell"` |
| Sierra | EM74xx/MC74xx/EM7511/MC7411 | `AT!GSTATUS?` | `AT!LTEINFO?` |
| Sierra | EM91/EM92 | `AT!GSTATUS?` + `AT!NRINFO?` | `AT!LTEINFO?` |
| SIMCom | SIM7070/SIM7080/SIM7090 | `AT+CPSI?` | `AT+CENG` |
| SIMCom | SIM7500/SIM7600 | `AT+CPSI?` | (not available) |
| SIMCom | SIM82XX/SIM83XX | `AT+CPSI?` | (not available) |
| Telit | LM960 | `AT#RFSTS` | `AT#MONI` (modes 1+2) |

### Stationary (fixed-location survey)

Run fast commands continuously plus periodic slow scans. The slow scans reveal
cells that the modem would not normally measure as neighbors (different PLMNs,
barred cells, low-priority cells).

| Vendor | Model | Serving (2s) | Neighbors (5s) | Full Scan (every 60-120s) |
|--------|-------|-------------|----------------|---------------------------|
| Quectel | EC2x/EG2x/EG9x/EM05/EP06 | `AT+QENG="servingcell"` | `AT+QENG="neighbourcell"` | `AT+QOPS` |
| Quectel | EG12/EM12/EG18 | `AT+QENG="servingcell"` | `AT+QENG="neighbourcell"` | (not available) |
| Quectel | RM500Q/RM502Q | `AT+QENG="servingcell"` | `AT+QENG="neighbourcell"` | `AT+QSCAN=3,1` |
| Telit | LM960 | `AT#RFSTS` | `AT#MONI` (modes 1+2) | `AT#CSURVC` |

### Multi-modem

When using multiple modems simultaneously, each modem sees different cells
depending on its SIM/PLMN, antenna, and band configuration. Running one modem
per carrier maximizes coverage. If two modems observe the same cell tower
(same MCC+MNC+TAC+CellID), the observations can be merged via the cell key.

---

## Terminology

| Term | Definition |
|------|-----------|
| **RAT** | Radio Access Technology — the generation/type of cellular network. Common values: GSM (2G), WCDMA/UMTS (3G), LTE (4G), NR (5G New Radio) |
| **PLMN** | Public Land Mobile Network — a cellular network identified by its MCC+MNC pair (e.g., 310260 = T-Mobile US) |
| **MCC** | Mobile Country Code — 3-digit country identifier (e.g., 310 = United States, 311 = United States extended) |
| **MNC** | Mobile Network Code — 2- or 3-digit network identifier within an MCC (e.g., 260 = T-Mobile, 410 = AT&T, 480 = Verizon) |
| **Serving cell** | The cell tower the modem is currently camped on |
| **Neighbor cell** | Cells the modem can detect but is not camped on |
| **Intra-frequency** | Neighbor cells on the same EARFCN as the serving cell |
| **Inter-frequency** | Neighbor cells on a different EARFCN |
| **Inter-RAT** | Neighbor cells on a different radio technology (e.g., WCDMA neighbors while camped on LTE) |
| **EARFCN** | E-UTRA Absolute Radio Frequency Channel Number — integer that maps to a specific DL center frequency via 3GPP TS 36.101 Table 5.7.3-1. Each LTE band has a reserved EARFCN range (e.g., 0-599 = Band 1, 66436-67335 = Band 66) |
| **NR-ARFCN** | NR Absolute Radio Frequency Channel Number — same concept as EARFCN but for 5G NR, with finer frequency granularity (5/15/60 kHz steps). Defined in 3GPP TS 38.104 Table 5.4.2.1-1 |
| **PCI** | Physical Cell Identity — a local identifier broadcast by each cell sector. Not globally unique — reused across the network. Range: 0-503 for LTE, 0-1007 for NR |
| **CID / Cell ID** | Cell Identifier — unique within a PLMN; for LTE this is the 28-bit E-UTRAN Cell Identifier (ECI) |
| **ECI** | E-UTRAN Cell Identifier — 28-bit value composed of eNB ID (20 bits) + Sector ID (8 bits). Globally unique when combined with PLMN |
| **eNB ID** | eNodeB Identifier — identifies the base station hardware. Derivable from ECI: `eNB_ID = ECI >> 8` |
| **TAC** | Tracking Area Code — groups cells into tracking areas for LTE paging and mobility. Analogous to LAC in 2G/3G |
| **LAC** | Location Area Code — the 2G/3G equivalent of TAC |
| **FDD / TDD** | Frequency Division Duplex / Time Division Duplex — how uplink and downlink are separated. FDD uses paired frequency bands; TDD uses the same frequency with time slots |
| **RSRP** | Reference Signal Received Power — average power of resource elements carrying cell-specific reference signals, in dBm. The primary metric for cell signal strength. Typical range: -140 to -44 dBm (3GPP TS 36.214 §5.1.1) |
| **RSRQ** | Reference Signal Received Quality — ratio of RSRP to total received wideband power (RSSI) across N resource blocks: `RSRQ = N × RSRP / RSSI`, in dB. Indicates signal quality relative to interference and noise. Typical range: -20 to -3 dB (3GPP TS 36.214 §5.1.3) |
| **RSSI** | Received Signal Strength Indicator — total received wideband power including serving cell, interference, and thermal noise, in dBm. Not cell-specific — measures everything the antenna receives (3GPP TS 36.214 §5.1.3) |
| **SINR** | Signal to Interference plus Noise Ratio — serving cell signal power relative to interference and noise, in dB. Higher is better. Not standardized in early LTE specs; vendor-reported |
| **RSCP** | Received Signal Code Power — the WCDMA (3G) equivalent of RSRP, measuring the power of a single spreading code on the pilot channel, in dBm |
| **Ec/Io** | Energy per chip to Interference ratio — the WCDMA (3G) equivalent of RSRQ, in dB |
| **DRX** | Discontinuous Reception — power-saving cycle length where the modem periodically wakes to check for paging. Reported in milliseconds |
| **SCS** | Subcarrier Spacing — NR-specific parameter (15, 30, 60, 120, or 240 kHz). Determines the OFDM numerology and affects time-domain granularity |
| **CQI** | Channel Quality Indicator — a 4-bit value (0-15) reported by the UE to indicate supportable modulation/coding rate. Higher = better channel |
| **CA** | Carrier Aggregation — bonding multiple LTE/NR carriers to increase bandwidth. PCC = Primary Component Carrier, SCC = Secondary Component Carrier |
| **ENDC** | E-UTRA NR Dual Connectivity — an NSA (Non-Standalone) 5G deployment where LTE provides the control plane anchor and NR provides additional data throughput |
| **srxlev** | Cell selection receive level value — `srxlev = RSRP - (Qrxlevmin + Qrxlevminoffset) - Pcompensation`. Positive means the cell meets minimum signal level for camping (3GPP TS 36.304 §5.2.3.2) |
| **squal** | Cell selection quality value — `squal = RSRQ - (Qqualmin + Qqualminoffset)`. Positive means the cell meets minimum quality for camping (3GPP TS 36.304 §5.2.3.2) |

---

## Source Documents

| Vendor | Document | Version | Date | Filename |
|--------|----------|---------|------|----------|
| Quectel | *BG95&BG77&BG600L Series AT Commands Manual* | V2.0 | (see file) | `Quectel_BG95BG77BG600L_Series_AT_Commands_Manual_V2.0.pdf` |
| Quectel | *EC2x&EG2x&EG9x&EM05 Series AT Commands Manual* | V2.1 | 2025-03-21 | `Quectel_EC2xEG2xEG9xEM05_Series_AT_Commands_Manual_V2.1.pdf` |
| Quectel | *EM12&EG12&EG18 Series AT Commands Manual* | V1.1 | (see file) | `Quectel_EM12EG12EG18_Series_AT_Commands_Manual_V1.1.pdf` |
| Quectel | *RG520N&RG525F&RG5x0F&RM5x0N Series AT Commands Manual* | V1.1 | (see file) | `Quectel_RG520NRG525FRG5x0FRM5x0N_Series_AT_Commands_Manual_V1.1.pdf` |
| Quectel | *RG50xQ&RM5xxQ Series AT Commands Manual* | V1.2 | 2021-08-09 | `Quectel_RG50xQ_RM5xxQ_Series_AT_Commands_Manual_V1.2.pdf` |
| Telit | *LM960 Series AT Command Reference Guide* | Rev.8 | 2022-03-21 | `Telit_LM960_Series_AT_Command_Reference_Guide_r8.pdf` |
| Telit | *FN980 Family AT Commands Reference Guide* | Rev.5 | (see file) | `telit_fn980_family_at_commands_reference_guide_r5.pdf` |
| Telit | *LE910Cx AT Commands Reference Guide* | Rev.18 | (see file) | `Telit_LE910Cx_AT_Commands_Reference_Guide_r18.pdf` |
| Sierra | *AirPrime EM75xx/EM-MC74x1 AT Command Reference* | r4 | (see file) | `4117727 AirPrime EM74xx-MC74xx AT Command Reference r3.pdf` |
| Sierra | *EM9 AT Command Reference* | r14 | 2026-01 | `41113480 EM9 AT Command Reference r14.pdf` |
| Fibocom | *L8 Family AT Commands Manual* | V2.0.2 | (see file) | (in knowledge base) |
| Fibocom | *L860-GL AT Commands Manual* | V3.2.3 | (see file) | `L860GL-AT-Commands_V3.2.3.pdf` |
| SIMCom | *SIM7070/SIM7080/SIM7090 Series AT Command Manual* | V1.08 | (see file) | `SIM7070_SIM7080_SIM7090_Series_AT_Command_Manual_V1.08.pdf` |
| SIMCom | *SIM7500/SIM7600 Series AT Command Manual* | V3.00 | (see file) | `SIM7500_SIM7600-Series_AT-Command-Manual_V3.00.pdf` |
| SIMCom | *SIM82XX&SIM83XX Series AT Command Manual* | V1.02 | (see file) | `SIM82XX_SIM83XX_Series_AT_Command_Manual_V1.02.pdf` |
| 3GPP | *TS 27.007 — AT command set for UE* | (various) | — | Standard |
| 3GPP | *TS 36.101 — E-UTRA UE radio transmission and reception* | (various) | — | Table 5.7.3-1 (EARFCN-to-frequency) |
| 3GPP | *TS 36.304 — UE procedures in idle mode* | (various) | — | srxlev, squal definitions |
| 3GPP | *TS 38.104 — NR BS radio transmission and reception* | (various) | — | Table 5.4.2.1-1 (NR-ARFCN-to-frequency) |
