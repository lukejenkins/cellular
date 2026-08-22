# T99W640 / DW5934e — FCC unlock (state of the public art)

> **What this page is and isn't.** This is a survey of the **already-public**
> FCC-unlock situation for this module plus our own *observed* result. It
> contains **no reverse-engineering of how the lock is enforced** and no bypass
> derivation of our own — only pointers to third-party work that is already
> published, and the behavior we saw using the upstream tooling. Where a link
> points at someone else's work, that is theirs, not ours.

## The situation

Like most carrier/OEM 5G M.2 modules, the DW5934e ships **FCC-locked**: RF
transmit is held off until an **FCC authentication** step runs. On this module
that step is handled by the Foxconn **"FOXAP" QMI service (wire id `0xE4`)** —
see `firmware_notes.md` §5 for the protocol surface.

## The public mechanism

The FCC-auth call is **upstream** — you do not need any private tooling:

- **libqmi** exposes it directly:
  `qmicli -d <dev> --fox-ap-set-fcc-authentication`
  (added in freedesktop libqmi **merge request !417**).
- **ModemManager** ships an **FCC-unlock dispatcher** keyed to Foxconn USB VID
  **`105b`**, so on a ModemManager host the unlock can run automatically.

### Observed result (ours)
Using the upstream libqmi FOXAP path on the DW5934e, the FCC authentication
succeeds and RF is permitted. The effect is **session-scoped** — it reverts on
reset / power-cycle, so it is applied per boot rather than being a persistent
flash change.

## Third-party FCC-unlock work (external — not ours)

These are independent, already-public efforts. Linked for completeness and
analysis; we neither host nor extend them:

| Project | What it is |
|---|---|
| [`akhundov13/foxconn-fcc-unlock`](https://github.com/akhundov13/foxconn-fcc-unlock) | Community host-side Foxconn FCC-unlock tool / algorithm. |
| [SySS blog — *Foxconn FCC unlock*](https://blog.syss.com/posts/foxconn-fcc-unlock/) | Security-research write-up of the Foxconn FCC-unlock scheme. |
| [chromium modemmanager `dispatcher-fcc-unlock/105b`](https://chromium.googlesource.com/chromiumos/third_party/modemmanager-next/+/master/data/dispatcher-fcc-unlock/105b) | ChromeOS ModemManager's Foxconn (VID 105b) FCC-unlock dispatcher script. |

## What is deliberately not here

We have done deeper reverse-engineering of the module firmware's internal
lock-arming logic. That material is **intentionally withheld** from this public
carve — it is our own decompilation-derived analysis of a regulatory control,
and publishing it is a different decision from documenting the module's
behavior. This page is the boundary: public mechanism + observed behavior +
pointers to existing third-party work, and nothing past it.
