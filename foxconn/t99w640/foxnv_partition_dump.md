# foxnv partition dump — Foxconn T99W640

## Summary

`foxnv` is the Foxconn-managed non-volatile partition on the T99W640. It
holds the module's RF calibration, per-band/per-tech power tables, and
device configuration flags. It is a raw NAND partition (`mtd26` in the
firmware observed), separate from the Qualcomm EFS2 / legacy-NV store.

This guide pulls a byte-exact, **read-only** image of it over the root
ADB shell. There is no EDL step, no firehose loader, and no modem-side
change — it is a raw block read of an MTD character device.

## Background

`foxnv` is not reachable through the usual NV interfaces. It is not an
EFS2 path and not a numbered Qualcomm NV item, so QMI/DIAG NV reads and
EFS2 filesystem walks never touch it. The only way to capture it is a raw
read of the underlying MTD device, which the root ADB shell makes
trivial: the on-module Linux exposes every flash partition as
`/dev/mtdN`, and `dd` over `adb exec-out` streams the bytes out
unmodified.

The image is **device-specific**: it contains per-unit RF calibration and
identifiers. Treat a dump as sensitive and redact before sharing.

## Requirements

- A working root ADB shell over MHI — see
  [adb_root_shell.md](./adb_root_shell.md). Everything below assumes the
  OOT `pcie_mhi.ko` driver is loaded, the bridge is running, and the
  client is connected:

  ```bash
  adb connect 127.0.0.1:6555
  adb -s 127.0.0.1:6555 shell    # uid=0(root)
  ```

- ~20 MB of free space on the host for the image.

## Procedure

### 1. Locate the partition

```bash
adb -s 127.0.0.1:6555 shell 'cat /proc/mtd'
```

Find the `foxnv` line:

```
dev:    size   erasesize  name
...
mtd26: 01200000 00040000 "foxnv"
```

The first hex column is the partition size (`0x01200000` = 18,874,368
bytes); the second is the erase-block size (`0x40000` = 256 KiB). The MTD
index (`26` here) is **not guaranteed stable** across firmware builds —
match the `"foxnv"` name from `/proc/mtd` rather than hardcoding `mtd26`.

### 2. Dump it

```bash
adb -s 127.0.0.1:6555 exec-out 'dd if=/dev/mtd26 bs=131072' > foxnv.bin
```

Two details matter:

- Use **`exec-out`, not `shell`**. `exec-out` streams stdout over a
  binary-clean channel. `adb shell` allocates a pty that rewrites
  `\n`→`\r\n`, which silently corrupts binary data and is invisible until
  the hash check below fails.
- Read **`/dev/mtd26`** (the raw MTD character device), not
  `/dev/mtdblock26`. `bs=131072` (128 KiB) is just the transfer chunk
  size; any value that divides the partition cleanly works.

Reading with `if=` is non-destructive. **Never** run `dd` with
`of=/dev/mtd26` — writing the raw partition can brick the modem or
destroy its calibration.

### 3. Verify the image

Hash the source on the modem and the copy on the host; they must match.

```bash
adb -s 127.0.0.1:6555 shell 'sha256sum /dev/mtd26'
sha256sum foxnv.bin
```

Confirm the size as well:

```bash
stat -c %s foxnv.bin     # expect 18874368 (0x1200000)
```

A size mismatch or short read usually means the bridge dropped frames at
open or teardown — re-run the dump and re-check.

## Notes

- **Read-only.** Nothing in this procedure writes to the modem.
- **Other partitions.** The same method captures any partition listed in
  `/proc/mtd` — substitute the matching index. To grab the full set,
  named by partition:

  ```bash
  adb -s 127.0.0.1:6555 shell 'cat /proc/mtd' | tr -d '\r' \
    | sed -n 's/^mtd\([0-9]*\): [0-9a-f]* [0-9a-f]* "\(.*\)"/\1 \2/p' \
    | while read n name; do
        adb -s 127.0.0.1:6555 exec-out "dd if=/dev/mtd$n bs=131072" \
          > "mtd${n}_${name}.bin"
      done
  ```

  Verify each one with the per-partition `sha256sum` check from step 3.
