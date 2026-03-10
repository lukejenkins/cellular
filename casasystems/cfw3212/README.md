# Casa Systems CFW-3212

5G NR / LTE fixed wireless access CPE (Fixed Wireless Access Customer Premises Equipment). Window-mount outdoor unit with integrated 4×4 MIMO antenna, PoE-powered via a single 2.5Gbit Ethernet port.

Originally designed by NetComm Wireless, acquired by Casa Systems (now DZS). The surveyed variant is the U.S. Cellular carrier-branded version distributed as part of the USCC Home Internet service.

---

## Hardware Overview

| Field | Value |
|-------|-------|
| Model | CFW-3212 (also labelled CFW-3212-02 in carrier firmware) |
| Series | AurusLink+ / CFW-32xx |
| Chipset | Qualcomm SDX62 (via Quectel RG520N-NA OpenCPU) |
| Category | 5G FWA CPE |
| PCB | HS-2800i-v2.1 (2022-02-02) |
| Ethernet | 1× 2.5Gbit (24VDC non-standard PoE input — **not** IEEE 802.3) |
| Antenna | Integrated 4×4 MIMO WWAN + BLE |
| SIM slot | Yes (internal) |
| Enclosure | IP65 (indoor/outdoor window mount) |

**OpenCPU architecture:** The unit runs embedded Linux directly on the SDX62 application processor — there is no separate host MCU. The Quectel RG520N-NA module is the entire compute platform, not just the modem.

---

## Contents

| Document | Description |
|----------|-------------|
| [flash_layout.md](flash_layout.md) | NAND flash partition map, UBI volumes, and A/B boot scheme |
| [guide_block_carrier_remote_mgmt.md](guide_block_carrier_remote_mgmt.md) | How to block TR-069 and LwM2M carrier remote management |
| [screenshots/](screenshots/README.md) | Web UI screenshots (42 images, all pages, sensitive fields redacted) |
| [tools/unlock_cfw3212.py](tools/unlock_cfw3212.py) | Root unlock script (config restore with injected root hash) |

---

## Firmware Versions

| Version | Build Date | Modem FW | Qualcomm Base |
|---------|------------|----------|---------------|
| USC_1.1.79.0 | 2023-07-20 | RG520NNADAR03A03M4G_03.001.03.001 | LE.UM.6.3.6.r1-02600-SDX65.0 |
| USC_1.1.99.0 | 2023-11-15 | RG520NNADAR03A03M4G_03.001.03.001 | LE.UM.6.3.6.r1-02600-SDX65.0 |
| USC_1.2.24.0 | 2024-03-08 | RG520NNADAR03A03M4G_03.002.03.002 | LE.UM.6.3.6.r1-02600-SDX65.0 |

Same Qualcomm base build across all three versions — firmware changes are at the Casa/OpenCPU application layer. 1.2.24.0 uses a newer modem firmware build (03.002 vs 03.001).

---

## LAN Behavior

The device's LAN IP changes depending on SIM / WAN state:

| State | LAN IP | Notes |
|-------|--------|-------|
| No SIM / unregistered | `192.168.1.1` | Standard LAN gateway |
| T-Mobile SIM active | `192.0.0.1` (or `192.0.0.2` client) | DS-Lite (IPv4-in-IPv6); WAN IPv4 is a shared address |
| Any state | `fe80::<EUI-64>%<interface>` | IPv6 link-local — always works |

**Always use IPv6 link-local for SSH.** The IPv4 address changes with SIM state and the SSH port may be filtered. The IPv6 link-local address is derived from the MAC address and never changes. Find it with:

```sh
# macOS
ipconfig getv6router en8    # returns the modem's link-local directly

# Linux
ip -6 route show dev eth0 | grep 'fe80'
```

---

## Default Web UI Credentials

| Field | Value |
|-------|-------|
| URL | `http://192.168.1.1` (or `http://192.0.0.1` with T-Mobile SIM) |
| Username | `admin` |
| Password | `roofclimberabove` |

These credentials are U.S. Cellular carrier defaults — not generated per-unit. All units ship with the same password.

---

## Root Unlock

SSH is disabled by default. The unlock path uses only Ethernet access (no special hardware, no UART).

**Vector 1: Config restore with injected root hash** — [`tools/unlock_cfw3212.py`](tools/unlock_cfw3212.py)

The web UI allows uploading a device configuration backup. The backup format contains a shadow-format root password hash. Uploading a modified backup with a known hash unlocks SSH root access.

Requires stock Python 3.x (no additional packages) on a Windows, macOS, or Linux host connected to the device via Ethernet.

```sh
python3 tools/unlock_cfw3212.py    # authenticates, injects hash, enables SSH
```

Tested on USC_1.1.79.0, USC_1.1.99.0, and USC_1.2.24.0.

---

## Management Interfaces

### RDB (Runtime Database)

RDB is the central configuration and state store on the CFW-3212. Every setting — from cellular band policy to SSH access to GPS coordinates — is an RDB key-value pair. The web UI, TR-069, LwM2M, and all system daemons read and write RDB; it is the single source of truth for the device.

RDB keys use a dotted hierarchy (e.g., `wwan.0.system_network_status.lte_ca_pcell.pci`). Values persist to NAND flash across reboots via the CDCS (Configuration Data and Control Service) subsystem. Factory defaults live in `/etc/cdcs/conf/default.conf`; runtime overrides are stored in `/usr/local/cdcs/conf/`.

**CLI tools** (available over SSH):

```sh
rdb_get <key>              # read a single key
rdb_set <key> <value>      # write a single key (persists to flash)
rdb_get -A                 # dump all keys (thousands of entries)
```

**Template system:** RDB changes trigger reactive templates in `/etc/cdcs/conf/mgr_templates/`. For example, writing `admin.local.ssh_enable=1` causes `40_ssh.template` to start sshd; writing firewall rules triggers `lan_admin_firewall.template` to rebuild iptables chains via `QCMAP_ConnectionManagerd`.

**Settings backup format:** The web UI backup/restore exports RDB as a `.cfg` file in `key;value` text format. Non-encrypted RDB values can be modified in this file and applied via restore — this is the basis of the root unlock method.

**Key RDB namespaces:**

| Prefix | Contents |
|--------|----------|
| `wwan.0.*` | Cellular status, signal metrics, band config |
| `service.*` | Service enable/disable (SSH, TR-069, LwM2M, ADB, syslog, etc.) |
| `admin.*` | User credentials, access control (local/remote HTTP, HTTPS, SSH) |
| `sensors.gps.0.*` | GPS fix data, satellite status |
| `link.profile.*` | WWAN APN profiles |
| `telnet.passwd.*` | Root password hash (synced to `/etc/shadow` on boot by `passwd_restore.sh`) |

### Web UI

Lighttpd-backed web application at port 80 (`http://192.168.1.1`). All reads and writes go through RDB via a Lua JSON API:

```
GET  /cgi-bin/jsonSrvr.lua?req={"getObject":[...],"csrfToken":"...","queryParams":{}}
POST /cgi-bin/jsonSrvr.lua  data={"setObject":[{"name":"...","field":value}],"csrfToken":"..."}
```

The CSRF token is a session value from a prior authenticated request. Object handlers (e.g., `objTR069Cfg.lua`, `objAdminCredentials.lua`, `objRestoreSettings.lua`) map JSON object names to RDB keys — not to AT commands.

Notable web API objects: `CellularStatus`, `SignalStrength`, `LteServingCell`, `Nr5gServingCell`, `GpsFix`, `LwM2MStatus`, `TR069Status`, `FirewallSettings`, `LogSettings`.

### SSH (post-unlock)

Port 22. Root access. Enable via `System → Access Control` in the web UI, or directly via RDB:

```sh
rdb_set admin.local.ssh_enable 1
```

**Note:** SSH authorized keys live on tmpfs at `/var/run/ssh-keys/root/authorized_keys` and are wiped on every reboot. Key auth must be re-established after each power cycle.

### Remote Syslog

Configure via `System → Log → System log settings` or via RDB:

```sh
rdb_set service.syslog.option.remote "192.168.x.x:514"
rdb_set service.syslog.option.logtoremote 1
rdb_set service.syslog.option.trigger 1    # restart syslogd
```

### TR-069 / CWMP

Active TR-069 client (`cwmp-crd` + `cwmp-xmpp`). ACS URL, credentials, and periodic inform interval are configured via RDB keys under `service.tr069.*`. The TR-069 data model maps parameters directly to RDB — for example, `Device.Users.User.1.Password` writes to `admin.user.root.encrypted`, and `LANSSHEnable` / `WANSSHEnable` control SSH access.

TR-069 XMPP (connection requests via XMPP server) is present and active on the USCC deployment.

### LwM2M

LwM2M client (`lwm2m_client` and `LwM2M_Client_App`) is present for device management, controlled via `service.lwm2m.*` RDB keys. The USCC deployment uses LwM2M for carrier-side configuration and remote management. The LwM2M endpoint name and PSK are stored in RDB in cleartext.

---

## AT Interface

The unit uses Qualcomm OpenCPU architecture. There is no USB AT command port exposed to the host. AT commands are available internally via `/dev/smd7` over SSH:

```sh
ssh root@<link-local> 'echo -e "AT+CEREG?\r" > /dev/smd7 && sleep 0.5 && cat /dev/smd7'
```

`ql_nw_service` (Quectel network service) may claim `/dev/smd7`. If AT commands return no response, disable the Quectel network bridge and restart `port_bridge`:

```sh
AT+QETH="eth_at","disable"
# or
systemctl restart port_bridge
```

AT command forwarding path: SSH → `/dev/smd7` → modem `ds_task`.

**Note:** `AT+CLAC` returns ERROR on this platform.

---

## Cellular Capabilities

| Feature | Value |
|---------|-------|
| Technology | 3GPP Release 16 |
| Modes | LTE, 5G NSA (EN-DC Option 3x), 5G SA (Option 2) |
| LTE bands (USCC profile) | 2, 4, 5, 12, 48 |
| NR5G NSA bands (USCC profile) | 2, 5, 12, 71, 77 |
| CBRS | Yes (Band 48) |
| Cell lock | Yes (LTE: EARFCN+PCI; NR: NR-ARFCN+PCI) |
| Carrier aggregation info | Yes |
| Max APN profiles | 6 |

Active carrier profile: `Commercial-USCC` (MBN). Profile controls band restrictions, APN provisioning, and VoLTE settings.

---

## Signal Monitoring

The web UI field test screen and JSON API expose per-frame radio metrics. Available via SSH AT commands or directly via the `SignalStrength` / `CellularStatus` API objects:

- LTE: RSRP, RSRQ, SINR, EARFCN, PCI, CQI, CA cells
- NR5G: NR-ARFCN, PCI, RSRP, RSRQ, SINR, SCS, SSB index
- GPS: lat/lon/alt/accuracy (GNSS engine active when geofence or service assurance is configured)
- Neighbor cell list (BPLMN scans)

---

## Security Notes

- **Default web password `roofclimberabove`** is the same across all units — not per-unit generated
- **TR-069 and LwM2M** are both active; the carrier has remote management capability
- **Root filesystem is read-only** (squashfs + overlayfs); persistent changes go to `/data` (rw)
- **QXDM** over Ethernet is documented in the carrier's installer guide (section 8.11) — the modem DM port is accessible to authenticated users with the appropriate QXDM software
- Carrier remote management blocking procedure: [guide_block_carrier_remote_mgmt.md](guide_block_carrier_remote_mgmt.md)
