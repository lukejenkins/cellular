# UPSTREAM ANALYSIS — OOT `pcie_mhi` vs in-tree `mhi` for the ADB case

Companion to `FINDINGS.md`. FINDINGS answers *"does this work?"* (yes).
This doc answers *"how much is the OOT driver actually doing, and how
hard would an in-tree implementation be?"*

## License posture

The OOT driver is **GPL-2.0-only** end to end:

- `core/mhi_init.c`, `core/mhi_main.c`, `devices/mhi_uci.c` carry
  `// SPDX-License-Identifier: GPL-2.0-only` at the top
- `MODULE_LICENSE("GPL v2")` in `core/mhi_init.c` and
  `devices/rmnet_nss.c`
- `controllers/mhi_qcom.c` uses the older pre-SPDX block: *"GNU
  General Public License version 2 and only version 2"* — same intent
- Copyright: *"The Linux Foundation"* (Qualcomm's standard form for
  kernel-destined contributions), dates 2018–2019
- No separate `LICENSE` / `COPYING` file; SPDX tags are authoritative

**Consequence:** no clean-room is required. The Linux kernel is also
GPL-2.0, so code from this OOT tree can be lifted, adapted, and
upstreamed as a normal kernel contribution — preserve the Linux
Foundation copyright line, add a `Signed-off-by`, send to the relevant
list. This doc therefore talks about an in-tree *port / adaptation*,
not a clean-room rewrite.

The `only` variant matters: the resulting in-tree code stays
GPL-2.0-only. That matches Linux's own policy (Linus is explicit
about preferring GPL-2 only), so it's a non-issue for upstreaming but
would block any attempt to later relicense under a permissive or
GPL-3 terms.

That Qualcomm chose *The Linux Foundation* as the copyright holder
rather than Qualcomm itself is a strong hint this code was authored
with eventual upstreaming in mind — consistent with the historical
context below. Quectel has been shipping it OOT, but the provenance is
upstream-ready.

Scope: the ADB-on-Linux use case specifically. Other OOT features
(rmnet, vendor PM quirks) are out of scope.

## What the OOT driver contributes vs. what's in-tree

| Component | In-tree equivalent | OOT adds real value? |
|---|---|---|
| MHI core state machine (READY→M0→AMSS→MISSION) | `drivers/bus/mhi/host/*` — same protocol, same ring semantics | **No** — mostly duplicate, just more verbose INFO logs + a couple of Qualcomm PM quirks |
| PCIe glue (vendor/device ID bind) | `mhi-pci-generic` with per-SKU `mhi_pci_dev_info` tables | **No** — the in-tree `mhi-pci-generic` already binds `17cb:0308` on our host (observed via `readlink /sys/bus/pci/devices/…/driver` after cold boot) |
| Channel declarations (`chan_cfg[]`) | Per-SKU channel arrays in `pci_generic.c` | **No** — trivially mirrored (5 LOC) |
| Netdev / QMI / AT consumers | `mhi_net`, `mhi_wwan_ctrl`, `qrtr_mhi` | **No** — in-tree covers these cleanly |
| **UCI chardev layer (`mhi_uci.c`)** | **Nothing** | **Yes** — this is the 90% that makes `/dev/mhi_ADB` possible |
| Controller init quirks (`mhi_controller_qcom.c`) | Generic path works | **No** — marginal Qualcomm-specific PM tweaks |

**The ADB win comes from one missing abstraction in-tree:** a way to
expose an arbitrary named MHI channel as a character device.

Upstream maintainers have deliberately avoided a generic
`mhi_chardev` (`mhi_uci` equivalent) because their architectural bet
is that MHI channels should flow into proper subsystems — wwan for
AT/QMI/MBIM, netdev for data, qrtr for QMI-routing. ADB doesn't
fit any of those — it's its own protocol with its own userspace
ecosystem (`adb`, `adbd`).

## In-tree port — effort estimate

### Engineering work

1. **Add the ADB channel** to the in-tree `mhi_pci_dev_info` channel
   table for the SDX62/SDX65 SKUs — ~5 LOC, direct analog to
   `0001-add-adb-channel-declaration.patch`.

2. **Write a small in-tree `mhi_adb` misc/char driver** — registers
   as an `mhi_device_driver` for channel name `"ADB"`, creates
   `/dev/mhi_ADB`, implements `read`/`write`/`poll` over
   `mhi_queue_transfer` / `mhi_poll_wait`. Because of the matching
   GPL-2.0-only license, most of this can be a direct adaptation of
   the OOT `mhi_uci.c` (~600 LOC) narrowed to a single channel, with
   callbacks ported to the in-tree `mhi_device_driver` API. Cleaner
   path: model after `drivers/net/wwan/mhi_wwan_ctrl.c` (~350 LOC)
   which already uses the in-tree API idiomatically, and fold in
   only the ADB-specific bits from `mhi_uci.c` (non-blocking semantics,
   0x4000 buffer sizing, open-triggers-prepare-channel behavior).
   **~250–400 LOC** of actual code.

3. **Kconfig / Makefile / MAINTAINERS** — ~20 LOC.

**Total:** ~300–450 LOC, roughly a weekend for someone fluent with
the in-tree MHI API. No state-machine reinvention, no PCIe
reinvention, no firmware-protocol reverse engineering — all the hard
parts are already solved upstream.

### Bonus: could drop the OOT driver entirely

If the clean-room driver lands, the OOT `pcie_mhi.ko` is no longer
needed on this host for the ADB use case:

- In-tree `mhi-pci-generic` would bind the device (already does).
- In-tree `mhi_net` + `qmi_wwan` already handle data/QMI (we saw
  them loaded after cold boot, before our `rmmod` swap).
- The new `mhi_adb` driver would create `/dev/mhi_ADB` as a chardev
  against channel 36/37.

This is a net simplification: no vendored source tree, no DKMS
shimming, no hot-modem-reload wedges caused by the OOT driver's
one-shot `MHI_RESET` behavior. Driver lifecycle becomes standard
kernel module lifecycle.

### Long pole: upstream acceptance, not engineering

The real risk is maintainer review, not LOC count.

**Option A — narrow driver: `drivers/char/mhi_adb.c`.** Scoped
narrowly as "ADB transport for Qualcomm modems exposing ADB over
MHI." Defensible because ADB is a well-known standard protocol that
legitimately doesn't belong in wwan/netdev/qrtr. Expected pushback:
*"why not fold this into wwan?"* — answer: wwan framework assumes
AT/QMI/MBIM semantics; ADB is its own thing with its own client
ecosystem. This is the faster path to merge.

**Option B — generic facility: `drivers/bus/mhi/host/uci.c`.**
Revives the `mhi_uci` debate. Technically cleaner (one driver
serves ADB, fastboot, future OEM diagnostic channels). Slower to
land because it re-opens the architectural discussion the
maintainers have previously closed. Would need an RFC thread and
probably a concrete multi-use-case motivation to win.

**Recommended path:** Option A first (get ADB-over-MHI landed as a
precedent), then later propose Option B if more channel-chardev
cases accumulate. Option A also doesn't foreclose Option B — the
narrow driver can be retired and folded into the generic facility
later if/when the latter lands.

## Historical context — why the OOT driver exists at all

Quectel's `pcie_mhi.ko` predates Qualcomm's upstream MHI merge
(which happened in ~2020, Linux 5.9 timeframe). The OOT driver has
been "maintained by copying fixes from upstream," which is why its
log-line format, state names, and ring semantics all look so much
like in-tree MHI — they share ancestry. Qualcomm later upstreamed a
cleaner core; the OOT driver lagged.

For modems shipped before ~Linux 5.9 was widely deployed, Quectel
had no choice but to vendor a driver. Today the pressure is lower,
but the OOT driver still provides one thing upstream doesn't
(chardev exposure via `mhi_uci`), which is why modem-root workflows
still depend on it.

## Evidence the in-tree stack is up to the job

Direct observation (kernel 6.19.12-arch1-1):

- `mhi-pci-generic` binds `0000:05:00.0` (`17cb:0308`) on cold boot
  without any vendor driver present.
- `lsmod` shows `mhi_net`, `mhi_wwan_ctrl`, `qrtr_mhi` all loaded
  and used, meaning the in-tree stack's channel infrastructure is
  active for data/AT/QMI — just not for ADB, because no in-tree
  consumer claims channel 36/37.
- When we unbound and swapped to the OOT driver, modem-side
  behavior (MISSION MODE reached, chardevs enumerated, adbd
  responding to CNXN) was identical to what we'd expect from any
  properly-driven MHI host — no Quectel-specific magic in the ADB
  path.

This means there's no hidden vendor protocol or proprietary ring
semantics for the ADB channel. The in-tree core could drive it
tomorrow; it just needs a consumer driver.

## If someone wanted to do this

Starting points, in order:

1. Read `drivers/bus/mhi/host/pci_generic.c` to see how per-SKU
   channel tables are structured and find the existing SDX-family
   entries to extend.
2. Read `drivers/net/wwan/mhi_wwan_ctrl.c` as the closest existing
   precedent for a single-channel-to-chardev driver. The structure
   is what a new `mhi_adb` driver would mirror.
3. Read the OOT `mhi_uci.c` for the ADB-specific details
   (non-blocking read semantics, buffer sizing 0x4000, how the
   channel open triggers `__mhi_prepare_channel`).
4. Run the probe (`mhi_adb_probe.py`) against the new driver
   as the acceptance test — same signal, same protocol.
5. Post an RFC to the linux-arm-msm / netdev lists with a narrow
   scope ("ADB transport over MHI"), citing the Qualcomm modem
   root-shell use case and the empty gap in the subsystem-mapping
   strategy.

## Summary

- OOT driver's unique contribution for ADB = **one missing in-tree
  abstraction**, roughly 300–500 LOC of real work to fill.
- Engineering is modest; upstream politics is the long pole.
- After landing, the OOT driver could be retired on modern kernels
  for this use case, yielding a net simplification.
