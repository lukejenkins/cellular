# USERSPACE ANALYSIS — Could we do ADB-over-MHI without a kernel driver?

Companion to `UPSTREAM-ANALYSIS.md`. That doc answers *"what would an
in-tree kernel port cost?"* — this one answers *"could we skip the
kernel entirely and do this in userspace?"*

Scope: the ADB-on-Linux use case specifically. Applies to the
RM520N-GL-AP platform but generalizes to any modem where MHI
is the transport and you control the host.

## Short answer

**Yes, via VFIO.** There's a well-established pattern (SPDK for
NVMe, DPDK for NICs) of moving the entire hardware driver into
userspace. MHI is a cleaner fit for that model than most devices,
because its protocol is well-specified, its state machine is small,
and its ring semantics map directly to the producer/consumer idioms
userspace drivers already use.

The blocker isn't protocol complexity; it's ecosystem cost. See the
tradeoffs section.

## Why VFIO, not UIO

- **UIO** (Userspace I/O) is the naive "map PCI BARs into userspace"
  facility. Works for polling but **doesn't safely give you
  DMA-capable memory** — a buggy userspace driver would scribble on
  arbitrary kernel memory. For MHI that's a dealbreaker: every
  channel uses DMA-coherent TRE rings and data buffers.
- **VFIO** uses the IOMMU to give userspace a *safe* DMA bounce
  buffer. mmap'd BARs, interrupt delivery via `eventfd`, and
  `VFIO_IOMMU_MAP_DMA` for userspace DMA memory — all with hardware
  isolation enforced by the IOMMU. A buggy userspace driver can't
  escape its DMA sandbox.

## Sketch of a userspace MHI + ADB stack

1. **Kernel prep (one-time).** Unbind `mhi-pci-generic` (or `mhi_q`)
   from `0000:05:00.0`, bind `vfio-pci` instead. Requires IOMMU
   enabled in BIOS (Intel VT-d / AMD-Vi). One sysfs write via
   `/sys/bus/pci/drivers_probe` plus a device-ID binding.

2. **Userspace MHI host library.** Reimplements what the kernel
   driver does, in userspace:
   - Map BAR 0 via `mmap` on the VFIO device fd.
   - Read MHI register layout from BAR (MHIREGLEN, CHDBOFF, ERDBOFF,
     BHI offset).
   - Allocate DMA-able memory via `VFIO_IOMMU_MAP_DMA` for the
     channel context arrays, CMD ring, event ring(s), and per-channel
     TRE rings.
   - Drive the state machine: write MHICTRL to transition
     `RESET → READY → M0 → MISSION MODE`; poll MHISTATUS.
   - MSI/MSI-X interrupts via VFIO → `eventfd` → `poll()`/`epoll()`.
   - Process the event ring: consume channel-state-change events,
     TRE completion events, EE change events.
   - Channel open: allocate channel TREs, send
     `MHI_RING_STATE_START` command on CMD ring, wait for CMD
     completion event.
   - Per-channel read/write: post TREs to the channel's ring, ring
     the doorbell (MMIO POST), wait on completion events.

3. **ADB channel driver** on top of (2): exposes a byte-stream API.
   Our existing TCP bridge becomes a thin wrapper, or disappears
   entirely if the same userspace process also speaks the adb
   server/client protocol over a Unix socket.

4. **Packaging.** Single binary. No kernel module. Survives kernel
   upgrades by construction.

## LOC + effort

| Layer | LOC estimate | Notes |
|---|---|---|
| VFIO boilerplate (device open, BAR mmap, IOMMU setup, IRQ eventfd) | ~200 | One-time, copy-pasteable from SPDK or DPDK reference |
| MHI host state machine + ring management | ~1500–3000 | Ported from `core/mhi_init.c` + `core/mhi_main.c`; GPL-2.0-only license permits direct adaptation |
| Channel/chardev abstraction for ADB | ~300–500 | Adapted from OOT `devices/mhi_uci.c` |
| Bridge/socket layer | ~100 | Similar to our current `mhi_adb_bridge.py` |
| **Total** | **~2500–4500** | Feasible by one motivated engineer in a couple of weeks |

Compare to the in-tree port (see `UPSTREAM-ANALYSIS.md`) at ~300–500
LOC. Userspace is roughly **10× more code** because you're reimplementing
the MHI core, which already exists in the kernel for anyone who
uses the in-tree path.

**Debugging is where userspace really bites you.** When an in-tree
driver misbehaves you get `dmesg`, `ftrace`, `bpftrace`, and a decade
of kernel-side tooling. When a userspace PCIe driver misbehaves,
there's no dmesg for the MMIO traffic — you instrument your own
register reads, dump your own ring contents, and correlate with
hardware events. The in-tree MHI driver has years of baked-in
recovery paths for edge cases (power-state transitions, link-down,
suspend/resume) that you'd rediscover the hard way.

## Why you might want this

- **Zero kernel dependency.** No OOT module, no DKMS, no
  kernel-upgrade rebuilds, no kernel-lockdown conflicts. Ships as
  userspace.
- **No hot-reload wedges.** The "cold-boot-only" hazard documented
  in `FINDINGS.md` is a driver-lifecycle issue. In userspace you
  just restart your process; the hardware doesn't care about the
  distinction.
- **Works in containers.** With `--device=/dev/vfio/...` passthrough,
  the userspace driver runs inside an unprivileged container. The
  IOMMU enforces isolation.
- **Parallel isolation.** Multiple independent consumers (ADB, a
  future fastboot tool, diagnostics) can coordinate in one
  userspace process with shared state — which is ironically
  *harder* to do cleanly in the kernel-driver model.

## Why you probably don't

- **VFIO is all-or-nothing per PCI function.** The moment
  `vfio-pci` binds `0000:05:00.0`, the kernel's `mhi-pci-generic`
  can't touch it. You lose `mhi_net`/rmnet (data path),
  `mhi_wwan_ctrl` (AT/QMI via wwan framework), and `qrtr_mhi` (QMI
  routing). If this modem is also your LTE/5G WAN, you need
  userspace reimplementations of those too, or you route traffic
  some other way — e.g. OEM QCMAP bridge on the modem's Ethernet
  side, which is broken on this firmware.
- **IOMMU required.** Fine for dev hosts, potentially a hassle on
  embedded / low-cost routers that ship IOMMU-disabled firmware.
- **Runtime PM / system suspend is harder.** The kernel driver
  plugs into `pm_runtime` and deals with bus-level D-state
  transitions. Userspace has to handle these via VFIO's
  suspend/resume hooks, which are less mature.
- **Single PCI device ownership.** You can't have "kernel drives
  data path, userspace drives ADB" — it's one or the other per PCI
  function.

## Middle-ground options

- **Mediated devices (`vfio-mdev`).** A kernel driver owns the
  hardware and carves out a "virtual MHI device" for userspace to
  claim just the ADB channel. Technically elegant, but writing the
  mdev driver is a substantial kernel patch of its own — more
  kernel work than the in-tree port in `UPSTREAM-ANALYSIS.md`, not
  less.
- **Modem-side ADB-over-network.** The modem's `adbd` is a normal
  Android-style adbd. In principle you could reconfigure it to
  listen on a TCP port bound to the modem's own IP (over whatever
  data composition is up — RNDIS, NCM, rmnet), bypassing the MHI
  channel entirely. That's a pivot of strategy, not a userspace
  driver, and depends on the modem firmware not blocking it (the
  broken QCMAP LAN bridge is why the MHI route was the escape hatch
  in the first place).
- **Keep kernel for everything, add one small in-tree `mhi_adb`
  driver.** This is the `UPSTREAM-ANALYSIS.md` recommendation.
  Compared to any userspace option it's the lowest LOC, the lowest
  ongoing maintenance, and the shortest path to a tool that
  "just works" on a stock kernel.

## Decision matrix

| Approach | LOC | Kernel work | Userspace work | Keeps wwan/netdev? | Best when |
|---|---|---|---|---|---|
| Keep OOT driver + bridge (current) | 0 kernel + ~100 us | none (vendored) | already done | yes (via OOT netdev) | today, to ship something that works |
| In-tree port (narrow `mhi_adb`) | ~400 kernel + ~100 us | small | bridge | yes | want sustainable upstream-able solution |
| Full userspace via VFIO | ~0 kernel + ~3000 us | none | large | **no** (without reimplementing them too) | modem is dedicated-ADB appliance |
| `vfio-mdev` carve-out | ~1000 kernel + ~500 us | substantial | moderate | yes | academic interest, very specialized fleet use |
| Modem-side ADB-over-TCP | depends | none | tiny | yes | firmware cooperates; not our case |

## Recommendation

For this project and this hardware, **the in-tree port remains the
best cost/benefit trade.** Userspace-via-VFIO is architecturally the
most elegant for a hypothetical "ADB-only modem appliance," but the
moment you also want the modem to be a WAN interface, the kernel
integration you'd sacrifice swamps the code-not-in-kernel win.

Keep this document as the record of *why* we didn't go userspace, so
future readers don't re-litigate it every time someone asks "why is
there a kernel module at all?"

## Philosophical note

The reason MHI lives in the kernel today is that *other consumers on
the same device* (wwan, netdev, qrtr) need to be in the kernel, so
the MHI core has to be there too to serve them. If ADB were the only
use, userspace would be a cleaner design.

This also sharpens what the real OOT-driver sin is: not "too much
code" but "wrong location." The 90% that duplicates in-tree code
doesn't need to be duplicated in *either* the kernel or userspace —
it just needs to exist once. The UCI chardev layer is the actual
novel work, and it's small enough that it could credibly live in any
of three places:

- **Kernel** (preferred, ~400 LOC) — what `UPSTREAM-ANALYSIS.md`
  proposes.
- **Userspace with the whole MHI stack above it** (viable, ~3000
  LOC) — what this doc describes.
- **Kernel-with-mdev-passthrough** (exotic, ~1000 LOC kernel + ~500
  LOC userspace) — middle ground, rarely worth it.

Engineering choice; no single right answer. Context picks the winner.
