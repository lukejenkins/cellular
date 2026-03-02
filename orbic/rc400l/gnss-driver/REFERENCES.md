# GNSS Driver References

## Source Code Dependencies

### qmiserial2qmuxd (original)
- **Author:** Joey Hewitt (scintill)
- **License:** GPLv3
- **Repository:** https://github.com/nickt1/qmiserial2qmuxd
- **Description:** Proxy that bridges libqmi's serial QMI protocol to Qualcomm's qmuxd Unix domain socket protocol. Our `qmiserial2qmuxd.c` is a modified version with SOCKPATH changed to `/var/` for non-Android Qualcomm Linux.
- **Usage context:** This was the initial approach (socat+adb bridge). It works for establishing a qmuxd connection but qmuxd's RAW_QMI_CTL handler returns EIO (-5) when trying to forward control messages. Superseded by the QMI CCI library approach.

### Qualcomm QMI Framework (struct definitions)
- **Source:** https://github.com/nickt1/qmi-framework (hades2013 fork)
- **Key files:**
  - `inc/qmi_idl_lib_internal.h` — `struct qmi_idl_service_object` definition
  - `inc/qmi_idl_lib.h` — public QMI IDL API types
  - `qcci/src/qmi_cci_common.c` — `qmi_client_init_instance` implementation, shows which service object fields are validated
- **Usage:** Provided the exact struct layout for constructing our fake LOC service object

### Qualcomm Location Service IDL (LOC v02)
- **Source:** https://github.com/nickt1/vendor-qcom-opensource-location (sonyxperiadev)
- **Key files:**
  - `loc_api/loc_api_v02/location_service_v02.h` — message structs, TLV definitions, enum values
  - `loc_api/loc_api_v02/location_service_v02.c` — IDL-generated TLV encoding tables
- **Usage:** Provided correct TLV formats for QMI_LOC_REG_EVENTS, QMI_LOC_START, QMI_LOC_STOP

### libqmi (freedesktop.org)
- **Repository:** https://github.com/nickt1/libqmi (linux-mobile-broadband)
- **Key files:**
  - `data/qmi-service-loc.json` — machine-readable LOC service definition
  - `src/libqmi-glib/qmi-enums-loc.h` — LOC enum definitions
  - `src/libqmi-glib/qmi-flags64-loc.h` — event registration flag bits
- **API docs:** https://www.freedesktop.org/software/libqmi/libqmi-glib/1.20.0/
- **Usage:** Cross-reference for LOC message IDs, TLV types, and enum values

### GobiAPI (Qualcomm GobiNet)
- **Key file:** `GobiAPI/Core/Socket.cpp` — `sQMUXDHeader` struct definition
- **Usage:** Confirmed the 40-byte qmuxd socket header format

## Osmocom Wiki
- **QMI Architecture:** https://projects.osmocom.org/projects/quectel-modems/wiki/QMI
- **Description:** General QMI architecture documentation. No GPS-specific content but useful for understanding the QMI framework layers.

## Key QMI LOC Constants

### Service Object
| Field | Value | Notes |
|-------|-------|-------|
| service_id | 0x10 | QMI LOC service |
| idl_version (major) | 0x02 | LOC v02 |
| idl_minor_version | 0x23 | From Sony AOSP |
| library_version | 6 | QMI_IDL_LIB_ENCDEC_VERSION (must be 1-6) |

### Message IDs
| ID | Name | Direction |
|----|------|-----------|
| 0x0021 | QMI_LOC_REG_EVENTS | Request |
| 0x0022 | QMI_LOC_START | Request |
| 0x0023 | QMI_LOC_STOP | Request |
| 0x0024 | QMI_LOC_EVENT_POSITION_IND | Indication |
| 0x0025 | QMI_LOC_EVENT_GNSS_SV_IND | Indication |
| 0x0026 | QMI_LOC_EVENT_NMEA_IND | Indication |
| 0x002B | Engine State Change | Indication |
| 0x002C | Fix Session State Change | Indication |

### Event Registration Masks
| Bit | Mask | Event |
|-----|------|-------|
| 0 | 0x0000000000000001 | Position Report |
| 1 | 0x0000000000000002 | GNSS Satellite Info |
| 2 | 0x0000000000000004 | NMEA |
| 7 | 0x0000000000000080 | Engine State |
| 8 | 0x0000000000000100 | Fix Session State |

### LOC_START TLV Format
| TLV | Field | Type | Mandatory | Values |
|-----|-------|------|-----------|--------|
| 0x01 | session_id | uint8 | Yes | 1-255 |
| 0x10 | fix_recurrence | uint32 | No | 1=PERIODIC, 2=SINGLE |
| 0x11 | horizontalAccuracyLevel | uint32 | No | 1=LOW, 2=MED, 3=HIGH |
| 0x12 | intermediateReportState | uint32 | No | 1=ON, 2=OFF |
| 0x13 | minInterval | uint32 (ms) | No | default 1000 |

### Position Indication (0x0024) Key TLVs
| TLV | Field | Type | Notes |
|-----|-------|------|-------|
| 0x01 | sessionStatus | uint32 | 0=SUCCESS, 1=IN_PROGRESS, 2=DONE |
| 0x02 | sessionId | uint8 | matches LOC_START session_id |
| 0x10 | latitude | double (8 bytes) | degrees, WGS84 |
| 0x11 | longitude | double (8 bytes) | degrees, WGS84 |
| 0x12 | horizontalUncertaintyCircular | float | meters |
| 0x15 | altitudeWrtEllipsoid | float | meters |
| 0x1B | horizontalReliability | float | meters |
| 0x22 | DOP | float | dilution of precision |
