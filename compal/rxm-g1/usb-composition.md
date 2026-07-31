# RXM-G1 — USB composition and host driver binding

This is the practical guide to (1) picking a USB composition that gives you
the interfaces you actually want, and (2) getting the Linux `option` serial
driver to bind those interfaces reliably, every time, without ever touching
the ADB interface. If you only read one section, read
[Binding `option` without wedging ADB](#binding-option-without-wedging-adb).

## Background: this is a configfs/GSI gadget

The module's USB personality is built with Linux **configfs**
(`/sys/kernel/config/usb_gadget/g1`), using Qualcomm's GSI (Generic Software
Interface) gadget functions. This matters if you're used to older Qualcomm
platforms that expose `/sys/class/android_usb/android0/...` — that legacy
interface is present but unused on this firmware; any script that only
writes to it will silently do nothing.

On-device, `/sbin/usb/compositions/<PID>` scripts apply a named composition
by wiring up configfs functions and (re)binding the gadget to the UDC
(USB Device Controller). The **boot-time** composition is read from
`/etc/usb/boot_hsusb_comp` on first boot, when the gadget is still empty —
that's the easy case and works exactly as documented.

**Switching composition on a live, already-running gadget is a separate,
undocumented step:** the vendor composition scripts assume the gadget is torn
down, and don't unbind the UDC first. On a live gadget this makes the switch
a silent no-op. If you're changing composition from a running shell (rather
than rebooting), unbind the UDC yourself first:

```sh
echo "" > /sys/kernel/config/usb_gadget/g1/UDC   # unbind (drops USB briefly)
/sbin/usb/compositions/<PID> n 0 n               # now actually applies
```

Do this from a process that will survive the USB drop (a `systemd-run`
transient unit, not a backgrounded shell over the connection you're about to
lose) — unbinding the UDC drops any ADB/serial session using it.

## Compositions worth knowing

USB PID (last 4 hex digits reported by `lsusb`) selects the interface set.
Not every composition script in `/sbin/usb/compositions/` has a working
configfs code path on this platform — the ones below do:

| PID | Interfaces | Notes |
|-----|------------|-------|
| `901F` | DIAG + ADB + AT (DUN) | Common default. No NMEA, no USB data interface. |
| `90D5` | DIAG + ADB + MBIM + GNSS + AT (DUN) | Stock "data" composition. Whether the MBIM interface actually passes IP traffic depends on how the specific unit's hardware is provisioned — see the note on PCIe-endpoint personality below. |
| `90AD` | DIAG + ADB + AT (DUN) + NMEA + RmNet (QMI) + mass storage + DPL | The most useful composition for RF survey / wardriving work — the only one that natively exposes DIAG, AT, **and** NMEA as host serial ports simultaneously, plus a QMI control-plane interface. |

`90AD` interface layout, in order:

| iface | class/sub/proto | function | host result once bound |
|-------|------------------|----------|--------------------------|
| if0 | `ff/ff/30` | DIAG | serial port (DIAG) |
| if1 | `ff/42/01` | ADB | **claimed by the host ADB stack (usbfs/libusb) — never bind a serial driver here** |
| if2 | `ff/00/00` | AT (DUN) | serial port (AT) |
| if3 | `ff/00/00` | NMEA | serial port (NMEA) |
| if4 | `ff/ff/ff` | RmNet (QMI control) | `/dev/cdc-wdm*` + `wwan*` via the `qmi_wwan` driver |
| if5 | `08/06/50` | Mass storage | `/dev/sd*` |
| if6 | `ff/ff/ff` | DPL (data-path logging) | — |

A module can be strapped at the hardware level to route its data plane over
PCIe (MHI) instead of USB — some CPE-oriented board variants do this. On
those units, a USB data composition's control plane comes up fine, but no IP
traffic passes over it; you get DIAG/AT/ADB/NMEA and nothing else over USB.
This is a board-personality question, not something a composition switch
alone fixes.

## Binding `option` without wedging ADB

The Linux kernel's generic USB-serial `option` driver does not ship a
built-in match for this vendor/product ID, so nothing auto-binds these
serial interfaces — you have to register the ID yourself. The naive way to
do that is the kernel's dynamic-ID mechanism:

```sh
echo 05c6 90ad > /sys/bus/usb-serial/drivers/option1/new_id
```

**Don't do this blindly.** Registering a VID:PID via `new_id` makes `option`
a candidate driver for *every* interface on the device that looks like a
plausible match (vendor-class, `ff/xx/xx`), including the ADB interface. If
`option` wins that race — e.g. it gets registered before the host ADB
daemon has opened the ADB interface — ADB drops `offline`, and depending on
the platform the only recovery is a physical power cycle. This is the
single most common way to lose access to this module.

The reliable pattern:

1. **Confirm ADB already holds its interface** before doing anything else —
   check that the ADB interface's `driver` symlink under
   `/sys/bus/usb/devices/.../<if>/driver` points at `usbfs` (i.e., a
   userspace program, the ADB server, already has it open). If it doesn't,
   wait — don't race it.
2. **Bind interfaces individually and explicitly**, by sysfs path, instead of
   registering a blanket `new_id` that lets the driver core auto-probe every
   interface on the device:

   ```sh
   # find the DIAG and AT interfaces for this device (adjust indices to the composition in use)
   echo <bus>-<port>:1.0 > /sys/bus/usb/drivers/option/bind   # if0 = DIAG
   echo <bus>-<port>:1.2 > /sys/bus/usb/drivers/option/bind   # if2 = AT
   ```

   This claims exactly the interfaces you want and leaves everything else —
   most importantly the ADB interface — untouched.
3. **Address the resulting ports by stable `by-id` path, not `/dev/ttyUSB*`
   number.** `ttyUSB` numbering is global across every serial device on the
   host and is not deterministic across replugs. Map by interface instead:

   ```sh
   ls -d /sys/bus/usb/devices/<bus>-<port>:1.0/ttyUSB*   # DIAG
   ls -d /sys/bus/usb/devices/<bus>-<port>:1.2/ttyUSB*   # AT
   ```

   or use the `/dev/serial/by-id/...-ifNN-portN` symlinks udev already
   creates, which encode the interface number directly.

A udev rule is the practical way to make this automatic on every plug,
scoped to exactly the interfaces you want:

```
# /etc/udev/rules.d/99-compal-rxm-g1.rules
# Bind option only to the DIAG (if0) and AT (if2) interfaces; never touch if1 (ADB).
ACTION=="add", SUBSYSTEM=="usb", ATTR{idVendor}=="05c6", ATTR{idProduct}=="90ad", \
  ENV{ID_USB_INTERFACE_NUM}=="00", RUN+="/bin/sh -c 'echo $kernel > /sys/bus/usb/drivers/option/bind'"
ACTION=="add", SUBSYSTEM=="usb", ATTR{idVendor}=="05c6", ATTR{idProduct}=="90ad", \
  ENV{ID_USB_INTERFACE_NUM}=="02", RUN+="/bin/sh -c 'echo $kernel > /sys/bus/usb/drivers/option/bind'"
```

Once bound, treat the interface as fragile: don't unbind/rebind the driver
repeatedly on a live device. Unbind/rebind churn on these interfaces has been
observed to knock the gadget into a minimal recovery-only composition,
requiring a power cycle to clear.

## Quick sanity check

```sh
stty -F <AT-port> 115200 raw -echo
printf 'ATI\r' > <AT-port>
# expect: COMPAL / <model> / <firmware string>
```

If the module boots with the radio off (`AT+CFUN?` returns `0`), send
`AT+CFUN=1` — this also powers the GNSS front-end, which is shared with the
cellular RF chain.
