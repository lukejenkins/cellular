# CFW-3212 Flash Storage Layout

Flash storage map for the Casa Systems CFW-3212 (USC-CFW8NA), reverse-engineered from firmware analysis and live device inspection.

## Flash Hardware

| Parameter | Value |
|-----------|-------|
| Chip | SK Hynix H27S4G8F2EKPB4 |
| Type | SLC NAND, ONFI compliant |
| Capacity | 512 MB |
| Page size | 4096 bytes (4 KB) |
| Erase block | 262,144 bytes (256 KB) |
| OOB per page | 256 bytes |
| ECC | BCH 4-bit |
| Bus width | 8-bit |
| Controller | Qualcomm QPIC (SDX65 / SDX-Lemur) |

Source: `dmesg` output from `msm_nand_flash_onfi_probe`.

## Partition Scheme

The device uses a **dual A/B slot** partition layout (identified as `aurus_ab` with `sdxlemur` override in the build configuration). The bootloader is Qualcomm Little Kernel (LK, target `mdm9640`), using Fastboot as the flashing protocol.

### Complete MTD Partition Table

Captured from `/proc/mtd` on a live device running USC 1.2.24.0. All 32 partitions are shown. The entire 512 MB NAND is allocated — there is no unpartitioned space.

| MTD | Name | Size | Purpose |
|-----|------|------|---------|
| mtd0 | `sbl` | 2.5 MB | Secondary bootloader |
| mtd1 | `mibib` | 1 MB | NAND partition table (MIBIB) |
| mtd2 | `efs2` | 22 MB | Modem EFS (NV items, RF calibration, MBN profiles) |
| mtd3 | `rawdata` | 2.5 MB | Factory calibration data |
| mtd4 | `sys_rev` | 3.5 MB | System revision info |
| mtd5 | `cust_info` | 2.5 MB | U-Boot environment (A/B slot metadata, 32 KB used) |
| mtd6 | `tz` | 2 MB | TrustZone firmware |
| mtd7 | `tz_devcfg` | 1.25 MB | TZ device configuration |
| mtd8 | `ddr` | 1.25 MB | DDR training parameters |
| mtd9 | `apdp` | 1.25 MB | Debug policy |
| mtd10 | `xbl_config` | 1.25 MB | XBL configuration |
| mtd11 | `xbl_ramdump` | 1 MB | RAM dump support |
| mtd12 | `multi_image` | 1.25 MB | Multi-image |
| mtd13 | `multi_image_qti` | 1 MB | QTI multi-image |
| mtd14 | `aop` | 1.25 MB | Always-On Processor firmware |
| mtd15 | `qhee` | 1.25 MB | Qualcomm Hypervisor (EL2) |
| mtd16 | `abl` | 1.25 MB | Android bootloader |
| mtd17 | `uefi` | 3.25 MB | UEFI firmware |
| mtd18 | `toolsfv` | 1.25 MB | Tools FV |
| mtd19 | `loader_sti` | 1.25 MB | Loader STI |
| mtd20 | `logfs` | 1 MB | Log filesystem |
| mtd21 | `devinfo` | 1.25 MB | Device info |
| mtd22 | `sec` | 1.25 MB | Security partition |
| mtd23 | `multi_fota` | 7.25 MB | FOTA (firmware over-the-air) |
| mtd24 | `misc` | 1.25 MB | Misc/recovery flags |
| mtd25 | `ipa_fw` | 1.25 MB | IPA (network accelerator) firmware |
| mtd26 | `usb_qti` | 1.25 MB | USB QTI configuration |
| mtd27 | `boot_a` | 12 MB | Linux kernel, slot A |
| mtd28 | `boot_b` | 12 MB | Linux kernel, slot B |
| mtd29 | `system_a` | 123 MB | Root filesystem UBI, slot A |
| mtd30 | `system_b` | 123 MB | Root filesystem UBI, slot B |
| mtd31 | `usrdata` | 173.75 MB | Persistent user data UBI |

**Size breakdown:** SoC/bootloader partitions (mtd0–mtd26) ~53 MB, boot images ~24 MB, system slots ~246 MB, usrdata ~174 MB. Note: `oemdata` is not a separate MTD partition on this hardware revision — the init scripts fall back to `/usrdata/cdcs` for Casa OEM configuration.

### UBI Volume Map

UBI volumes are created within the MTD partitions listed above. The active system slot (A or B) is attached as `ubi0` at boot.

| UBI Device | Volume Name | Mount Point | Filesystem | Access | Description |
|------------|-------------|-------------|------------|--------|-------------|
| `ubi0` | rootfs | `/` | SquashFS (via ubiblock) | RO | Root filesystem from active slot |
| `ubi0` | modem | `/firmware/` (via `ubi0_101`) | ext4 (via ubiblock) | RO | Modem baseband firmware (~52 MB) |
| `ubi0` | (modem_pr) | `/firmware/image/modem_pr` (via `ubi0_102`) | ext4 (via ubiblock) | RO | Modem provisioning data |
| `ubi0` | volatile | `/rw/local` | UBIFS | RW | Writable overlay backing store |
| `ubi0` | (local) | (block device, `ubi0_110`) | ubiblock | RO | Local access partition |
| `ubi2` | usrdata | `/usrdata` | UBIFS | RW | Persistent user data (~144 MB usable) |
| `ubi3` | oemdata | `/oemdata` | UBIFS | RW | Casa OEM data; bind-mounted to `/usr/local/cdcs` |

## Filesystem Layering

The root filesystem uses a **SquashFS + OverlayFS** design:

```
┌─────────────────────────────────────────────┐
│              OverlayFS mounts               │
│  /etc  = lower:/etc + upper:/rw/local/etc   │
│  /data = lower:/data + upper:/rw/local/data │
├─────────────────────────────────────────────┤
│  /rw/local  ←  ubi0:volatile (UBIFS, RW)    │
├─────────────────────────────────────────────┤
│  /  ←  SquashFS from active system slot     │
│       (read-only, from ubi0 rootfs volume)  │
└─────────────────────────────────────────────┘
```

1. **Lower layer:** Read-only SquashFS root mounted from the active system slot's `rootfs` UBI volume
2. **Upper layer:** Writable UBIFS volume (`ubi0:volatile`) mounted at `/rw/local`
3. **OverlayFS:** `/etc` and `/data` are overlay-mounted with upper dirs under `/rw/local/`
4. **Factory reset:** Deletes `/rw/local/data`, `/rw/local/etc`, and `/rw/local/lib`, then clears `/usrdata/cdcs`

### Bind Mounts

| Source | Target | Notes |
|--------|--------|-------|
| `/usrdata/cache` | `/cache` | Persistent cache |
| `/usrdata/systemrw` | `/systemrw` | System read-write data |
| `/usrdata/persist` | `/persist` | Persistent storage |
| `/oemdata/cdcs` | `/usr/local/cdcs` | Casa device configuration (falls back to `/usrdata/cdcs` if oemdata unavailable) |

## A/B Boot Slot Management

The `abctl` utility manages dual-slot booting. The active slot is selected via environment variables stored in the `cust_info` MTD partition.

### Per-Slot Environment Variables

| Variable | Purpose |
|----------|---------|
| `BOOT_X_ACTIVE` | Slot is marked active |
| `BOOT_X_SUCCESS` | Slot booted successfully |
| `BOOT_X_UNBOOT` | Slot is marked unbootable |
| `BOOT_X_MAX_RTY_CNT` | Maximum boot retry count |
| `BOOT_X_PRI` | Slot priority |
| `BOOT_X_FW_VERSION` | Casa firmware version in this slot |
| `BOOT_X_MODULE_VERSION` | Quectel module firmware version |

(Where `X` is `A` or `B`.)

The kernel command line carries `androidboot.slot_suffix=_a` or `_b` to identify the running slot.

### Environment Partition Details

| Parameter | Value |
|-----------|-------|
| MTD partition | `cust_info` |
| Environment size | 32 KB (`0x8000`) |
| Storage type | `nhrf` (NAND-resident hardware runtime firmware) |
| Redundancy | Up to 64 environment copies across 8 erase blocks |

## Firmware Update Flow

OTA updates are handled by `/sbin/flashtool`, which writes to the **inactive** slot:

1. Detect current active slot via `abctl --boot_slot`
2. Verify firmware archive signature against `/etc/cdcs/fw/keys/*.pem`
3. Verify product model compatibility
4. Flash system image: `ubiformat` the inactive `system_X` partition with `sdxlemur-sysfs.ubi`
5. Flash kernel image: `flash_erase` + `nandwrite` the inactive `boot_X` partition with `boot.img`
6. Update slot metadata via `abctl --set_active` to switch to the newly written slot
7. Optionally trigger factory reset if `fw.incompat_ver` requires it

For `--factory` installs, both slots are flashed with the same image.

## Free Space (Live Measurement)

Captured from a live device running USC 1.2.24.0 (March 2026):

| Mount | Total | Used | Free | Use% |
|-------|-------|------|------|------|
| `/usrdata` | 144.4 MB | 1.8 MB | 137.9 MB | 1% |
| `/rw/local` (volatile) | 7.1 MB | 176 KB | 6.5 MB | 3% |

The `usrdata` UBIFS volume provides ~144 MB usable out of the 174 MB MTD partition after UBI overhead (bad block reserves of 40 PEBs = 10 MB, plus journal and metadata). The `volatile` overlay backing `/etc` and `/data` changes is limited to ~7 MB.

On a near-stock device, `/usrdata` is almost empty (~1% used). Most of that 1.8 MB is the Casa device configuration store (`/usrdata/cdcs`), logs, and persist data.

## Compatibility Notes

- The Casa partition layout is **incompatible** with stock Quectel RG520N firmware packages. Boot partition sizes differ (12 MB vs 15 MB), and Casa adds/removes partitions compared to stock.
- Casa secure boot (OEM_ID `0x01dc`) prevents use of Quectel-signed EDL/firehose programmers for raw flash access. Live MTD dumps via SSH (`/dev/mtdX`) are the only viable backup method.
- The `V_BASEFS_FILE_UBI_NAMES` build variable confirms the base UBI volume set: `rootfs modem usrdata`.

## Sources

Reverse-engineered from firmware versions USC 1.1.79.0, 1.1.99.0, and 1.2.24.0, plus live device inspection. Key files analyzed:

- `/etc/variant.sh` — build-time storage parameters
- `/sbin/flashtool` — firmware update tool
- `/sbin/init-overlay` — early-boot UBI/OverlayFS setup
- `/etc/initscripts/firmware-ubi-mount.sh` — firmware and usrdata mounting
- `/etc/initscripts/casa-ubi-mount.sh` — OEM partition mounting
- `/usr/sbin/abctl` — A/B boot slot controller
- `/usr/sbin/env_info.sh` — environment partition configuration
- `dmesg` kernel log — NAND chip identification and UBI attachment
