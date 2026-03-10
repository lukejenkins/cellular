# CFW-3212 Web UI Screenshots

Screenshots of the Casa Systems CFW-3212 web management interface at `http://192.168.1.1`. Captured from firmware USC 1.1.99.0. Sensitive fields (IMEI, ICCID, IMSI, serial numbers, cell IDs, coordinates, MAC addresses, etc.) have been redacted.

## Status Tab

| File | Page | Description |
|------|------|-------------|
| [screenshot0001.png](screenshot0001.png) | Status | Status page overview (all sections collapsed) |
| [screenshot0002.png](screenshot0002.png) | Status > System Information | Device name, model, HW version, serial, firmware version, module model/FW, IMEI |
| [screenshot0003.png](screenshot0003.png) | Status > Cellular Connection Status | SIM status, signal strength, registration, provider, roaming, allowed bands, current band, RAT, coverage, service option |
| [screenshot0004.png](screenshot0004.png) | Status > WWAN Connection Status | Profile name, WWAN IP, DNS, APN, MTU, connection uptime, max/current DL/UL speeds |
| [screenshot0005.png](screenshot0005.png) | Status > Advanced Status | 4G LTE and 5G NR detailed metrics — ECGI, eNodeB, Cell ID, PCI, EARFCN, RSRP, RSRQ, SINR, CQI, Scell info, NR ARFCN, SS-RSRP, SS-RSRQ, SS-SINR, NR CQI, SSB index |
| [screenshot0006.png](screenshot0006.png) | Status > 4G LTE Neighbouring Cell Information | Neighbour cell table: PCI, EARFCN, RSRP, RSRQ, RAT |
| [screenshot0007.png](screenshot0007.png) | Status > 5G NR Neighbouring Cell Information | Neighbour cell table: PCI, NR ARFCN, SS-RSRP, SS-RSRQ, Type (Serving/Neighbour), Role, SSB ARFCN, RAT |
| [screenshot0008.png](screenshot0008.png) | Status > LAN | LAN IP/mask, MAC address, Ethernet port status, IP passthrough host MAC |
| [screenshot0009.png](screenshot0009.png) | Status > Bluetooth | Bluetooth LE MAC address |
| [screenshot0010.png](screenshot0010.png) | Status > GPS | Latitude, longitude, altitude, height of geoid, PDOP, horizontal/vertical uncertainty |

## Networking Tab

| File | Page | Description |
|------|------|-------------|
| [screenshot0011.png](screenshot0011.png) | Networking > Cellular settings > Wireless WAN profiles | 6 WWAN profile slots — on/off, profile name, APN, IP passthrough toggle, LAN/VLAN mapping, default route |
| [screenshot0012.png](screenshot0012.png) | Networking > Cellular settings > Operator setting | Operator selection mode (auto/manual), scan button, roaming control on/off |
| [screenshot0013.png](screenshot0013.png) | Networking > LAN > LAN | LAN IP address, subnet mask, hostname |
| [screenshot0014.png](screenshot0014.png) | Networking > LAN > DHCP | DHCP on/off, start/end range, lease time. Note: DHCP must be enabled when IP passthrough is active |
| [screenshot0015.png](screenshot0015.png) | Networking > LAN > VLAN | VLAN enable/disable, VLAN rules table (name, interface, address, subnet, DHCP range, admin access, enabled) |
| [screenshot0016.png](screenshot0016.png) | Networking > Firewall > NAT | Port forwarding list (name, profile, protocol, public port, local IP, local port, enable) |
| [screenshot0017.png](screenshot0017.png) | Networking > Firewall > MAC Whitelist | MAC filtering enable/disable, MAC whitelist table |
| [screenshot0018.png](screenshot0018.png) | Networking > Routing > Static | Static routing list and active routing table (destination, gateway, netmask, flags, metric, interface) |
| [screenshot0019.png](screenshot0019.png) | Networking > Service assurance | Network connectivity test — WWAN profile selection, DNS test, ping test, web test (GET/POST), results |

## Services Tab

| File | Page | Description |
|------|------|-------------|
| [screenshot0020.png](screenshot0020.png) | Services > Network time (NTP) | Timezone settings, NTP on/off |
| [screenshot0021.png](screenshot0021.png) | Services > TR-069 | TR-069 enable/disable, last inform status, device info (manufacturer: Casa Systems, OUI: F8CA59, model: CFW-3212-02, product class: CFW3212 Series) |
| [screenshot0022.png](screenshot0022.png) | Services > DNS server | Primary/secondary DNS, cache size, local TTL |
| [screenshot0023.png](screenshot0023.png) | Services > GPS > GPS configuration | GPS enable/disable, GPS status (fix data, DOP, satellite count), satellite status table (system, SV ID, health, status, SNR, elevation, azimuth) |
| [screenshot0024.png](screenshot0024.png) | Services > GPS > GPS configuration (cont.) | Satellite status table continued — GPS and GLONASS SVs with track/search status and SNR values |
| [screenshot0025.png](screenshot0025.png) | Services > GPS > Assisted GPS | A-GPS enable/disable |
| [screenshot0026.png](screenshot0026.png) | Services > OMA-LWM2M (disabled) | LwM2M endpoint name, enable/disable toggle (shown disabled) |
| [screenshot0027.png](screenshot0027.png) | Services > OMA-LWM2M (enabled) | Full LwM2M config — endpoint, enable, management WWAN profile, port, override server settings (URI, lifetime, bootstrap, queue mode, security mode, PSK) |

## System Tab

| File | Page | Description |
|------|------|-------------|
| [screenshot0028.png](screenshot0028.png) | System > Log > System log | System log download/clear buttons |
| [screenshot0029.png](screenshot0029.png) | System > Log > System log settings | Log capture level, volatile log buffer size, non-volatile log on/off and file size, remote syslog server |
| [screenshot0030.png](screenshot0030.png) | System > Ping diagnostics | Ping test config — host, repetitions, timeout, block size, DSCP, interface, protocol version; results section |
| [screenshot0031.png](screenshot0031.png) | System > System configuration > Restore factory defaults | Factory reset with warning text |
| [screenshot0032.png](screenshot0032.png) | System > System configuration > Web server setting | HTTPS/HTTP toggles, TLS certificate management (generate or upload) |
| [screenshot0033.png](screenshot0033.png) | System > System configuration > Web UI credentials | Username (admin), password change form |
| [screenshot0034.png](screenshot0034.png) | System > System configuration > Settings backup/restore | Password-protected config backup download, config restore upload |
| [screenshot0035.png](screenshot0035.png) | System > System configuration > Runtime configuration | Runtime config upload (RDB, Cert, MBN, EFS sections) |
| [screenshot0036.png](screenshot0036.png) | System > Firmware upgrade | Firmware file upload, reset-to-default toggle |
| [screenshot0037.png](screenshot0037.png) | System > Access control | Remote access (HTTP, HTTPS, SSH, Ping) and local access (HTTP, HTTPS, SSH) toggles |
| [screenshot0038.png](screenshot0038.png) | System > Reboot | Reboot button with warning |
| [screenshot0039.png](screenshot0039.png) | System > Field test | LTE PCell info (PCI, EARFCN, band, bandwidth), LTE SCell info, 5G NR serving cell info (Cell ID, DL/UL ARFCN, band, band type, DL/UL BW, MIMO) |
| [screenshot0040.png](screenshot0040.png) | System > Encrypted debuginfo | Generate and download encrypted debug bundle |
| [screenshot0041.png](screenshot0041.png) | System > Encrypted debuginfo (generating) | "Please wait" progress state |
| [screenshot0042.png](screenshot0042.png) | System > Encrypted debuginfo (complete) | "Success!" confirmation after generation |
