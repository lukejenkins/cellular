# cellular

A place for my notes and ramblings about all things cellular.

## Modules and Devices

### Quectel

- [Quectel EC2x / EG2x (EG25-G)](/quectel/eg25g/) — LTE Cat 4 module (Qualcomm MDM9207). AT command docs, firmware captures, scanning commands.
- [Quectel BG95-M3](/quectel/bg95m3/) — LPWA module: NB-IoT / LTE Cat M1 (Qualcomm MDM9205). AT command docs and firmware captures.
- [Quectel RM502Q](/quectel/rm502q/) — 5G Sub-6 module (Qualcomm SDX55). AT command docs.

### Telit

- [Telit LM960](/telit/lm960/) — LTE Cat 18 module (Qualcomm SDX20). AT command docs and firmware captures.

### Casa Systems

- [Casa Systems CFW-3212](/casasystems/cfw3212/) — 5G FWA CPE (Qualcomm SDX62 / Quectel RG520N-NA OpenCPU). Root unlock tool and carrier remote management blocking guide.

### Orbic

- [Orbic RC400L](/orbic/rc400l/) — LTE Cat 4 MiFi hotspot (Qualcomm MDM9207). QMI LOC GNSS driver for stripped MDM9207 devices.

## Quests

- ~~try to figure out a way to get geolocation data on the Rayhunter: <https://github.com/EFForg/rayhunter/issues/20#issuecomment-2762008339>~~ — Done. See [Orbic RC400L GNSS Driver](/orbic/rc400l/gnss-driver/QMI-LOC-GNSS-Driver.md)

## Links

### Links to information

- <https://github.com/iamromulan/cellular-modem-wiki>

### Tools

- [QCSuper is a tool communicating with Qualcomm-based phones and modems, allowing to capture raw 2G/3G/4G (and for certain models 5G) radio frames, among other things.](https://github.com/P1sec/QCSuper)
- [SCAT: Signaling Collection and Analysis Tool](https://github.com/fgsect/scat)
- <https://github.com/mobile-insight>

### Blogs

- <https://markhoutz.com/>

### Privacy

* <https://github.com/EFForg/rayhunter>
** Keep an eye on this work, lors of extra logging which might be useful for PCI Scanning/Mapping. <https://github.com/EFForg/rayhunter/discussions/447>
* <https://github.com/MarlinDetection/Marlin>

## Unsorted

* <https://mjg59.dreamwidth.org/61725.html> blog post about the orbic RC400 and modifying usb mode
* <https://www.osmocom.org/projects/quectel-modems/wiki/EC25>
* <https://github.com/the-modem-distro>
