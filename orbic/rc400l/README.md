# Orbic RC400L

LTE Cat 4 MiFi hotspot based on Qualcomm MDM9207 (MDM9607 SoC).

| Field | Value |
|-------|-------|
| Chipset | Qualcomm MDM9207 (MDM9607 SoC) |
| Kernel | Linux 3.18.48 |
| Modem firmware | MPSS.JO.2.0.2.c1.7-00044-9607_GENNS_PACK-1 |
| USB VID:PID | 05c6:f622 |
| USB interfaces | RNDIS, DIAG, 3x Serial, ADB |

## GNSS Driver

The OEM removed all GNSS software from the application processor, but the DSP modem firmware retains a complete QMI LOC service implementation. This project reverse-engineered access to that service and built a working GNSS driver using a fake QMI IDL service object.

See [`gnss-driver/`](gnss-driver/) for source code, build instructions, and detailed protocol documentation.

The QMI LOC protocol work and driver code are valid for any MDM9207-based device with a functional GNSS antenna. The RC400L itself has an antenna limitation that prevents satellite acquisition, but the technique has been validated on a Quectel EG25-G (same SoC family).
