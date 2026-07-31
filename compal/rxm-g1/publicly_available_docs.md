# Compal RXM-G1 — Publicly Available Documentation

## FCC filings

The module and its resellers' rebrands each have their own FCC grant. FCC
filings include internal photos, block diagrams, and test reports that are
often the best public source of hardware detail for a module like this.

* Compal RXM-G1 (original module grant) — FCC ID `GKRRXMG1`:
  <https://fccid.io/GKRRXMG1>
* Tri Cascade SG500M2-X (M.2 module rebrand) — FCC ID `2ACARSG500M2`:
  <https://fccid.io/2ACARSG500M2> — also mirrored at
  <https://device.report/compal/rxm-g1>
* Tri Cascade VOS 5G Dongle (finished USB-C dongle) — FCC ID `2ACARVOS5GC`:
  <https://fccid.io/2ACARVOS5GC>

## Community documentation

* [leandroadonis86/5G-USB-Dongle-SDX55](https://github.com/leandroadonis86/5G-USB-Dongle-SDX55)
  — a community write-up covering a fully OEM-configured Tri Cascade VOS
  dongle built on this same module: shell output captures, the web GUI, and
  a FOTA/NAND-extraction procedure. Useful as an independent cross-check —
  it documents the same NAND partition table observed here.

## General references

* [3GPP TS 27.007](https://www.3gpp.org/) — AT command set for User
  Equipment. This module's AT dialect sticks close to the 3GPP baseline
  described here rather than adding vendor extensions.
