# Casa Systems CFW-3212

5G NR / LTE fixed wireless access CPE. Window-mount outdoor unit with integrated 4x4 MIMO antenna, PoE-powered via a single 2.5Gbit Ethernet port.

Originally designed by NetComm Wireless, acquired by Casa Systems (now DZS).

| Field | Value |
|-------|-------|
| Chipset | Qualcomm SDX62 (via Quectel RG520N-NA OpenCPU) |
| Category | 5G FWA CPE |
| Ethernet | 1x 2.5Gbit (PoE input) |
| Antenna | Integrated 4x4 MIMO WWAN + BLE |
| Enclosure | IP65 (indoor/outdoor window mount) |

**OpenCPU architecture:** The unit runs embedded Linux directly on the SDX62 application processor -- there is no separate host MCU. The Quectel RG520N-NA module is the entire compute platform, not just the modem.

## Contents

| Document | Description |
|----------|-------------|
| [flash_layout.md](flash_layout.md) | NAND flash partition map, UBI volumes, and A/B boot scheme |
| [guide_block_carrier_remote_mgmt.md](guide_block_carrier_remote_mgmt.md) | How to block TR-069 and LwM2M carrier remote management |
| [screenshots/](screenshots/README.md) | Web UI screenshots (42 images, all pages, sensitive fields redacted) |
| [tools/unlock_cfw3212.py](tools/unlock_cfw3212.py) | Root unlock script (config restore with injected root hash) |
