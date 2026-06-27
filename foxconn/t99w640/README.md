# Foxconn T99W640 Module

The Foxconn T99W640 is a PCIe (M.2) 5G modem built on the Qualcomm
Snapdragon X72 (SDX72) platform. It runs an on-module OpenWrt-based Linux
system and exposes its host interfaces over PCIe/MHI rather than USB.

## This directory contains documentation and resources related to the Foxconn T99W640 module

* Root ADB shell over MHI: See [adb_root_shell.md](./adb_root_shell.md)
* MHI ↔ TCP ADB bridge script: [mhi_adb_bridge.py](./mhi_adb_bridge.py)

## Notes

* The module is PCIe-only; USB is disabled in the default composition,
  so host access goes through the MHI character devices (`/dev/mhi_*`)
  or the in-tree `/dev/wwan0*` interfaces.
* A root ADB shell is available over the MHI `ADB` channel without any
  on-module unlock step — see the guide above for the host-side driver
  and bridge setup.
