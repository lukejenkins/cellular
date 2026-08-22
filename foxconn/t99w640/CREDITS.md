# Credits & related work — T99W640 / DW5934e

This module has an active independent hacker community. Much of what makes the
DW5934e tractable was worked out in the open by other people; this directory
builds on that, and the entries below are theirs. Corrections and additions
welcome.

## Community reverse-engineering

- **OpenWrt forum — [DW5934e / Snapdragon X72 4G/5G modem](https://forum.openwrt.org/t/dw5934e-snapdragon-x72-4g-5g-modem/244169)**
  — the primary community thread for this module. In particular, forum user
  **[Angrtoy](https://forum.openwrt.org/u/angrtoy/summary)** posted a deep
  reverse-engineering write-up of the Foxconn firmware (the on-module OpenWrt
  personality, the `AT+FOXNV` / `AT+FOX_MFT` / `AT+DPR` command surface, and the
  customer-id personality mechanism). Several behaviors documented here were
  first surfaced there. Treat that thread as the community's running notebook for
  this module.

## Vendor & upstream

- **[foxconn-pc/fii_linux](https://github.com/foxconn-pc/fii_linux)** — Foxconn's
  own Linux integration repo (host-side reference for the module's Linux support).
- **libqmi — [merge request !417](https://gitlab.freedesktop.org/mobile-broadband/libqmi/-/merge_requests/417)**
  — upstreamed the Foxconn "FOXAP" QMI service, including
  `--fox-ap-set-fcc-authentication`. This is the supported path for the FCC-auth
  step (see `fcc_unlock.md`).
- **ModemManager** — ships an FCC-unlock dispatcher keyed to Foxconn USB VID
  `105b` (see `fcc_unlock.md` for the ChromeOS ModemManager dispatcher link).

## Tooling used / referenced

- **[bkerler/edl](https://github.com/bkerler/edl)** — Qualcomm EDL (Sahara/Firehose)
  client, for the recovery/EDL path.
- **[1alessandro1/atcli_rust](https://github.com/1alessandro1/atcli_rust)** — a Rust
  AT-command client usable against the module's AT ports.
- **[iamromulan/qfenix](https://github.com/iamromulan/qfenix)** — Qualcomm modem
  tooling referenced during this work.

## FCC-unlock (external, not ours)

Third-party FCC-unlock projects are surveyed with links in
[`fcc_unlock.md`](./fcc_unlock.md) — `akhundov13/foxconn-fcc-unlock`, the SySS
blog write-up, and the chromium ModemManager dispatcher. Those are independent,
already-public efforts; they are credited there, and this directory publishes no
FCC-lock bypass derivation of its own.
