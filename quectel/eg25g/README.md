# Quectel EG25-G Module

## Firmware

As of the writing of this document, the latest firmware version for the Quectel EG25-G module is R07A08_A0.301.A0.301. The release notes document for this version has a date of 2025-04-24.

The official way to get upgraded firmware is to ask for it on the Quectel Technology Forum or from the company that sold you the module. But if you do enough searching online, many vendors that make things using Quectel modems will publish updated firmware.

## This directory contains documentation and resources related to the Quectel EG25-G module

* AT Commands: See the documentation in [at_commands.md](./at_commands.md)
* Links to publicly Available Documentation that I've found can be found in [publicly_available_docs.md](./publicly_available_docs.md)

## Commands that are useful for scanning and gathering information about the cellular network

### Spam commands

These commands are useful for running at a regular cadence to get info about the cellular network:

* AT+QENG="servingcell" - Get information about the cell that the modem is currently connected to
* AT+QENG="neighbourcell" - Get information about the cells that the modem can see in addition to the serving cell

### Slow commands

This command takes a minute or two to run, so useful for intel on the cellular networks nearby, but not something you can use to map out signal levels

* AT+QOPS - Get information about the cells that the modem can see, including signal strength and other details

## Outputs by Module and Firmware Version

To try to help answer questions about which modules and firmware versions have support for specific commands and options, I've started a collection of outputs from various modules and firmware versions. Here are the outputs I've collected so far:

* [Quectel EG25-G Module Firmware Version EG25GGBR07A08M2G_01.003.01.003](./outputs/EG25GGBR07A08M2G_01.003.01.003.md)
* [Quectel EG25-G Module Firmware Version EG25GGBR07A08M2G_30.203.30.203](./outputs/EG25GGBR07A08M2G_30.203.30.203.md)
* [Quectel EC25-G Module Firmware Version EG25GGBR07A08M2G_A0.301.A0.301](./outputs/EG25GGBR07A08M2G_A0.301.A0.301.md)

## Key features by firmware version

| Firmware Version | Key Feature | Note |
|-------------|------------------|------------------|
| R07A08_01.002.01.002 | AT+QOPS   | The main "scan all the frequencies" command for this family of modules |
| R07A08_01.002.01.002 | AT+QOPSCFG="scancontrol" | Configure which bands are scanned by the AT+QOPS command |
| R07A08_01.002.01.002 | AT+QOPSCFG="displayrssi" | Adds RSSI to the AT+QOPS output table |
| R07A08_30.004.30.004 | AT+QOPSCFG="displaybw" | Adds the width of the channel to the AT+QOPS output table |
| R07A08_30.204.30.204 | AT+QOPS Optimizations  | Release notes for this version include optimizations to AT+QOPS to "solve the band scanning issue"  |

For the purposes of doing scans trying to collect all info about cells, you should be fine running anything as far back as R07A08_01.002.01.002, but the bandwidth info added in R07A08_30.004.30.004 is insightful. If you're already upgrading firmware, you might as well go to at least R07A08_30.204.30.204 or the latest version.
