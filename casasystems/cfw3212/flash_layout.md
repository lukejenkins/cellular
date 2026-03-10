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

### Key MTD Partitions

The following partitions are confirmed from `dmesg` kernel log, init scripts, and the `flashtool` utility. MTD numbers are assigned dynamically at boot and may shift between firmware versions; the partition **names** are stable.

| MTD Name | Size | Format | Purpose |
|----------|------|--------|---------|
| `boot_a` | 12 MB | Raw (kernel image) | Linux kernel, slot A |
| `boot_b` | 12 MB | Raw (kernel image) | Linux kernel, slot B |
| `system_a` | 123 MB | UBI | Root filesystem, slot A |
| `system_b` | 123 MB | UBI | Root filesystem, slot B |
| `usrdata` | 173 MB | UBI | Persistent user data |
| `oemdata` | (variable) | UBI | Casa OEM configuration |
| `cust_info` | (small) | Raw | U-Boot environment (32 KB used) |

Additional standard Qualcomm partitions (SBL, TZ, RPM, APPSBL, etc.) are present but not documented here as they are not user-accessible.

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

## Compatibility Notes

- The Casa partition layout is **incompatible** with stock Quectel RG520N firmware packages. Boot partition sizes differ (12 MB vs 15 MB), and Casa adds/removes partitions compared to stock.
- Casa secure boot (OEM_ID `0x01dc`) prevents use of Quectel-signed EDL/firehose programmers for raw flash access. Live MTD dumps via SSH (`/dev/mtdX`) are the only viable backup method.
- The `V_BASEFS_FILE_UBI_NAMES` build variable confirms the base UBI volume set: `rootfs modem usrdata`.

## Sources

Reverse-engineered from firmware versions USC 1.1.79.0, 1.1.99.0, and 1.2.24.0. Key files analyzed:

- `/etc/variant.sh` — build-time storage parameters
- `/sbin/flashtool` — firmware update tool
- `/sbin/init-overlay` — early-boot UBI/OverlayFS setup
- `/etc/initscripts/firmware-ubi-mount.sh` — firmware and usrdata mounting
- `/etc/initscripts/casa-ubi-mount.sh` — OEM partition mounting
- `/usr/sbin/abctl` — A/B boot slot controller
- `/usr/sbin/env_info.sh` — environment partition configuration
- `dmesg` kernel log — NAND chip identification and UBI attachment
