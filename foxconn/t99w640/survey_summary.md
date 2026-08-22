# Foxconn T99W640 (Dell DW5934e) — Capability Survey Summary

A sanitized, model-level summary of the verified capabilities and enumerated
state of the Foxconn T99W640 / Dell DW5934e 5G modem. All unit-specific and
subscriber identifiers have been omitted; this document describes the platform,
not any individual unit.

## Platform overview

| Field | Value |
|---|---|
| Vendor | Foxconn International (Hon Hai) |
| Model | Foxconn T99W640 / **Dell DW5934e** |
| Model / board number | `DP25-42843-47` |
| Chipset | Qualcomm Snapdragon X72 (SDX72) |
| 3GPP release | Release 17 |
| Category | 5G NR Sub-6 (RedCap-capable per X72 platform) |
| Form factor | M.2 |
| Bus | PCIe via MHI (no USB enumeration; `boot_hsusb_comp = none`) |
| PCI VID:PID | `105b:e11d` |
| Hardware revision (DMS) | `V005` |
| Firmware GUID (MS firmware-id) | ASCII `0489E1310515FX00` (Dell firmware/part identifier packed into the MBIM `ms-firmware-id` GUID) — model-level, identical across units on this firmware |

## Firmware builds

The catalog anchor is apps build `047` on the stable modem prefix
`FDE2.F0.0.0.1.2`. The ATI suffix tracks the active carrier MCFG (personality)
independently of the underlying image:

| Firmware string | Notes |
|---|---|
| `FDE2.F0.0.0.1.2.DF.001` | Dell retail personality (catalog directory) |
| `FDE2.F0.0.0.1.2.GC.001` | Generic/base personality |
| `FDE2.F0.0.0.1.2.TO.003` | T-Mobile MCFG active |
| apps build | `047` (sysfs `apps_ver`) |
| AMSS / software version | `FDE2.F0.0.0.1.2` (DMS `--dms-get-software-version`) |
| SBL / UEFI | `sbl_ver 0.0.0.5`, `uefi_ver 002` |
| Build date | Dec 16 2024 |

DMS manufacturer string is `DELL` under the Dell personality; it reports
`Qualcomm` under the generic base personality. DMS model reports
`DP25-42843-47` in both.

## Supported RATs and bands

Reported via QMI NAS system-selection-preference (band-capability lists are
model-level and independent of any SIM/attach state). RF is software-off in all
captures, so these are the firmware's supported/preferred band sets rather than
a live serving configuration.

| RAT | Bands |
|---|---|
| WCDMA (UMTS) | 2100, PCS-1900, 1700 (US), 850 (US), 900 |
| LTE | 1, 2, 3, 4, 5, 7, 8, 12, 13, 14, 17, 18, 19, 20, 25, 26, 28, 29, 30, 32, 34, 38, 39, 40, 41, 42, 43, 46, 48, 66, 67, 68, 70, 71 |
| NR5G SA | 1, 2, 3, 5, 7, 8, 12, 13, 14, 18, 20, 25, 26, 28, 29, 30, 38, 40, 41, 48, 66, 67, 70, 71, 75, 76, 77, 78, 79, 91, 92, 93, 94 |
| NR5G NSA | (same set as NR5G SA) |
| TD-SCDMA | a–f (preference field present) |

Mode preference: `umts, lte, 5gnr`. Technology preference: `3gpp, cdma-or-wcdma,
lte` (permanent). NR bands are all FR1/sub-6; no FR2/mmWave bands are listed
(consistent with the sub-6 X72 SKU, though `mmw*` thermal zones exist on the
platform).

## USB / MHI composition and device-node layout

The modem is **PCIe-only** — no USB enumeration. The in-tree upstream
`mhi_pci_generic` driver binds it (`MHI PCI device found: foxconn-dw5934e`) and
registers **5 MHI channels**:

| MHI channel | Userspace node | Type / purpose |
|---|---|---|
| `mhi0_DIAG` | `/dev/wwan0qcdm0` | QCDM (Qualcomm DIAG) |
| `mhi0_DUN` | `/dev/wwan0at0` | AT command port (natively registered) |
| `mhi0_IP_HW0_MBIM` | `wwan0` (net, RAWIP, MTU 1500) | combined hardware IP + MBIM data |
| `mhi0_LOOPBACK` | kernel-internal | MHI loopback test |
| `mhi0_MBIM` | `/dev/wwan0mbim0` | MBIM control |

PCIe link: Gen (cap 16 GT/s x2), negotiated 5 GT/s x1 in this host slot; AER
present, MSI 8/32. No standalone QMI or IPCR channel in the generic driver
table; QMI is reached over the MBIM `qmi` tunnel service or, on a host running
the vendor out-of-tree MHI driver, via additional `/dev/mhi_*` channels
(including a `/dev/mhi_QMI0` and an ADB channel).

Compared with the Quectel SDX62/SDX65 MHI modems in the fleet, this firmware
exposes a **native DUN/AT channel** and collapses the IP + MBIM data path into a
single combined channel.

## Driver / attachment state (which paths work)

| Path | Status |
|---|---|
| MBIM (`mbimcli` on `/dev/wwan0mbim0`) | **Works** — primary capture path; basic-connect, ms-basic-connect-extensions, ms-firmware-id, ms-uicc-low-level-access all respond |
| QMI-over-MBIM (`qmicli --device-open-mbim`) | Partial — service enumeration + DMS/NAS/UIM/WDS/PDC queries succeed; some indications time out on the tested libqmi/libmbim build |
| Standalone QMI (`/dev/wwan0qmi0`) | Not present with the in-tree driver; a `/dev/mhi_QMI0` appears under the vendor out-of-tree driver |
| Host-side AT (`/dev/wwan0at0` / `mhi_DUN`) | Registered but **silent** on this firmware via naive host I/O |
| On-modem AT | Reachable via the modem's internal serial device once a shell is available |
| ADB (over MHI) | Reachable on a host running the vendor OOT MHI driver; not exposed by the in-tree generic driver |

## QMI service inventory

Enumeration returned ~42 service entries. Named services (service IDs/names
only — no subscriber data):

`ctl, wds, dms, nas, qos, wms, auth, at, voice, cat2, uim, pbm, test, loc, sar,
ts, tmd, wda, csvt, coex, pdc, dsd`, a vendor service **`fox`** (1.0), plus a
number of unnamed vendor service IDs (`0x2d, 0x2e, 0x30, 0x31, 0x44, 0x49, 0x4a,
0x4c, 0x4d, 0x4e, 0x55–0x5a, 0x5c, 0xe4`).

Notable verified queries: DMS identity/model/revision/capabilities, NAS
band/technology preferences, UIM card + slot status, WDS profile list, LOC
engine-lock, WDA data format, PDC carrier-config list.

## MBIM device-service inventory (19 services)

| Service | CIDs | Notes |
|---|---|---|
| basic-connect | 21 | standard MBIM baseline |
| sms | 5 | |
| ussd | 1 | |
| phonebook | 4 | |
| stk | 3 | SIM toolkit |
| auth | 2 | SIM AKA/SIM auth |
| qmi | 1 | QMI-over-MBIM tunnel |
| ms-host-shutdown | 2 | |
| (vendor `2d0c12c9…`) | 4 | unnamed vendor service |
| ms-firmware-id | 1 | returns the Dell part-number GUID |
| atds | 6 | Microsoft extended stats/operators/RAT |
| qdu | 3 | firmware-update CIDs only (update-session, file-open, file-write) |
| ms-uicc-low-level-access | 10 | direct SIM APDU access (atr, apdu, read-binary, etc.) |
| ms-basic-connect-extensions | 17 | device-caps-v2, slot mappings, base-stations-info, device-reset, etc. |
| ms-sar | 2 | SAR config |
| ms-voice-extensions | 1 | NITZ only |
| (vendor `cfd497ea…`) | 10 | unnamed vendor service (likely Dell/Foxconn) |
| (vendor `fbc51292…`) | 1 | unnamed vendor service |
| google | 1 | `carrier-lock` (GSMA-style carrier-lock management) |

Max DSS sessions: 0. The `google carrier-lock` service and the three unnamed
vendor services indicate an Android-like internal subsystem, consistent with the
SDX72 platform's smartphone lineage.

## SIM / eUICC architecture

- **Two SIM slots**: slot 0/1 = physical removable UICC; the other = **soldered
  eUICC** (`Is eUICC: yes`, Thales/Gemalto issuer range; the EID itself is
  per-unit and is not published).
- eUICC is alive at the ICC level: UICC ATR retrieved successfully (direct
  convention, 3 V class); MBIM `ms-uicc-low-level-access` APDU service available
  for LPA/profile operations.
- Executor/slot model (MBIM sys-caps): 1 executor, 2 slots, concurrency 1.
- Reported SIM applications on a populated slot include USIM and ISIM.

## Carrier configuration (MCFG) bundles

PDC reported **22 stored carrier configurations** (one active at a time). Bundle
names present in firmware: GCF, T-Mobile, CU, CT, CMCC, Swisscom, KDDI, SBM,
Docomo, Verizon / Verizon_ext, DT / DT2, US, Telefonica, Orange, EE, Vodafone,
Telstra, Optus, ATT / ATT2. These are firmware-level carrier config bundles, not
subscriber data.

## Observed operating state (non-identifying)

Across captures the radio was software-off / LPM (`+CFUN: 7`, software radio
state `off`, hardware radio state `on`), deregistered, packet service detached,
no serving cell — so no live-network or location-revealing data was present to
redact. SMS-only registration (`0,6`) was reported in the offline state.

Platform thermal telemetry exposes the expected SDX72 sensor set (sdr, mmw0–3,
cpuss, mdmss/mdmq6, aoss, pmx75, xo-therm), confirming the sub-6 + mmW-capable
silicon family even though only sub-6 bands are enabled.

---

*Model-level facts only. Unit identifiers (IMEI, IMSI, ICCID, EID, MSISDN,
serial numbers) and any live-network attach data have been omitted by design.*
