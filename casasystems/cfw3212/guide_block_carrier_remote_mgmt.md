# Blocking Carrier Remote Management on the Casa Systems CFW-3212

> Copyright 2026 Luke Jenkins. Licensed under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).
>
> This is a living document and may be updated as new firmware versions or infrastructure changes are observed. Check [github.com/lukejenkins/cellular](https://github.com/lukejenkins/cellular) for the latest version.

## Overview

The Casa Systems CFW-3212 is a 5G fixed wireless access (FWA) CPE built on the Qualcomm SDX62 platform running embedded Linux. Like most carrier-managed CPEs, it ships with multiple remote management channels that allow the carrier and device manufacturer to push firmware updates, change configuration, reboot, or factory-reset the device without physical access.

When these devices are repurposed — purchased secondhand, decommissioned from a carrier fleet, or used for research — these management channels become a liability. This guide explains what they are, how they work, and how to disable them.

All configuration values referenced below are from `default.conf` on the read-only squashfs root filesystem (`/etc/cdcs/conf/default.conf`). They have been verified identical across firmware versions 1.1.79.0, 1.1.99.0, and 1.2.24.0 (USCC variants).

## Remote Management Channels

### TR-069 (CWMP)

**What it is:** TR-069 is the Broadband Forum's CPE WAN Management Protocol (CWMP). It is the industry standard for carrier remote management of residential gateways, modems, and fixed wireless devices. Nearly every ISP-provided home router uses TR-069.

**How it works:** The device runs a TR-069 client (`cwmpd`) that periodically polls an Auto Configuration Server (ACS) operated by the carrier over HTTPS. The ACS can also initiate sessions on demand via XMPP connection requests (ports 5222/5223) or a connection request daemon (`cwmp-crd`) listening for HTTP callbacks.

**What the carrier can do via TR-069:**
- Push firmware updates
- Read or write any configuration parameter exposed in the device's data model
- Reboot the device
- Trigger a factory reset
- Run diagnostics (ping, traceroute, speed test)
- Retrieve logs

**Configuration in `default.conf`:**

The `default.conf` file uses a layered structure. A base section near the top of the file sets the platform defaults, and a vendor-specific USCC override block later in the file overrides select values. For TR-069:

| Key | Base default | USCC override |
|-----|-------------|---------------|
| `service.tr069.enable` | `0` | `1` |
| `tr069.server.url` | *(empty)* | `https://remotedevicemgmt.uscellular.com:7547/live/CPEManager/CPEs/genericTR69` |
| `tr069.server.username` | `acs` | *(empty)* |
| `tr069.server.password` | `acs` | *(empty)* |
| `tr069.server.periodic.interval` | `600` | *(not overridden — 600s)* |

The USCC override enables TR-069 and points it at the carrier's ACS. The client begins polling within seconds of cellular network registration, with a 600-second periodic inform interval. Additionally, the connection request daemon (`cwmp-crd`) and XMPP client (`cwmp-xmpp`) allow the ACS to initiate sessions without waiting for the polling interval.

Uses HTTPS with server-side TLS validation against the system CA bundle (`/etc/ssl/certs/`); no client certificate is required.

**Traffic routing:** TR-069 traffic is routed out the cellular WAN interface using Linux cgroups and iptables mangle rules on a dedicated management traffic class (`net_cls_tr069`, chain `mangle_OUTPUT_TR069`). This means that if you have a valid cell subscription, the management traffic **bypasses any upstream LAN router or firewall entirely**. You cannot block it from the LAN side; it must be blocked on the device itself.

### OMA LwM2M (FOTA)

**What it is:** OMA Lightweight M2M (LwM2M) is a device management protocol designed for IoT and constrained devices. On the CFW-3212, it is used specifically for firmware-over-the-air (FOTA) updates.

**How it works:** The device runs an LwM2M client that registers with a Remote Device Management (RDM) server over CoAP with DTLS (CoAPS, port 5684). Authentication uses a pre-shared key (PSK) that is hardcoded in the device's default configuration. The RDM server can push firmware packages to the device independently of TR-069.

**Configuration in `default.conf`:**

| Key | Base default | USCC override |
|-----|-------------|---------------|
| `service.lwm2m.enable` | `0` | `1` |
| `service.lwm2m.override.server-uri` | *(empty)* | `coaps://rdm.casa-systems.com:5684` |
| `service.lwm2m.override.psk-id` | *(empty)* | `uscc_fota` |
| `service.lwm2m.override.psk-key` | *(empty)* | `3c8ec28b5b2f67f77cc17e7206219bc6a6aa44c1acde98011c9c793a5dc2f1da` |

The LwM2M endpoint registers with the RDM server as `uscc_fota:<serial_number>`.

**Notable detail:** On USCC-branded CFW-3212 units, the LwM2M FOTA server is operated by **Casa Systems** (the device manufacturer), not by the carrier directly. This means the manufacturer retains an independent firmware update path even if the carrier decommissions their ACS infrastructure.

**Traffic routing:** Like TR-069, LwM2M traffic routes out the cellular interface on its own traffic class (chain `mangle_OUTPUT_LWM2M` in firmware versions 1.1.99.0+), bypassing the LAN.

### XMPP (TR-069 Connection Requests)

**What it is:** XMPP (ports 5222/5223) is used as a side channel for TR-069. Rather than waiting for the device to poll on its next interval, the ACS can send an XMPP message that triggers the device to initiate an immediate CWMP session. This gives the carrier near-real-time control.

## Why a LAN Firewall Is Not Sufficient

Both TR-069 and LwM2M are configured to route through the cellular WAN interface using dedicated cgroup-based traffic classes with iptables mangle rules. These traffic classes ensure management traffic takes the cellular path regardless of default routing.

An upstream router, even with strict egress filtering, will never see this traffic — it exits on the cellular radio, not the Ethernet port. Blocking must be performed on the CPE itself.

## Why Device-Level RDB Changes Alone Aren't Sufficient

Both services' configuration lives in `default.conf` on the read-only squashfs rootfs. RDB values set via `rdb set` persist to flash and survive reboots — but a factory reset (which TR-069 can trigger remotely before you disable it) restores `default.conf` defaults, including `service.tr069.enable=1` and the LwM2M PSK. This is why the iptables layer matters: even if a factory reset restores the RDB, the DROP rules prevent the management clients from reaching their servers.

## Are These Servers Still Active?

Before going through the blocking procedure, you can verify from any machine whether the management infrastructure is still maintained. If the servers are decommissioned, blocking is less urgent (though still good practice — infrastructure can be brought back online).

**TR-069 ACS — `remotedevicemgmt.uscellular.com`**

```sh
# DNS resolution — expect AWS ELB IPs if still active
dig +short remotedevicemgmt.uscellular.com

# TCP port check
nc -z -w 5 remotedevicemgmt.uscellular.com 7547 && echo "OPEN" || echo "CLOSED"

# TLS certificate — check issuer and expiry date
echo | openssl s_client -connect remotedevicemgmt.uscellular.com:7547 \
  -servername remotedevicemgmt.uscellular.com 2>/dev/null | \
  openssl x509 -noout -subject -issuer -dates
```

**LwM2M RDM — `rdm.casa-systems.com`**

```sh
# DNS resolution
dig +short rdm.casa-systems.com

# UDP port check (unreliable — no ICMP unreachable ≠ service is listening)
nc -z -w 5 -u rdm.casa-systems.com 5684 && echo "OPEN" || echo "CLOSED"
```

As of March 2026, the TR-069 ACS resolves to an AWS Elastic Load Balancer in `us-east-2`, port 7547 is open, and the TLS certificate is valid (issued by DigiCert to US Cellular, Knoxville, TN; valid 2025-04-15 to 2026-04-14). The server returns HTTP 403 to unauthenticated requests — it is actively maintained. The LwM2M RDM resolves to an AWS IP in `us-west-2` but does not respond to DTLS probes from non-cellular source IPs, so its status is inconclusive from an external vantage point.

## Blocking Procedure

### Prerequisites

- Root SSH access to the CFW-3212
- These steps should be performed **before inserting a carrier SIM** or immediately after — TR-069 activates within seconds of network registration

### Step 1 — Disable Services via RDB

The CFW-3212 stores its runtime configuration in an RDB (Runtime Database). RDB is the central configuration store used by NetComm/Casa Systems embedded Linux platforms. It is a key-value database backed by the `luardb` Lua C library and managed by the `rdb_manager` daemon.

**Architecture:**

At boot, `rdb_manager` loads three configuration layers in order:

1. `/etc/cdcs/conf/default.conf` — read-only base defaults on the squashfs rootfs (includes vendor override blocks)
2. `/usr/local/cdcs/conf/system.conf` — persistent user/runtime overrides on the writable overlay
3. `/usr/local/cdcs/conf/override.conf` — additional overrides

The merged result is exposed via shared memory. All system services, the web UI, TR-069 handlers, and the `rdb` CLI read and write configuration through this single store. When a key changes, `rdb_manager` can regenerate service config files from templates (`/etc/cdcs/conf/mgr_templates/`) and restart affected services automatically.

**Persistence:** Each key has flags — the `p` (persist) flag determines whether a value is written back to `system.conf` and survives reboot. The `rdb set` command writes to shared memory immediately (all services see the change in real time). Keys with the persist flag are flushed to `system.conf` periodically or on `SIGHUP` to `rdb_manager`. A factory reset deletes `system.conf`, so all user changes are lost and `default.conf` becomes the sole effective configuration again.

**CLI operations beyond get/set:** The `rdb` tool also supports `wait` (subscribe to key changes — this is how `cwmpd-wrap.sh` monitors `service.tr069.enable`), `cas` (compare-and-swap for atomic locking), `invoke` (RPC-style service calls), `dump` (all keys with flags and values), and `list` (key enumeration with pattern matching).

Values set via `rdb set` persist to flash and survive reboots (but not a full factory reset, which restores defaults from the read-only squashfs).

```sh
# Disable TR-069 and clear server credentials
rdb set service.tr069.enable 0
rdb set tr069.server.url ''
rdb set tr069.server.username ''
rdb set tr069.server.password ''

# Disable LwM2M and clear PSK credentials
rdb set service.lwm2m.enable 0
rdb set service.lwm2m.override.enable 0
rdb set service.lwm2m.override.server-uri ''
rdb set service.lwm2m.override.psk-key ''
rdb set service.lwm2m.override.psk-id ''
```

Setting `service.tr069.enable=0` causes the TR-069 wrapper script (`cwmpd-wrap.sh`) to exit its watch loop and kill the running daemons. Kill any remaining processes explicitly:

```sh
killall cwmpd.lua cwmp-xfrd.lua cwmp-crd lwm2m_client 2>/dev/null
```

### Step 2 — Add iptables DROP Rules

This is a belt-and-suspenders layer. Even if the RDB values are somehow restored (e.g., by a factory reset), the kernel-level DROP rules prevent the management clients from reaching their servers:

```sh
iptables -I OUTPUT -p tcp --dport 7547 -j DROP   # TR-069 ACS
iptables -I OUTPUT -p udp --dport 5684 -j DROP   # LwM2M CoAPS/DTLS
iptables -I OUTPUT -p tcp --dport 5222 -j DROP   # XMPP
iptables -I OUTPUT -p tcp --dport 5223 -j DROP   # XMPP (TLS)
```

### Step 3 — Persist Across Reboots

RDB values persist across reboots on their own, but the iptables rules do not. To restore them automatically at boot, write a blocking script to the `/usrdata` partition (UBIFS, survives reboots and `/etc` overlay resets) and install a systemd service unit.

**Create `/usrdata/block-carrier-mgmt.sh`:**

```sh
#!/bin/sh
# Block carrier remote management channels (TR-069, LwM2M/FOTA)

# Disable via RDB
rdb set service.tr069.enable 0
rdb set tr069.server.url ''
rdb set tr069.server.username ''
rdb set tr069.server.password ''
rdb set service.lwm2m.enable 0
rdb set service.lwm2m.override.enable 0
rdb set service.lwm2m.override.server-uri ''
rdb set service.lwm2m.override.psk-key ''
rdb set service.lwm2m.override.psk-id ''

# Drop outbound management traffic (idempotent — check before insert)
iptables -C OUTPUT -p tcp --dport 7547 -j DROP 2>/dev/null || \
    iptables -I OUTPUT -p tcp --dport 7547 -j DROP
iptables -C OUTPUT -p udp --dport 5684 -j DROP 2>/dev/null || \
    iptables -I OUTPUT -p udp --dport 5684 -j DROP
iptables -C OUTPUT -p tcp --dport 5222 -j DROP 2>/dev/null || \
    iptables -I OUTPUT -p tcp --dport 5222 -j DROP
iptables -C OUTPUT -p tcp --dport 5223 -j DROP 2>/dev/null || \
    iptables -I OUTPUT -p tcp --dport 5223 -j DROP

logger -t block-carrier-mgmt 'Carrier management channels blocked'
```

```sh
chmod +x /usrdata/block-carrier-mgmt.sh
```

**Create `/etc/systemd/system/block-carrier-mgmt.service`:**

```ini
[Unit]
Description=Block carrier remote management channels (TR-069, LwM2M)
After=network.target
Before=invoke_tr069client.service tr069identity.service

[Service]
Type=oneshot
ExecStart=/usrdata/block-carrier-mgmt.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```

```sh
systemctl enable block-carrier-mgmt.service
```

The `Before=invoke_tr069client.service tr069identity.service` ordering ensures the blocks are applied before systemd starts any TR-069 initialization services, eliminating the race window between boot and blocking.

### Step 4 — Verify

```sh
# No management processes running
ps | grep -E 'cwmp|lwm2m|fota' | grep -v grep

# iptables rules present
iptables -L OUTPUT -n | grep -E '7547|5684|5222|5223'

# RDB values cleared
rdb get service.tr069.enable       # expect: 0
rdb get tr069.server.url           # expect: (empty)
rdb get service.lwm2m.override.psk-key  # expect: (empty)

# No management traffic on wire (watch for 5 seconds)
timeout 5 tcpdump -n -i any 'port 7547 or port 5684 or port 5222 or port 5223'
```

### Step 5 — Reboot and Re-verify

Reboot the device and repeat Step 4 to confirm the systemd service fired correctly. Check the service status:

```sh
systemctl status block-carrier-mgmt.service
```

You should see `Active: active (exited)` with `status=0/SUCCESS`.

## Persistence Matrix

| Event | RDB zeros | iptables rules | Script (/usrdata) | Service (/etc) |
|-------|-----------|---------------|-------------------|----------------|
| Reboot | Survive | Restored by service | Survives | Survives |
| /etc overlay reset | Survive | Restored manually | Survives | Lost — re-install |
| Factory reset (local) | Restored to defaults | Restored by service if unit survives | Survives | Depends on reset scope |
| Firmware update (OTA) | Lost | Lost | Depends on userdata wipe | Lost |

The combination of iptables and RDB provides defense in depth: if a factory reset restores the RDB defaults (re-enabling TR-069), the iptables rules prevent the TR-069 client from reaching the ACS. Since a firmware update requires TR-069 connectivity to deliver the payload, the blocks are self-protecting against the primary threat. If the service unit is lost (e.g., `/etc` overlay reset), manually re-run `/usrdata/block-carrier-mgmt.sh` and re-enable the unit.

## What Not to Remove

- **Firmware signature verification keys** (`/etc/cdcs/fw/keys/usc_cfw8na_01_public.pem`) — RSA public key used by `flashtool` and `apply_late_configs.sh` to verify firmware archive signatures before writing to flash. If this directory is empty, `flashtool` exits with "no firmware keys found" and refuses to flash anything. Removing it prevents the carrier from pushing firmware, but also prevents you from ever applying firmware yourself. The iptables approach is strictly better.

- **System CA certificate bundle** (`/etc/ssl/certs/`) — Removing this would break TR-069's TLS connection, but would also break all other HTTPS on the device. Unnecessary when iptables blocks the destination ports.

- **Local web service certificates** (`/etc/authenticate/{ca,server}.{crt,key}`) — These serve the local NetComm Device Agent web interface (OWA-NIT) on the LAN, port 27068. They have no role in outbound carrier management.

## Quectel Modem FOTA (Not Addressed)

The CFW-3212 contains a Quectel RG520N-NA modem module running its own baseband firmware. A separate binary (`/usr/bin/quectel_ktfota`) handles over-the-air updates for the modem firmware independently of the TR-069 and LwM2M channels described above.

This channel is **not blocked** by the procedure in this guide. The destination server and port used by `quectel_ktfota` have not been fully characterized. In practice, modem FOTA typically requires the modem to be registered on a carrier network with an active data session, and the update server to have a matching firmware package staged for the device's IMEI or model. The risk is considered low for secondhand/research devices, but it could be investigated further with `strace` or `tcpdump` if desired.

To check whether the Quectel FOTA client is active:

```sh
ps | grep ktfota | grep -v grep
rdb get service.quectel_ktfota.enable 2>/dev/null
```

## Background Reading

- [TR-069 (CWMP) specification](https://www.broadband-forum.org/technical/download/TR-069.pdf) — Broadband Forum
- [OMA LwM2M specification](https://technical.openmobilealliance.org/OMNA/LwM2M/LwM2MRegistry.html) — Open Mobile Alliance
- [QCMAP (Qualcomm Connected Mobile Application Platform)](https://docs.qualcomm.com/) — the networking stack underlying the CFW-3212's traffic routing and cgroup management
