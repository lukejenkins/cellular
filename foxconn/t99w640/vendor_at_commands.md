# Foxconn T99W640 (Dell DW5934e) — Vendor AT Command Reference

The **Foxconn T99W640** (Dell **DW5934e**) is a Qualcomm **SDX72** 5G M.2 module.
This is a factual reference for the vendor (`AT+...`) command surface implemented by
the firmware's `atfwd-daemon`, intended for interoperability and tooling work.

- **Firmware in scope:** `FDE2.F0.0.0.1.2` modem build, apps build `047`
  (`.DF.001` retail slot; the `.GC.001` slot's `atfwd-daemon` is byte-identical).
- **Hardware model string:** `DP25-42843-47` (board revisions `-42843-47` / `-43`,
  rev `V005` observed).
- **Chipset transport:** PCIe / M.2, so DIAG and AT ride **MHI** (`/dev/mhi_*`),
  not USB endpoints, on the units observed.

> **Discovery method:** the vendor verb list was recovered by `strings`/symbol
> analysis of the on-modem `atfwd-daemon` binary (ARM aarch64, musl-linked). Verbs
> marked "discovered-in-binary" have a compiled handler but were not necessarily
> live-probed. The live-probe results in §2 were taken read-only from an on-modem
> AT channel.

---

## 1. Vendor AT command catalog

`atfwd-daemon` registers **40 vendor `AT+...` verbs** (plus the standard 3GPP
`AT+CFUN`). On this firmware the vendor extensions live exclusively under the `AT+`
prefix — `AT^...`, `AT$...`, `AT+DELL*` families all return `ERROR`. Each verb is
dispatched through an `atfwd_exec_<name>_cmd` handler; state-changing verbs pair with
an `_after_response` finalizer.

Some verbs are gated by the on-device **SDC** (Software Debug Control) access-control
subsystem (see §3) and return `ERROR` ("Command not supported without SDC unlock")
unless SDC is unlocked.

### Identity / status (read-oriented, generally safe)

| Command | Purpose |
|---|---|
| `AT+TEMP` | Read modem thermal state. |
| `AT+BOOT_VER` | Read SBL / UEFI bootloader versions. |
| `AT+SERIAL_NUMBER` | Read the module serial number. |
| `AT+FUSEID` | Read Qualcomm fuse identifiers (`fx_get_fuseid_info` / `fx_get_fusesub_id`). |
| `AT+SECBOOT_STATUS` | Read the secure-boot fuse flag (`/sys/devices/virtual/oem/sw/secbootflag`). *(DF-slot only)* |
| `AT+FPIN` / `AT+RFPIN_STATUS` | RF pin readback / status. |
| `AT+RF_LED` | RF-status LED control. |
| `AT+PDSHFLG` | Power-down / shutdown flag (`pdflag` SMEM bridge). |

### Personality / feature / security

| Command | Purpose |
|---|---|
| `AT+CUSTOMER` | OEM personality selector. SET writes a raw customer id `N` to the `vendor` MTD partition, removes the `/data/fx_usb` USB-composition cache, then reboots; READ (`?`) reports `+CUSTOMER :<N>`. See §4 for the id map. **SDC-gated on retail (`.DF.001`/`047`); ungated on engineering builds.** Reboots. |
| `AT+FEATURE` | Feature-mask editor with exactly two sub-keys: `esim` (bit 0) and `fcc_lock` (bit 1). Write form `=<key>,<0\|1>` does a read-modify-write of that one bit and persists `{'FX' magic, project id, feature_mask}` to the `vendor` partition; READ (`?`) reports both bits. **SDC-gated on retail.** See §4.2. |
| `AT+SDC_UNLOCK` | SDC challenge-response unlock (SET-only; no read/test mode). Unlocks SDC-gated verbs. See §3. |
| `AT+SDC_LOCK` | Re-lock SDC. |
| `AT+FCC_LOCK` | FCC-lock state command. READ (`?`) returns `+FCC_LOCK: <0\|1>`; SET issues a QMI request to the Foxconn (FX) QMI service. See §5 for observed behavior and a response-terminator quirk. |

### Logging / debug

| Command | Purpose | Notes |
|---|---|---|
| `AT+FXLOGLVL` | Foxconn log-level control (`fx_set_debug_log_level`). | |
| `AT+CONSOLELOG` | Console-log enable toggle. | |
| `AT+DIAG_ENABLE` | Enable/disable the Qualcomm DIAG plane. SET form is `AT+DIAG_ENABLE=sdx72,<0\|1>` (the literal `sdx72` is a mandatory guard word; `1`=enable, `0`=disable), then reboots. Backed by the presence/absence of `/data/diag_disable` (file present ⇒ DIAG disabled — inverted polarity). | **SDC-gated** (both read and set gated by the same predicate). |
| `AT+URRXTX` | Debug-UART enable. SET `=<0\|1>` writes a flag consumed by the ABL bootloader at boot to keep or strip `console=ttyMSM0,115200,n8` from the kernel command line. Valid values `0`/`1` only; takes effect at next boot. | **SDC-gated.** Persists across `/data` factory wipe (stored in the `fwinfo` flash partition on the retail slot). |

### Boot / recovery / reset

| Command | Purpose | Danger |
|---|---|---|
| `AT+RESET` | Normal modem reboot. | Reboots. |
| `AT+FASTBOOT` | Writes the `bootloader` literal into the `misc` partition and reboots into fastboot. | Non-destructive but reboots. |
| `AT+ODIS` | ODIS (OEM device identity strings), per-MCC/MNC. Responses include `+ODIS: Reset to default` / `+ODIS: Incorrect input format`. | |
| `AT+EDL` | ⚠️ **DESTRUCTIVE.** Runs `mtd erase /dev/mtd0 && sync && reboot` — erases the SBL and forces host-side EDL (9008) factory recovery. Do **not** invoke without a recovery package and a vendor-signed loader on hand. |
| `AT+PCIE_EDL` | ⚠️ **DESTRUCTIVE.** Same action as `AT+EDL` (erases the bootloader to force EDL entry). *(DF-slot only.)* |
| `AT+EFS_ERASE` | ⚠️ **DESTRUCTIVE.** Wipes EFS2 calibration. (Not SDC-gated on this build.) |

### DPR (dynamic power restriction)

| Command | Purpose |
|---|---|
| `AT+DPR_ENABLE` | Read/write `/data/dpr/dpr_enable`; reflected in the first digit of `/sys/devices/virtual/oem/sw/dpr_info`. |
| `AT+DPR_CONTROL_MODE` | Read/write `/data/dpr/dpr_control_mode`; second digit of `dpr_info`. |

### GNSS

| Command | Purpose |
|---|---|
| `AT+FGPS` (bare) | GNSS engine control. |
| `AT+FGPS_START` / `AT+FGPS_STOP` | Start / stop the GNSS engine. |
| `AT+FGPS_INFO` / `AT+FGPS_LOCDATA` | Fix / location-data readout. |
| `AT+FGPS_OPER_MODE` | Standalone / MS-based / MS-assisted mode select. |
| `AT+FGPS_SUPL` / `AT+FGPS_XTRA` | SUPL server / XTRA assistance config. |
| `AT+FGPS_PIN_STATUS` | GNSS RF-pin / antenna status. *(DF-slot only.)* |

### USB / transport

| Command | Purpose |
|---|---|
| `AT+USBSWITCH` | USB composition switch — writes a 4-hex composition id into `/etc/usb/boot_hsusb_comp`. On PCIe/M.2 units USB is typically disabled (`boot_hsusb_comp = none`). |
| `AT+USBTYPE` | USB endpoint type (when USB is enabled). |

### LWM2M device management

| Command | Purpose |
|---|---|
| `AT+LWM2M_SWITCH` | LwM2M client enable (per-carrier). |
| `AT+LWM2M_BS_ADDR` | LwM2M bootstrap server address. |
| `AT+LWM2M_FOTA_SWITCH` | LwM2M FOTA enable. *(DF-slot only.)* |

### Standard 3GPP (for reference)

| Command | Purpose |
|---|---|
| `AT+CFUN` | Radio functionality (3GPP 27.007). On observed units the modem often idles in `+CFUN: 7` (manufacturer test mode). |

> The four DF-slot-only verbs (`+SECBOOT_STATUS`, `+PCIE_EDL`, `+FGPS_PIN_STATUS`,
> `+LWM2M_FOTA_SWITCH`) are present on the retail build but absent from the engineering
> build, which registers 36 vendor verbs instead of 40.

---

## 2. Observed read-mode responses

Read-only probes taken from an on-modem AT channel (`/dev/at_mdm0`), modem in
`+CFUN: 7`. **Real device identifiers below are redacted; the response structure is
preserved.**

| Command | Response |
|---|---|
| `ATI` | `Manufacturer: DELL` / `Model: DP25-42843-47` / `Revision: FDE2.F0.0.0.1.2.TO.003` / `SVN: 53` / `IMEI: <IMEI>` / `+GCAP: +CGSM` / `MPN: 00` / `OK` |
| `AT+CFUN?` | `+CFUN: 7` / `OK` (manufacturer test mode) |
| `AT+CFUN=?` | `+CFUN: (0-1,4-7),(0-1)` / `OK` |
| `AT+CUSTOMER?` | `+CUSTOMER :<N>` once SDC is unlocked; `ERROR` (SDC-gate reject) while locked. Test mode (`=?`) not supported. |
| `AT+CUSTOMER=?` | `ERROR` (test mode not implemented) |
| `AT+SDC_UNLOCK?` / `AT+SDC_UNLOCK=?` | `ERROR` (SET-only, no read/test mode) |
| `AT+FCC_LOCK?` | `+FCC_LOCK: 1` (see terminator quirk in §5) |
| `AT+FCC_LOCK=?` | `ERROR` (test mode not implemented) |

### AT transport notes

- On PCIe/M.2 units the reliable path is an **on-modem shell** talking to
  `/dev/at_mdm0` (also `/dev/smd7`, `/dev/smd8`). `/dev/smd11` is silent on this
  firmware — tools that default to it appear to hang.
- USB-AT devices (`/dev/at_usb0/1`) have no peer when USB is disabled
  (`boot_hsusb_comp = none`).
- The host-side MHI AT device (`/dev/wwan0at0`, in-tree `mhi_pci_generic` /
  `mhi_wwan_ctrl`) has been observed silent even when the modem AT processor itself
  responds — a transport/driver-side condition, not a firmware-AT-readiness issue.
- `/dev/wwan0at0` is a WWAN char device, **not** a TTY, so termios-based tools
  (tio, picocom, minicom, pyserial) do not work against it. Use raw `os.open` + `select`
  (see the companion `atsh.py`).

Internally, vendor AT commands reach `atfwd-daemon` **via QMI**: the modem AT parser
forwards registered vendor tokens to the daemon over the QMI AT service
(`QMI_AT_REG_AT_CMD_FWD_EX_REQ`/`QMI_AT_FWD_RESP_AT_CMD_REQ`/`QMI_AT_SEND_AT_URC_REQ`).
The daemon's unix socket (`/dev/socket/atfwd/ds_at_connect_socket`) is a control-plane
IPC, **not** a raw-AT entry point.

---

## 3. SDC (Software Debug Control) subsystem

SDC is a multi-level access-control layer inside `atfwd-daemon` that gates a subset of
vendor verbs. `AT+SDC_UNLOCK` is a **challenge-response**: the client sends a request,
the daemon replies with a random string, and the client must return the derived
password; `AT+SDC_LOCK` re-locks.

Gated verbs and their required flags:

| Verb(s) | Gate |
|---|---|
| `AT+DIAG_ENABLE` | `diagenable` flag |
| `AT+URRXTX` | `urrxtx` flag |
| `AT+CUSTOMER`, `AT+FEATURE` | level-2 (raised by any unlock) — **retail build only**; ungated on engineering builds |

SDC status is persisted in `/data/sdc_config` and mirrored to SMEM (so the modem
firmware can read it) and to `/sys/devices/virtual/oem/sw/sdc` (a 2-char status string,
e.g. `"00"` locked / `"11"` fully unlocked). A boot-time initializer rebuilds the SMEM
state from `/data/sdc_config`, so a persisted unlock survives reboot.

Observable status strings in the binary:
`check_sdc_status_lvl_2`, `check_sdc_status_lvl_3`, `check_sdc_status_diagenable`,
`check_sdc_status_urrxtx`, `check_sdc_status_antirollback`.

> `AT+SDC_UNLOCK` is a vendor-internal authentication command. The specific
> challenge-response derivation is not documented here.

---

## 4. `AT+CUSTOMER` and `AT+FEATURE` — personality / feature detail

### 4.1 `AT+CUSTOMER` — OEM personality selector

- **Option space (SET):** `N ∈ {0, 32, 33, 34, 35, 36}`, matched as literal strings;
  any other value is a no-op/ERROR. `AT+CUSTOMER=0` uses the bare literal `0`
  (length-1); the two-digit values are matched exactly.
- **Action:** writes a small struct `{'FX' magic, N, feature_info}` to the `vendor`
  MTD partition, then (deferred) `rm -rf /data/fx_usb` + `sync` + reboot. READ reports
  `+CUSTOMER :<N>`.

| N | hex | OEM personality | ATI manufacturer / model |
|--:|----:|---|---|
| 0 | 0x00 | Qualcomm generic | `Qualcomm` / `DP25-42843-47` |
| 32 | 0x20 | base / generic | `Qualcomm` / `DP25-42843-47` |
| 33 | 0x21 | **Dell** | `DELL` / `DP25-42843-47` |
| 34 | 0x22 | Thales | `Qualcomm` / `DP25-42843-47` |
| 35 | 0x23 | HP | `HP` / `Qualcomm(R) snapdragon(TM) X72` |
| 36 | 0x24 | HP (2nd variant) | `HP` / `Qualcomm(R) snapdragon(TM) X72` |

> ⚠️ **Caution — `AT+CUSTOMER=0`:** community reports (OpenWrt DW5934e forum thread)
> associate `AT+CUSTOMER=0` with **loss of ADB access** on this module family, because
> the personality change re-derives the USB/MHI composition on reboot (a `0`/Qualcomm
> personality may not expose an ADB interface). Recovery is to set `N` back to an
> ADB-bearing OEM value (33–36) via the same (SDC-gated) verb. Treat `=0` as a
> persistent, reboot-triggering write.

`AT+CUSTOMER` only mutates the `vendor`-partition customer field (plus the volatile
`/data/fx_usb` cache); it does not itself alter the feature mask.

### 4.2 `AT+FEATURE` — feature-mask editor

Exactly two sub-keys (it is **not** a generic key/value store):

| Sub-key | Feature-mask bit |
|---|---|
| `esim` | bit 0 |
| `fcc_lock` | bit 1 |

- Write form `AT+FEATURE=<key>,<0\|1>` requires two args; `<val>` must be the literal
  `"0"` or `"1"`. It read-modify-writes that one bit and persists
  `{'FX' magic | project id | feature_mask}` to the `vendor` partition. The write hits
  the flash partition only; the runtime SMEM copy is not refreshed until the next boot.
- Read form `AT+FEATURE?` reports `+FEATURE :` / `esim,<bit0>` / `fcc_lock,<bit1>` / `OK`.
- SDC-gated on the retail build.

> `AT+FEATURE=fcc_lock,1` is a real persistent feature-provisioning write. It is
> documented here for interop; it is not a routine command.

---

## 5. `AT+FCC_LOCK`

`AT+FCC_LOCK` reads/sets the module's FCC-lock state. The handler
(`atfwd_get_fcc_lock_status_handle`) calls into `libfxcm.so` and communicates with the
Foxconn (FX) QMI service.

- **Read:** `AT+FCC_LOCK?` → `+FCC_LOCK: <0|1>` (1 = locked).
- **Set:** `AT+FCC_LOCK=<n>` issues a QMI set-status request. The daemon enumerates
  three internal outcomes: (1) disable, radio unchanged; (2) disable, radio on;
  (3) enable, radio LPM.
- Observed: on the units tested, a runtime `AT+FCC_LOCK=0` was **volatile** — it
  changed the reported value but reverted to `1` on reboot and did not bring the radio
  online on its own.

> ⚠️ **Response-terminator quirk (interop-relevant).** `AT+FCC_LOCK?` emits
> `+FCC_LOCK: <n>` **without** a trailing `OK\r\n`. Tools that wait for a standard
> terminator (`OK\r\n` / `ERROR\r\n` / `+CME ERROR:`) will hang — the value is on the
> wire immediately. Bound reads with a quiet-timeout / read-until-quiet helper rather
> than a strict terminator match.

This document names `AT+FCC_LOCK` and its observable behavior only. It intentionally
does **not** cover FCC-lock defeat or any unlock-derivation procedure.

---

## 6. Retail vs. engineering/prototype builds

The customer-personality system branches on a vendor id (Dell / HP / Thales), so the
same `AT+CUSTOMER=N` invocation can have different downstream effects across SKUs.
Notable differences observed between the retail Dell build (`.DF.001`/`.GC.001`, apps
`047`) and engineering builds (`.TO.001`, apps `032`):

| Aspect | Retail Dell | Engineering / prototype |
|---|---|---|
| `AT+CUSTOMER` / `AT+FEATURE` | SDC-gated | natively ungated |
| Registered vendor verbs | 40 | 36 (no `+SECBOOT_STATUS`/`+PCIE_EDL`/`+FGPS_PIN_STATUS`/`+LWM2M_FOTA_SWITCH`) |
| `AT+URRXTX` storage | `fwinfo` flash partition (survives `/data` wipe) | `/data/dpr/uart_enable` (wiped by factory reset) |

Recipes validated on an engineering/prototype sample do not necessarily transfer 1:1
to a retail unit.

---

## 7. Quick danger reference

| Command | Effect |
|---|---|
| `AT+EDL`, `AT+PCIE_EDL` | Erase SBL → forced EDL; needs signed loader + recovery image. **Destructive.** |
| `AT+EFS_ERASE` | Wipes EFS2 calibration. **Destructive.** |
| `AT+CUSTOMER=0` | Persistent personality flip; may remove ADB. Reboots. |
| `AT+CUSTOMER=<33..36>` | Persistent personality write. Reboots. |
| `AT+FEATURE=<key>,<v>` | Persistent feature-mask write. |
| `AT+DIAG_ENABLE=sdx72,<v>` | Toggles DIAG plane; reboots. |
| `AT+URRXTX=<v>` | Enables/strips the debug UART at next boot (persistent). |
| `AT+FASTBOOT`, `AT+RESET` | Reboot (non-destructive). |
