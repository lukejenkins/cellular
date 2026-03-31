# cellular

A place for my notes and ramblings about all things cellular.

## Cross-Vendor References

- [Cellular Modem Scan Commands Reference](cell-scan-commands.md) — Cross-vendor comparison of AT commands that return cell tower observations. Covers Fibocom, Quectel, Sierra Wireless, SIMCom, and Telit modems with data field matrices, WiGLE submission compatibility, and scan strategy recommendations.

## Modules and Devices

### Casa Systems

- [Casa Systems CFW-3212](/casasystems/cfw3212/) — 5G FWA CPE (Qualcomm SDX62 / Quectel RG520N-NA OpenCPU). Root unlock tool and carrier remote management blocking guide.
  - [Flash Layout](/casasystems/cfw3212/flash_layout.md)
  - [Block Carrier Remote Management](/casasystems/cfw3212/guide_block_carrier_remote_mgmt.md)
  - [Web UI Screenshots](/casasystems/cfw3212/screenshots/)

### Orbic

- [Orbic RC400L](/orbic/rc400l/) — LTE Cat 4 MiFi hotspot (Qualcomm MDM9207). QMI LOC GNSS driver for stripped MDM9207 devices.
  - [QMI LOC GNSS Driver](/orbic/rc400l/gnss-driver/QMI-LOC-GNSS-Driver.md)
  - [Build Instructions](/orbic/rc400l/gnss-driver/BUILD.md)
  - [References](/orbic/rc400l/gnss-driver/REFERENCES.md)

### Quectel

- [Quectel Overview](/quectel/)
- [Quectel BG95-M3](/quectel/bg95m3/) — LPWA module: NB-IoT / LTE Cat M1 (Qualcomm MDM9205). AT command docs and firmware captures.
  - [AT Commands](/quectel/bg95m3/at_commands.md)
  - [Publicly Available Docs](/quectel/bg95m3/publicly_available_docs.md)
- [Quectel EC2x / EG2x (EG25-G)](/quectel/eg25g/) — LTE Cat 4 module (Qualcomm MDM9207). AT command docs, firmware captures, scanning commands.
  - [AT Commands](/quectel/eg25g/at_commands.md)
  - [Publicly Available Docs](/quectel/eg25g/publicly_available_docs.md)
- [Quectel RM502Q](/quectel/rm502q/) — 5G Sub-6 module (Qualcomm SDX55). AT command docs.
  - [AT Commands](/quectel/rm502q/at_commands.md)
  - [Publicly Available Docs](/quectel/rm502q/publicly_available_docs.md)

### Telit

- [Telit Overview](/telit/)
- [Telit LM960](/telit/lm960/) — LTE Cat 18 module (Qualcomm SDX20). AT command docs and firmware captures.
  - [AT Commands](/telit/lm960/at_commands.md)
  - [Publicly Available Docs](/telit/lm960/publicly_available_docs.md)

## Quests

- ~~try to figure out a way to get geolocation data on the Rayhunter: <https://github.com/EFForg/rayhunter/issues/20#issuecomment-2762008339>~~ — Done. See [Orbic RC400L GNSS Driver](/orbic/rc400l/gnss-driver/QMI-LOC-GNSS-Driver.md)

## Links

### Information

- <https://github.com/iamromulan/cellular-modem-wiki>

### Tools

- [QCSuper — capture raw 2G/3G/4G/5G radio frames from Qualcomm-based phones and modems](https://github.com/P1sec/QCSuper)
- [SCAT: Signaling Collection and Analysis Tool](https://github.com/fgsect/scat)
- <https://github.com/mobile-insight>

### Blogs

- <https://markhoutz.com/>

### Privacy

- <https://github.com/EFForg/rayhunter>
  - Keep an eye on this work, lots of extra logging which might be useful for PCI Scanning/Mapping. <https://github.com/EFForg/rayhunter/discussions/447>
- <https://github.com/MarlinDetection/Marlin>

## Unsorted

- <https://mjg59.dreamwidth.org/61725.html> — blog post about the Orbic RC400 and modifying USB mode
- <https://www.osmocom.org/projects/quectel-modems/wiki/EC25>
- <https://github.com/the-modem-distro>
