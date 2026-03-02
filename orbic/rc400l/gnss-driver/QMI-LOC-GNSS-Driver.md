# Activating GNSS on a Stripped Qualcomm MDM9207 via QMI LOC

## Overview

This document describes how to activate the GNSS engine on a Qualcomm MDM9207-based cellular module where the OEM has completely stripped all GNSS/GPS software from the application processor. The technique uses a **fake QMI IDL service object** to register a client with the LOC service (0x10) on the modem DSP, bypassing the missing userspace GNSS stack entirely.

The work was validated on two devices sharing the same MDM9207 SoC:

- **Orbic RC400L** -- a budget LTE MiFi hotspot where all GNSS software has been removed
- **Quectel EG25-G** -- a cellular module with a functioning GNSS stack, used as a control device

The same cross-compiled binary was deployed to both devices. The EG25-G achieved a GPS fix within seconds (7 GPS + 8 GLONASS satellites, 4m accuracy); the RC400L saw zero navigation satellites, confirming a hardware antenna limitation rather than a software problem.

## Table of Contents

1. [Background and Problem Statement](#background-and-problem-statement)
2. [Architecture](#architecture)
3. [The Fake Service Object Technique](#the-fake-service-object-technique)
4. [Cross-Compilation for the MDM9207](#cross-compilation-for-the-mdm9207)
5. [Driver Code Walkthrough](#driver-code-walkthrough)
6. [Engine Configuration and Tuning](#engine-configuration-and-tuning)
7. [Assistance Data Injection](#assistance-data-injection)
8. [XTRA Predicted Orbit Injection](#xtra-predicted-orbit-injection)
9. [Test Results and Analysis](#test-results-and-analysis)
10. [SBAS/WAAS PRN Analysis](#sbaswaas-prn-analysis)
11. [Conclusions](#conclusions)
12. [Appendix: Complete Source Code](#appendix-complete-source-code)
13. [References](#references)

---

## Background and Problem Statement

The Orbic RC400L is a budget LTE MiFi hotspot built on Qualcomm's MDM9207 platform (MDM9607 SoC). It runs a stripped BusyBox-based Linux (kernel 3.18.48) with ADB enabled. Root access is provided by `/bin/rootshell`, installed as part of the [rayhunter](https://github.com/EFForg/rayhunter) IMSI catcher detection project by EFF.

Investigation of the device revealed:

- **Zero GPS/GNSS AT commands** out of 174 total supported AT commands
- **No location libraries** -- `libqmiservices.so.1` has NO `loc_get_service_object` symbol
- **No GPS config files** -- no `gps.conf`, `izat.conf`, `lowi.conf`, or `sap.conf`
- **No GPS daemons** -- no `loc_launcher`, `garden_app`, or any location HAL
- **No QMI USB endpoint** -- no `/dev/cdc-wdm*` on the host side

The MDM9607 SoC includes a GNSS engine, so we worked off an assumption that some GNSS functionality might still be present in the DSP firmware despite the OEM stripping it from the AP side. The question: can we talk to it directly?

The answer is yes, via Qualcomm's QMI CCI (Common Client Interface) library.

## Architecture

The data path from our test program to the GNSS engine:

```
+------------------------------------------------------------------+
|                           Host                                   |
|  +-----------+  +-----------+  +-----------------+               |
|  |  USB ADB  |  | ttyUSB0-3 |  | RNDIS (enx...)  |               |
|  +-----+-----+  +-----------+  +--------+--------+               |
|        |                                 |                       |
|        | adb push/shell                  | 192.168.1.x           |
+--------+---------------------------------+-----------------------+
         |                                 |
+--------+---------------------------------+-----------------------+
|        |          Orbic RC400L (MDM9207) |                       |
|  +-----v-----+                           |                       |
|  |   adbd    |                           |                       |
|  +-----+-----+                           |                       |
|        | /bin/rootshell                  |                       |
|  +-----v---------------------------------v---------------------+ |
|  |               /tmp/qmi_loc_test                             | |
|  |  +------------------------------------------------------+   | |
|  |  | fake_loc_svc_obj (service_id=0x10, v=6)              |   | |
|  |  +--------------------------+---------------------------+   | |
|  |                             | qmi_client_init_instance()    | |
|  |                             | qmi_client_send_raw_msg_sync()| |
|  +-----------------------------+-------------------------------+ |
|                                |                                 |
|  +-----------------------------v-------------------------------+ |
|  |             libqmi_cci.so.1 (QMI CCI)                       | |
|  |        libqmi_client_qmux.so.1 (QMUX transport)             | |
|  +-----------------------------+-------------------------------+ |
|                                | Unix socket                     |
|  +-----------------------------v-------------------------------+ |
|  |                        qmuxd                                | |
|  |             /var/qmux_connect_socket                        | |
|  +-----------------------------+-------------------------------+ |
|                                | QMI IPC Router                  |
|  +-----------------------------v-------------------------------+ |
|  |             MDM9207 DSP (Modem Firmware)                    | |
|  |                                                             | |
|  |   +-----------+  +-----------+  +--------------+            | |
|  |   |  QMI DMS  |  |  QMI NAS  |  |   QMI LOC    |            | |
|  |   |  (svc 02) |  |  (svc 03) |  |  (svc 0x10)  |            | |
|  |   +-----------+  +-----------+  +------+-------+            | |
|  |                                        |                    | |
|  |                                 +------v-------+            | |
|  |                                 | GNSS Engine  |            | |
|  |                                 | GPS L1 RF    |            | |
|  |                                 +------+-------+            | |
|  +----------------------------------------+--------------------+ |
|                                           |                      |
|                            +--------------v--------------+       |
|                            | Possible RF Path to Antenna |       |
|                            |         (1575 MHz)          |       |
|                            +-----------------------------+       |
+------------------------------------------------------------------+
```

The key insight is that `qmuxd` -- the QMI multiplexer daemon -- is running on the modem and routes QMI messages between AP-side clients and DSP services. It doesn't know or care whether a client is "official" software or our test program. The QMI CCI library handles the Unix socket protocol to `qmuxd`, and `qmuxd` routes messages to the appropriate DSP service based on the service ID in the QMI header.

### Why Not Use the Host-Side QMI Stack?

An earlier attempt used `qmiserial2qmuxd` to bridge between the host's `libqmi`/`qmicli` and the modem's `qmuxd`:

```
qmicli -> PTY -> socat -> TCP:4400 -> adb forward -> Unix socket -> bridge -> qmuxd
```

This failed because `qmuxd`'s `RAW_QMI_CTL` handler returned `syserr=-5` (EIO). The modem's `qmuxd` requires proper client allocation via `eQMUXD_MSG_ALLOC_QMI_CLIENT_ID`, whose message enum is not documented in any open-source code.

Running directly on the modem using QMI CCI bypasses all of this -- the CCI library handles `qmuxd` client allocation internally.

## The Fake Service Object Technique

### How QMI CCI Service Registration Works

Every QMI service (DMS, NAS, LOC, etc.) has an IDL-generated "service object" -- a struct that contains metadata about the service: its ID, version, message tables, and type tables. To create a client for a service, you call:

```c
qmi_client_init_instance(service_object, instance_id, callback, ...)
```

On a normal Qualcomm device, you'd get the LOC service object by calling `loc_get_service_object_v02()`, which returns a pointer to the IDL-generated `loc_qmi_idl_service_object_v01`. On the RC400L, this symbol doesn't exist.

But analysis of the QMI CCI source code (`qmi_cci_common.c`) revealed that `qmi_client_init_instance` only reads **three fields** from the service object:

1. `service_id` -- to tell `qmuxd` which DSP service to connect to
2. `idl_version` -- for version negotiation
3. `max_msg_len` -- for buffer allocation

And critically, `qmi_client_send_raw_msg_sync()` -- the API we use to send pre-encoded TLV buffers -- does **NOT** use the message tables or type tables at all. It sends raw bytes directly.

### The Service Object Structure

The `struct qmi_idl_service_object` layout (52 bytes on ARM32), from Qualcomm's `qmi_idl_lib_internal.h`:

```c
struct qmi_idl_service_object {
    uint32_t library_version;        // must be 1-6 (validated by switch)
    uint32_t idl_version;            // major version
    uint32_t service_id;             // 0x10 for LOC
    uint32_t max_msg_len;            // buffer sizing
    uint16_t n_msgs[3];             // req/resp/ind message counts
    const void *msgid_to_msg[3];    // message tables (unused by raw API)
    const void *p_type_table;       // type tables (unused by raw API)
    uint32_t idl_minor_version;     // minor version
    void *parent_service_obj;       // inheritance (NULL for us)
};
```

The `library_version` field is critical -- it must be between 1 and 6 inclusive. Every accessor function in the QMI IDL library validates this with a switch statement and returns `NULL` on mismatch. The correct value depends on the modem firmware's QMI framework version.

### Constructing the Fake Object

Rather than guessing the `library_version`, we read it at runtime from a service object that *does* exist on the modem -- the DMS service object:

```c
/* Reference the DMS service object data symbol directly */
extern struct qmi_idl_service_object dms_qmi_idl_service_object_v01;

/* Empty type table -- we don't use IDL encoding/decoding */
static struct qmi_idl_type_table_object loc_type_table = {
    .n_types = 0,
    .n_messages = 0,
    .n_referenced_tables = 0,
    .p_types = NULL,
    .p_messages = NULL,
    .p_referenced_tables = NULL,
    .p_ranges = NULL,
};

static struct qmi_idl_service_object fake_loc_svc_obj = {
    .library_version = 0,       /* filled at runtime from DMS */
    .idl_version = 0x02,        /* LOC_V02_IDL_MAJOR_VERS */
    .service_id = 0x10,         /* QMI_LOC_SERVICE_ID */
    .max_msg_len = 0x4000,      /* 16KB - generous buffer */
    .n_msgs = {0, 0, 0},
    .msgid_to_msg = {NULL, NULL, NULL},
    .p_type_table = &loc_type_table,
    .idl_minor_version = 0x23,  /* LOC v02.35, from Sony AOSP tree */
    .parent_service_obj = NULL,
};
```

At runtime, before initializing the LOC client:

```c
/* Copy library_version from DMS to match the modem's QMI framework */
fake_loc_svc_obj.library_version = dms_qmi_idl_service_object_v01.library_version;
```

On the RC400L, this value is `6`. On the EG25-G, it's also `6`. By reading it dynamically, the driver should work on any MDM9207 firmware variant without hardcoding.

### Why This Works

The DSP-side LOC service doesn't validate the AP client's service object. The QMI transport layer (CCI + qmuxd + IPC router) only cares about:

1. The service ID in the QMI header (0x10 for LOC)
2. The message ID and TLV payload

Since we use `qmi_client_send_raw_msg_sync()` to send hand-crafted TLV buffers, we bypass the entire IDL encoding/decoding layer. The fake service object is only needed to get past `qmi_client_init_instance()`'s validation checks.

## Cross-Compilation for the MDM9207

### The Problem

The MDM9207 runs an ARM32 Linux with glibc 2.22. A standard cross-compilation with `arm-linux-gnueabihf-gcc` on a modern host links against the host's glibc (version 2.34 during my testing),which introduces symbol versions that don't exist on the modem:

```
/lib/ld-linux.so.3: symbol __libc_start_main, version GLIBC_2.34 not found
```

Additionally, the host's dynamic linker path (`/lib/ld-linux-armhf.so.3`) differed from the modem's (`/lib/ld-linux.so.3`).

### The Solution

Pull the modem's own C library and link against it with `-nostdlib`:

```bash
# Step 1: Pull libraries from the modem
mkdir -p tmp/modem_libs
for lib in libqmi_cci.so.1 libqmi_client_qmux.so.1 libqmiservices.so.1 \
           libqmi_encdec.so.1 libqmiidl.so.1 libdsutils.so.1 libdiag.so.1 \
           libconfigdb.so.0 libtime_genoff.so.1 libxml.so.0; do
    adb pull /usr/lib/$lib tmp/modem_libs/
done
adb pull /lib/libc.so.6 tmp/modem_libs/
adb pull /lib/ld-linux.so.3 tmp/modem_libs/

# Create unversioned symlinks for the linker
cd tmp/modem_libs
for f in *.so.*; do
    base=$(echo "$f" | sed 's/\.so\..*//')
    ln -sf "$f" "${base}.so"
done
cd -
```

```bash
# Step 2: Compile
arm-linux-gnueabihf-gcc -o tmp/qmi_loc_test \
  modules/orbic/rc400l/gnss-driver/src/qmi_loc_test.c \
  -D_TIME_BITS=32 \
  -Ltmp/modem_libs \
  -lqmi_cci -lqmiservices -lqmi_client_qmux -lqmiidl -lqmi_encdec \
  -Wl,-rpath,/usr/lib \
  -Wl,--allow-shlib-undefined \
  -Wl,--dynamic-linker,/lib/ld-linux.so.3 \
  -nostdlib tmp/modem_libs/libc.so.6 -lgcc \
  $(arm-linux-gnueabihf-gcc -print-file-name=crtbeginS.o) \
  $(arm-linux-gnueabihf-gcc -print-file-name=crtendS.o) \
  $(arm-linux-gnueabihf-gcc -print-file-name=crti.o) \
  $(arm-linux-gnueabihf-gcc -print-file-name=crtn.o) \
  $(arm-linux-gnueabihf-gcc -print-file-name=Scrt1.o)
```

Each flag explained:

| Flag | Purpose |
|------|---------|
| `-D_TIME_BITS=32` | Force 32-bit `time_t`. The modem's glibc 2.22 doesn't have 64-bit time APIs. Without this, calls to `time()` link to `__time64` which doesn't exist. |
| `-Ltmp/modem_libs` | Search path for the modem's QMI shared libraries |
| `-lqmi_cci` etc. | Link against the QMI CCI framework, QMUX transport, services, IDL, and encoder/decoder |
| `-Wl,-rpath,/usr/lib` | Set runtime library search path to `/usr/lib` (where libs live on the modem) |
| `-Wl,--allow-shlib-undefined` | QMI libraries have transitive dependencies on glib, dsutils, diag, configdb, etc. These resolve at runtime on the modem; we don't need them at link time. |
| `-Wl,--dynamic-linker,/lib/ld-linux.so.3` | Set the ELF interpreter to the modem's linker, not the host's `/lib/ld-linux-armhf.so.3` |
| `-nostdlib tmp/modem_libs/libc.so.6` | Don't link the host's glibc; use the modem's glibc 2.22 directly |
| `-lgcc` | Still need the GCC runtime library for compiler builtins |
| `crtbeginS.o`, `crtendS.o`, `crti.o`, `crtn.o`, `Scrt1.o` | C runtime startup objects from the cross-toolchain. Required because `-nostdlib` removes the default CRT. These handle `_start`, `__libc_start_main`, global constructors/destructors, etc. |

### Deployment

```bash
# Push to modem and run as root
adb push tmp/qmi_loc_test /tmp/qmi_loc_test
adb shell chmod 755 /tmp/qmi_loc_test
adb shell '/bin/rootshell -c "/tmp/qmi_loc_test 2>&1"'
```

The same binary works on both the RC400L and EG25-G without modification -- both are MDM9207 with compatible QMI CCI libraries.

## Driver Code Walkthrough

The driver executes a specific sequence of QMI LOC operations. Each step is described below with the corresponding QMI message details.

### Step 1: QMI CCI Framework Validation (DMS Test)

Before touching the LOC service, we validate that the QMI CCI framework is functional by querying DMS (Device Management Service, service ID 0x02):

```c
/* Reference the exported DMS service object -- no fake needed */
extern struct qmi_idl_service_object dms_qmi_idl_service_object_v01;

static int test_dms(void) {
    qmi_client_type client = NULL;
    qmi_idl_service_object_type svc_obj = &dms_qmi_idl_service_object_v01;

    /* Connect to DMS with a 10-second timeout */
    qmi_client_error_type rc = qmi_client_init_instance(
        svc_obj, 0, NULL, NULL, NULL, 10000, &client);
    if (rc != QMI_NO_ERR) return -1;

    /* DMS_GET_DEVICE_MFR (0x0021) -- empty request, response has manufacturer string */
    uint8_t resp[512];
    unsigned int resp_len = 0;
    rc = qmi_client_send_raw_msg_sync(client, 0x0021,
        NULL, 0, resp, sizeof(resp), &resp_len, 5000);

    /* DMS_GET_DEVICE_MODEL (0x0022) */
    rc = qmi_client_send_raw_msg_sync(client, 0x0022,
        NULL, 0, resp, sizeof(resp), &resp_len, 5000);

    qmi_client_release(client);
    return 0;
}
```

Expected output:
```
DMS_GET_DEVICE_MFR:   "Reliance"  (RC400L)  /  "QUALCOMM" (EG25-G)
DMS_GET_DEVICE_MODEL: "RC400L"               /  "EG25"
```

If DMS fails, the QMI CCI framework isn't working at all (e.g., `qmuxd` isn't running, library version mismatch) and there's no point trying LOC.

### Step 2: LOC Client Initialization

```c
/* Copy library_version from DMS, then init the LOC client */
fake_loc_svc_obj.library_version = dms_qmi_idl_service_object_v01.library_version;

qmi_client_type client = NULL;
qmi_client_error_type rc = qmi_client_init_instance(
    &fake_loc_svc_obj,    /* our fake service object */
    0,                     /* default instance */
    loc_ind_cb,           /* indication callback function */
    NULL,                  /* callback user data */
    NULL,                  /* OS params (not needed) */
    10000,                /* 10-second timeout */
    &client               /* output: client handle */
);
```

If `qmi_client_init_instance` returns `QMI_TIMEOUT_ERR` (-7), the LOC service doesn't exist on the DSP. On both the RC400L and EG25-G, it succeeds immediately.

### Step 3: NMEA Type Configuration

```c
/*
 * QMI_LOC_SET_NMEA_TYPES (0x003E)
 *   TLV 0x01: nmea_type_mask (uint32, mandatory)
 *
 * Bitmask values:
 *   bit 0: GGA (fix data)          bit 4: VTG (track/speed)
 *   bit 1: RMC (recommended min)   bit 5: PQXFI (Qualcomm extended)
 *   bit 2: GSV (satellites in view) bit 6: PSTIS (Qualcomm timing)
 *   bit 3: GSA (DOP and active SVs)
 *
 * Enabling all types (0xFFFF) gives us GP (GPS), GA (Galileo),
 * GL (GLONASS), and GN (multi-constellation) prefixed sentences.
 */
uint32_t mask = 0x0000FFFF;  /* all types */
uint8_t req[8];
req[0] = 0x01;                    /* TLV type */
req[1] = 0x04; req[2] = 0x00;    /* TLV length = 4 */
memcpy(&req[3], &mask, 4);       /* TLV value */

qmi_client_send_raw_msg_sync(client, 0x003E,
    req, 7, resp, sizeof(resp), &resp_len, 5000);
```

### Step 4: Event Registration

```c
/*
 * QMI_LOC_REG_EVENTS (0x0021)
 *   TLV 0x01: eventRegMask (uint64, mandatory)
 *
 * We register for five event types:
 *   bit 0: Position reports (lat/lon/accuracy updates)
 *   bit 1: GNSS SV info (satellite visibility data)
 *   bit 2: NMEA sentences (standard NMEA 0183 output)
 *   bit 7: Engine state changes (ON/OFF)
 *   bit 8: Fix session state changes (STARTED/ENDED)
 */
uint64_t mask = (1ULL << 0) |   /* POSITION */
                (1ULL << 1) |   /* GNSS_SV_INFO */
                (1ULL << 2) |   /* NMEA */
                (1ULL << 7) |   /* ENGINE_STATE */
                (1ULL << 8);    /* FIX_SESSION */

uint8_t reg_req[11];
reg_req[0] = 0x01;                      /* TLV type */
reg_req[1] = 0x08; reg_req[2] = 0x00;   /* TLV length = 8 */
memcpy(&reg_req[3], &mask, 8);          /* TLV value */

qmi_client_send_raw_msg_sync(client, 0x0021,
    reg_req, 11, resp, sizeof(resp), &resp_len, 5000);
```

After registration, the engine starts delivering indications to our callback function whenever these events occur.

### Step 5: Start Positioning Session

```c
/*
 * QMI_LOC_START (0x0022)
 *   TLV 0x01: sessionId (uint8, mandatory) -- identifies this session
 *   TLV 0x10: fixRecurrence (uint32, optional)
 *     1 = PERIODIC (continuous tracking)
 *     2 = SINGLE (one fix then stop)
 *   TLV 0x11: horizontalAccuracyLevel (uint32, optional)
 *     1 = LOW (accept weaker signals, less precise fixes)
 *     2 = MED
 *     3 = HIGH (require strong signals, precise fix)
 *   TLV 0x13: minInterval (uint32 ms, optional)
 *     Minimum time between position reports
 *
 * IMPORTANT: fixRecurrence must be 1 or 2, NOT 0.
 * Value 0 causes QMI_ERR_MALFORMED_MSG (result=1, error=1).
 */
uint8_t start_req[48];
int pos = 0;

/* Session ID = 1 */
start_req[pos++] = 0x01;                      /* TLV type */
start_req[pos++] = 0x01; start_req[pos++] = 0x00;  /* TLV length = 1 */
start_req[pos++] = 0x01;                      /* session ID */

/* Fix recurrence = PERIODIC */
start_req[pos++] = 0x10;
start_req[pos++] = 0x04; start_req[pos++] = 0x00;
uint32_t recurrence = 1;
memcpy(&start_req[pos], &recurrence, 4); pos += 4;

/* Horizontal accuracy = LOW (most permissive) */
start_req[pos++] = 0x11;
start_req[pos++] = 0x04; start_req[pos++] = 0x00;
uint32_t accuracy = 1;
memcpy(&start_req[pos], &accuracy, 4); pos += 4;

/* Minimum interval = 1000ms */
start_req[pos++] = 0x13;
start_req[pos++] = 0x04; start_req[pos++] = 0x00;
uint32_t interval = 1000;
memcpy(&start_req[pos], &interval, 4); pos += 4;

qmi_client_send_raw_msg_sync(client, 0x0022,
    start_req, pos, resp, sizeof(resp), &resp_len, 5000);
```

### Step 6: Indication Processing

All indications arrive via the callback registered during `qmi_client_init_instance`. The callback receives raw TLV buffers that we parse manually:

```c
static void loc_ind_cb(
    qmi_client_type client,
    unsigned int msg_id,
    void *ind_buf,
    unsigned int ind_buf_len,
    void *ind_cb_data
) {
    /* NMEA sentences (0x0026):
     *   TLV 0x01: nmea string (variable length)
     *   Wire format: [type:1][len:2][string_data:len]
     */
    if (msg_id == 0x0026 && ind_buf_len > 4) {
        const uint8_t *p = ind_buf;
        if (p[0] == 0x01) {
            uint16_t slen;
            memcpy(&slen, &p[1], 2);
            if (slen > 0 && slen + 3 <= ind_buf_len)
                printf("NMEA: %.*s\n", slen, &p[3]);
        }
    }

    /* Position indications (0x0024):
     *   TLV 0x01: sessionStatus (uint32) -- 0=SUCCESS, 1=IN_PROGRESS
     *   TLV 0x10: latitude (double, WGS84 degrees)
     *   TLV 0x11: longitude (double, WGS84 degrees)
     *   TLV 0x12: horUncCircular (float, meters)
     */
    if (msg_id == 0x0024) {
        const uint8_t *p = ind_buf;
        size_t i = 0;
        uint32_t status = 0xFF;
        double lat = 0, lon = 0;
        float hacc = 0;

        /* Walk through TLVs */
        while (i + 3 <= ind_buf_len) {
            uint8_t type = p[i];
            uint16_t len;
            memcpy(&len, &p[i+1], 2);
            i += 3;
            if (i + len > ind_buf_len) break;

            if (type == 0x01 && len >= 4) memcpy(&status, &p[i], 4);
            if (type == 0x10 && len >= 8) memcpy(&lat, &p[i], 8);
            if (type == 0x11 && len >= 8) memcpy(&lon, &p[i], 8);
            if (type == 0x12 && len >= 4) memcpy(&hacc, &p[i], 4);
            i += len;
        }

        if (status == 0)
            printf("POSITION FIX (SUCCESS): hacc=%.0f m\n", (double)hacc);
        else if (status == 1)
            printf("POSITION UPDATE (IN_PROGRESS): hacc=%.0f m\n", (double)hacc);
    }

    /* GNSS SV Info (0x0025):
     *   Binary array of 28-byte satellite records.
     *   See "SBAS/WAAS PRN Analysis" section for format details.
     */
    if (msg_id == 0x0025) {
        printf("GNSS SV INFO received\n");
    }
}
```

### Step 7: Session Termination

```c
/*
 * QMI_LOC_STOP (0x0023)
 *   TLV 0x01: sessionId (uint8, mandatory) -- must match LOC_START
 */
uint8_t stop_req[4];
stop_req[0] = 0x01;
stop_req[1] = 0x01; stop_req[2] = 0x00;
stop_req[3] = 0x01;  /* session ID */

qmi_client_send_raw_msg_sync(client, 0x0023,
    stop_req, 4, resp, sizeof(resp), &resp_len, 5000);

qmi_client_release(client);
```

## Engine Configuration and Tuning

### Operation Mode

The GNSS engine supports several positioning modes:

| Mode | Value | Description |
|------|-------|-------------|
| DEFAULT | 1 | Engine chooses based on available assists |
| MSB (MS-Based) | 2 | Uses assistance server for ephemeris, computes fix locally |
| MSA (MS-Assisted) | 3 | Sends raw measurements to server, server computes fix |
| STANDALONE | 4 | Pure GPS -- no network assistance, self-contained |
| CELL_ID | 5 | Cell tower positioning only |

For a device with no network assistance infrastructure, STANDALONE is the correct mode:

```c
/* Query current mode */
/* QMI_LOC_GET_OPERATION_MODE (0x004B) -- no mandatory TLVs */
qmi_client_send_raw_msg_sync(client, 0x004B,
    NULL, 0, resp, sizeof(resp), &resp_len, 5000);

/* The response arrives as an async indication with TLV 0x10: mode (uint32) */
/* Parse the synchronous response for the result TLV first: */
/*   TLV 0x02: [result:u16][error:u16] -- 0/0 = success */

/* Set to STANDALONE if not already */
uint32_t mode = 4;  /* STANDALONE */
uint8_t req[7];
req[0] = 0x01;
req[1] = 0x04; req[2] = 0x00;
memcpy(&req[3], &mode, 4);

qmi_client_send_raw_msg_sync(client, 0x004A,
    req, 7, resp, sizeof(resp), &resp_len, 5000);
```

**Result on both devices:** Already in STANDALONE mode (4). Setting it again succeeds but has no effect.

### Engine Lock

The engine lock controls which types of positioning sessions are allowed:

| Lock | Value | Description |
|------|-------|-------------|
| NONE | 0 | All sessions allowed |
| MI | 1 | Mobile-initiated sessions blocked |
| MT | 2 | Mobile-terminated sessions blocked |
| ALL | 3 | All sessions blocked |

```c
/* QMI_LOC_GET_ENGINE_LOCK (0x003D) */
qmi_client_send_raw_msg_sync(client, 0x003D,
    NULL, 0, resp, sizeof(resp), &resp_len, 5000);

/* QMI_LOC_SET_ENGINE_LOCK (0x003C) -- unlock */
uint32_t lock = 0;  /* NONE */
uint8_t req[7];
req[0] = 0x01;
req[1] = 0x04; req[2] = 0x00;
memcpy(&req[3], &lock, 4);

qmi_client_send_raw_msg_sync(client, 0x003C,
    req, 7, resp, sizeof(resp), &resp_len, 5000);
```

**Result on both devices:** `GET_ENGINE_LOCK` returns error 0x5E (not supported). `SET_ENGINE_LOCK` returns error 0x01 (failure). This API is not implemented in the MDM9207 firmware.

## Assistance Data Injection

### UTC Time Injection

Without a time reference, the GNSS engine must perform a blind search across all possible satellite Doppler shifts and code phases -- a process that can take 12+ minutes. Injecting the current UTC time allows the engine to predict satellite positions and narrow the search:

```c
/*
 * QMI_LOC_INJECT_UTC_TIME (0x0038)
 *   TLV 0x01: timeUtc (uint64) -- milliseconds since Unix epoch
 *   TLV 0x02: timeUnc (uint32) -- uncertainty in milliseconds
 *
 * 5000ms uncertainty is conservative -- the modem's system clock
 * may be off by a few seconds from the network, but not more.
 */
time_t now = time(NULL);
uint64_t time_ms = (uint64_t)now * 1000;
uint32_t unc_ms = 5000;

uint8_t req[20];
int pos = 0;

/* TLV 0x01: time in ms */
req[pos++] = 0x01;
req[pos++] = 0x08; req[pos++] = 0x00;
memcpy(&req[pos], &time_ms, 8); pos += 8;

/* TLV 0x02: uncertainty in ms */
req[pos++] = 0x02;
req[pos++] = 0x04; req[pos++] = 0x00;
memcpy(&req[pos], &unc_ms, 4); pos += 4;

qmi_client_send_raw_msg_sync(client, 0x0038,
    req, pos, resp, sizeof(resp), &resp_len, 5000);
```

The synchronous response is a transport-level ACK. The engine's actual acceptance or rejection arrives as an asynchronous 0x0038 indication.

### Position Seed Injection

Injecting an approximate position tells the engine which hemisphere to search, which satellites should be above the horizon, and approximately where in the sky to look:

```c
/*
 * QMI_LOC_INJECT_POSITION (0x0039)
 *
 * Note: Unlike most QMI messages where mandatory TLVs start at 0x01,
 * LOC_INJECT_POSITION uses optional TLV numbers matching the position
 * indication (0x0024):
 *   TLV 0x10: latitude (double, WGS84 degrees)
 *   TLV 0x11: longitude (double, WGS84 degrees)
 *   TLV 0x12: horUncCircular (float, meters)
 *
 * A 10 km uncertainty radius covers a wide area while still
 * narrowing the satellite search space significantly.
 */
double lat = ...;      /* approximate latitude */
double lon = ...;      /* approximate longitude */
float hacc = 10000.0f; /* 10 km uncertainty */

uint8_t req[64];
int pos = 0;

req[pos++] = 0x10;
req[pos++] = 0x08; req[pos++] = 0x00;
memcpy(&req[pos], &lat, 8); pos += 8;

req[pos++] = 0x11;
req[pos++] = 0x08; req[pos++] = 0x00;
memcpy(&req[pos], &lon, 8); pos += 8;

req[pos++] = 0x12;
req[pos++] = 0x04; req[pos++] = 0x00;
memcpy(&req[pos], &hacc, 4); pos += 4;

qmi_client_send_raw_msg_sync(client, 0x0039,
    req, pos, resp, sizeof(resp), &resp_len, 5000);
```

## XTRA Predicted Orbit Injection

Qualcomm's XTRA service provides predicted satellite orbit data (ephemeris/almanac) that can reduce TTFF from minutes to seconds. The data is downloaded from Qualcomm's servers as a binary file and injected in chunks via QMI.

### Downloading XTRA Data

```bash
# Download XTRA2 data on the host (valid for ~72 hours)
wget http://xtrapath4.izatcloud.net/xtra2.bin -O /tmp/xtra2.bin
adb push /tmp/xtra2.bin /tmp/xtra2.bin
```

### Chunked Injection

The XTRA file must be split into chunks because the QMI message size is limited. A critical discovery during development: the MDM9207 firmware has a hard-coded limit of **218 parts** (`QMI_LOC_MAX_PREDICTED_ORBITS_PARTS_V02`). Part 219 and above return `QMI_ERR_MALFORMED_MSG`.

For the standard `xtra2.bin` file (60,787 bytes):
- `ceil(60787 / 218) = 279 bytes/chunk`
- Using 280 bytes/chunk gives exactly 218 parts with the last part at 27 bytes

```c
/*
 * QMI_LOC_INJECT_PREDICTED_ORBITS (0x0035)
 *   TLV 0x01: totalSize  (uint32) -- total file size in bytes
 *   TLV 0x02: totalParts (uint16) -- number of chunks
 *   TLV 0x03: partNum    (uint16) -- 1-based chunk index
 *   TLV 0x04: partData   (uint8[]) -- chunk payload
 *
 * TLV 0x04 wire encoding for variable-length byte arrays:
 *   [type:1][tlv_len:2][element_count:2][data:N]
 *
 * The 2-byte element_count prefix is critical -- without it, the firmware
 * misinterprets the first 2 bytes of XTRA data as the count, causing
 * QMI_ERR_ARG_TOO_LONG (0x0013).
 *
 * TLV 0x05 (formatType: 0=XTRA1, 1=XTRA2, 2=XTRA3) is intentionally
 * OMITTED. The MDM9207 firmware returns QMI_ERR_ENCODING (0x003A) for
 * any optional TLV not present in its compiled IDL schema, even though
 * the QMI specification mandates that unknown optional TLVs should be
 * silently ignored. Without TLV 0x05, the modem accepts XTRA2 data
 * natively -- it auto-detects the format from the file header.
 */
#define XTRA_CHUNK_SIZE 280

for (uint16_t part = 1; part <= total_parts; part++) {
    size_t n = fread(chunk, 1, XTRA_CHUNK_SIZE, f);
    int pos = 0;

    /* TLV 0x01: totalSize */
    req[pos++] = 0x01;
    req[pos++] = 0x04; req[pos++] = 0x00;
    memcpy(&req[pos], &total_size, 4); pos += 4;

    /* TLV 0x02: totalParts */
    req[pos++] = 0x02;
    req[pos++] = 0x02; req[pos++] = 0x00;
    memcpy(&req[pos], &total_parts, 2); pos += 2;

    /* TLV 0x03: partNum (1-indexed) */
    req[pos++] = 0x03;
    req[pos++] = 0x02; req[pos++] = 0x00;
    memcpy(&req[pos], &part, 2); pos += 2;

    /* TLV 0x04: partData with element_count prefix */
    req[pos++] = 0x04;
    uint16_t data_len = (uint16_t)n;
    uint16_t tlv_len = 2 + data_len;     /* count + data */
    memcpy(&req[pos], &tlv_len, 2); pos += 2;   /* TLV length */
    memcpy(&req[pos], &data_len, 2); pos += 2;  /* element count */
    memcpy(&req[pos], chunk, n); pos += n;       /* actual data */

    qmi_client_send_raw_msg_sync(client, 0x0035,
        req, pos, resp, sizeof(resp), &resp_len, 10000);

    usleep(50000); /* 50ms between chunks */
}
```

The engine returns a 0x0035 indication for each chunk with:
- `TLV 0x01: status (uint32)` -- 0 = SUCCESS
- `TLV 0x11: maxPartSize (uint32)` -- the engine's preferred chunk size

All 218 parts were accepted successfully on both devices.

## Test Results and Analysis

### Side-by-Side Comparison

Both devices were tested outdoors with clear sky view, running the exact same cross-compiled binary:

| Metric | RC400L | EG25-G |
|--------|--------|--------|
| **SoC** | MDM9207 | MDM9207 |
| **Binary** | qmi_loc_test (identical) | qmi_loc_test (identical) |
| **QMI LOC Version** | v02.35 | v02.35 |
| **Operation Mode** | STANDALONE (4) | STANDALONE (4) |
| **Engine Lock** | Not supported (0x5E) | Not supported (0x5E) |
| **Time Injection** | Accepted | Accepted |
| **Position Seed** | Accepted | Accepted |
| **XTRA Injection** | 218/218 accepted | 218/218 accepted |
| **GPS Satellites** | **0** | **7** (SNR 43-50 dBHz) |
| **GLONASS Satellites** | **0** | **8** (SNR 29-50 dBHz) |
| **Galileo Satellites** | 0 | 0 |
| **SBAS Satellites** | 1-8 (phantom) | 4 (3 real WAAS) |
| **Best Accuracy** | 13,027 m (seed drift) | **4 m** |
| **Time to Fix** | Never | < 5 seconds |

### EG25-G Satellite Visibility (NMEA Output)

The EG25-G produced rich NMEA output showing multi-constellation tracking:

```
$GPGSV,3,1,09,05,63,137,48,11,37,053,43,12,36,175,48,18,15,250,46,1*62
$GPGSV,3,2,09,21,52,074,44,25,56,233,50,29,52,310,49,26,01,324,,1*6B
$GLGSV,3,1,10,78,48,264,49,,,,41,77,21,196,35,65,16,137,44,1*4A
$GLGSV,3,2,10,88,54,313,50,87,32,052,39,72,31,081,40,79,22,324,,1*7F
```

GPS satellites (PRN 1-32) with SNR 43-50 dBHz -- strong signals indicating a healthy RF front-end and antenna. GLONASS satellites (reported in `$GLGSV` with NMEA PRN 65-96) at SNR 29-50 dBHz. Position converged to 4m accuracy within seconds.

### RC400L Satellite Visibility

The RC400L produced only `$GPGSV` sentences with no GPS satellites and a handful of SBAS codes:

```
$GPGSV,2,1,06,33,,,,38,,,,39,,,,40,,,*72
$GPGSV,2,2,06,46,,,,48,,,*70
```

All reported SVs had no elevation, no azimuth, and uniform SNR around 34 dBHz. Position reports echoed the injected seed with uncertainty growing from ~12 km upward -- the engine had no real measurements to improve upon.

### NMEA Sentence Types Observed

Both devices produced GPS (`$GP*`), Galileo (`$GA*`), and multi-constellation (`$GN*`) prefixed sentences. The EG25-G additionally produced GLONASS (`$GL*`) sentences. Both produced Qualcomm-proprietary `$PQXFI` and `$PSTIS` sentences.

| Sentence | RC400L | EG25-G | Description |
|----------|--------|--------|-------------|
| $GPGGA | Empty | Valid fix | GPS fix data |
| $GPRMC | Status "V" (void) | Status "A" (active) | Recommended minimum |
| $GPGSV | SBAS PRNs only | GPS + SBAS PRNs | Satellites in view |
| $GPGSA | Mode 1 (no fix) | Mode 3 (3D fix) | DOP and active SVs |
| $GLGSV | Not present | 8+ GLONASS SVs | GLONASS satellites |
| $GAGGA | Empty | Empty | Galileo fix data |
| $GNGSA | Mode 1 | Mode 3 | Multi-constellation DOP |
| $PQXFI | Present | Present | Qualcomm extended fix |

## SBAS/WAAS PRN Analysis

### NMEA PRN Numbering Convention

In NMEA 0183, PRN numbers in `$GPGSV` sentences encode the constellation:

| NMEA PRN Range | Constellation | Conversion to SV ID |
|----------------|--------------|---------------------|
| 1-32 | GPS | PRN = SV ID |
| 33-64 | SBAS | SV ID = PRN + 87 |
| 65-96 | GLONASS (in `$GLGSV`) | Slot = PRN - 64 |

SBAS satellites share the `$GP` talker ID with GPS. Any PRN above 32 in a `$GPGSV` sentence is SBAS, not GPS.

### QMI LOC SV INFO Binary Format

The raw SV INFO indication (0x0025) provides more detail than NMEA, including the constellation system field:

```
Each satellite record (28 bytes):
  validMask:    uint32  (bitmask of valid fields)
  system:       uint32  (1=GPS, 2=Galileo, 3=SBAS, 4=BeiDou, 5=GLONASS)
  svId:         uint16  (satellite vehicle ID within constellation)
  healthStatus: uint16  (0=healthy, 1=unhealthy)
  svStatus:     uint16  (0x0002 = TRACKING)
  navData:      uint16  (navigation data availability)
  elevation:    float32 (degrees above horizon)
  azimuth:      float32 (degrees from north)
  snr:          float32 (signal-to-noise ratio in dBHz)
```

Binary decoding confirmed that every satellite detected by the RC400L had `system=3` (SBAS). No satellites with `system=1` (GPS) or `system=5` (GLONASS) were ever observed.

### SBAS Satellite Identification

The RC400L reported tracking the following SBAS SV IDs:

| NMEA PRN | SBAS SV ID | System | Region | Visible from North America? |
|----------|-----------|--------|--------|---------------------------|
| 33 | 120 | EGNOS | Europe (decommissioned) | No |
| 38 | 125 | SDCM | Russia | No |
| 39 | 126 | EGNOS | Atlantic/Europe | No |
| 40 | 127 | GAGAN | India | No |
| 41 | 128 | GAGAN | India | No |
| 42 | 129 | MSAS | Japan | No |
| 44 | 131 | **WAAS** | Americas | **Yes** |
| 46 | 133 | **WAAS** | Americas | **Yes** |
| 48 | 135 | **WAAS** | Americas | **Yes** |
| 49 | 136 | EGNOS | Europe | No |
| 50 | 137 | MSAS | Japan | No |

Only 3 of 11 reported SBAS satellites (PRN 44, 46, 48 = WAAS) are physically visible from the test location. The other 8 correspond to SBAS systems on the opposite side of the planet (EGNOS over Europe, GAGAN over India, MSAS over Japan, SDCM over Russia).

### Phantom SBAS Detections

These phantom detections share characteristics:
- No elevation or azimuth data (both 0.0)
- Uniform SNR values (~30-34 dBHz)
- Marked as `UNHEALTHY` with `navData=0`
- No usable navigation data extracted

This suggests the GNSS engine performs a **blind code search** across all known SBAS PRN codes (120-158) and reports noise-floor correlator energy as "tracking." Because SBAS uses the same L1 frequency (1575.42 MHz) as GPS, and SBAS geostationary satellites transmit at higher EIRP than GPS MEO satellites, even a marginal antenna picks up some L1 energy.

The EG25-G also detected some of these phantom SBAS codes but could easily distinguish them from real satellites because it had strong GPS and GLONASS signals to compare against.

### Implications

The presence of SBAS phantom detections at ~34 dBHz confirms:
1. The RC400L's L1 RF path is somewhat functional -- some 1575.42 MHz energy reaches the correlator
2. The signal level is far too low for GPS satellite acquisition (typically requires >38-42 dBHz)
3. The antenna is likely a poorly-matched cellular antenna providing residual sensitivity near the GPS L1 band, not a dedicated GNSS antenna

## Conclusions

### Hardware Limitation Confirmed

The side-by-side comparison is conclusive:

1. **Same SoC (MDM9207):** Both devices use identical Qualcomm basebands with identical GNSS DSP capabilities
2. **Same binary:** The exact same cross-compiled `qmi_loc_test` was deployed to both
3. **Same QMI LOC behavior:** Both engines return the same error codes (0x5E for engine lock), accept the same assists, and produce the same NMEA sentence types
4. **Radically different satellite visibility:** EG25-G sees 15+ navigation satellites at strong SNR; RC400L sees zero

The only difference is the RF hardware. FCC internal documentation for the RC400L shows no dedicated GNSS L1 antenna. The MDM9207 is a BGA package on a multi-layer PCB, so there is no reasonable way to visually determine whether an RF path exists from the cellular antennas or RF front-ends to the SoC's GNSS input. The marginal SBAS detections at ~34 dBHz suggest some L1 energy may leak through the cellular RF path, but far below the level needed for GPS satellite acquisition.

### Software Interventions Summary

Every software-level intervention was attempted and had no effect on the RC400L's satellite acquisition:

| Intervention | Result |
|-------------|--------|
| XTRA2 orbit injection (218 parts) | Accepted, no effect |
| UTC time injection | Accepted, no effect |
| Position seed injection | Accepted, no effect |
| Set STANDALONE mode | Already in STANDALONE |
| Unlock engine | API not supported |
| Low accuracy threshold | No effect |
| All NMEA types enabled | No new satellites detected |

### The Driver Works

The QMI LOC driver code is validated. On the EG25-G -- a device with a functional GNSS antenna -- it achieves a GPS fix within seconds with 4m accuracy using GPS and GLONASS constellations. The driver is portable to any MDM9207-based device with a functional GNSS antenna and a running `qmuxd` daemon.

### Future Work

1. **RF path analysis:** Perform X-ray or destructive multi-layer PCB analysis to try to identify any way to inject an L1 GNSS signal into the MDM9207.

---

## Appendix: Complete Source Code

The complete source code for `qmi_loc_test.c` is shown below with inline commentary.

```c
/*
 * qmi_loc_test.c -- Minimal QMI LOC GNSS client for MDM9207
 *
 * Copyright (C) 2026 Luke Jenkins
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * ---
 *
 * Activates the GNSS engine on Qualcomm MDM9207-based cellular modules
 * where the OEM has stripped all GNSS/GPS software from the application
 * processor. Constructs a fake QMI IDL service object to register with
 * the LOC service on the modem DSP, then communicates via raw TLV messages.
 *
 * QMI IDL struct layouts are interoperability interface definitions derived
 * from Qualcomm's qmi_idl_lib_internal.h. QMI LOC message IDs and TLV
 * formats were cross-referenced from:
 *   - Qualcomm LOC v02 IDL (BSD, Copyright (c) 2011-2014 Code Aurora Forum)
 *   - libqmi (LGPL-2.1+, freedesktop.org)
 *
 * Build: see BUILD.md for the cross-compilation recipe.
 * Usage: /tmp/qmi_loc_test [/path/to/xtra2.bin]
 *
 * Links dynamically against the modem's own QMI libraries:
 *   libqmi_cci.so.1        -- QMI Common Client Interface
 *   libqmi_client_qmux.so.1 -- QMUX transport layer
 *   libqmiservices.so.1     -- QMI service objects (DMS, NAS, etc.)
 *   libqmiidl.so.1          -- QMI IDL encoding/decoding
 *   libqmi_encdec.so.1      -- QMI message encoder/decoder
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

/* ================================================================
 * QMI IDL Service Object Structures
 *
 * These structs match Qualcomm's qmi_idl_lib_internal.h exactly.
 * The QMI CCI library uses the service object to:
 *   1. Identify which DSP service to connect to (service_id)
 *   2. Negotiate version compatibility (idl_version, library_version)
 *   3. Allocate response buffers (max_msg_len)
 *
 * For our purposes, only these fields matter. The message tables
 * and type tables are used by qmi_client_send_msg_sync() (the
 * encoding API), but NOT by qmi_client_send_raw_msg_sync() (the
 * raw TLV API that we use).
 * ================================================================ */

struct qmi_idl_service_message_table_entry {
    uint16_t qmi_message_id;
    uint16_t message_table_message_id;
    uint16_t max_msg_len;
};

struct qmi_idl_type_table_entry {
    uint32_t c_struct_sz;
    const uint8_t *p_encoded_type_data;
};

struct qmi_idl_message_table_entry {
    uint32_t c_struct_sz;
    const uint8_t *p_encoded_tlv_data;
};

struct qmi_idl_range_table_entry {
    uint32_t range_offset;
    uint32_t range_low;
    uint32_t range_high;
};

struct qmi_idl_type_table_object {
    uint16_t n_types;
    uint16_t n_messages;
    uint8_t  n_referenced_tables;
    const struct qmi_idl_type_table_entry *p_types;
    const struct qmi_idl_message_table_entry *p_messages;
    const struct qmi_idl_type_table_object **p_referenced_tables;
    const struct qmi_idl_range_table_entry *p_ranges;
};

struct qmi_idl_service_object {
    uint32_t library_version;    /* Must be 1-6; validated by switch() */
    uint32_t idl_version;        /* Major version of the IDL spec */
    uint32_t service_id;         /* QMI service ID (0x10 for LOC) */
    uint32_t max_msg_len;        /* Max message length for buffer alloc */
    uint16_t n_msgs[3];          /* Count of req/resp/ind messages */
    const struct qmi_idl_service_message_table_entry *msgid_to_msg[3];
    const struct qmi_idl_type_table_object *p_type_table;
    uint32_t idl_minor_version;  /* Minor version of the IDL spec */
    struct qmi_idl_service_object *parent_service_obj;
};

typedef struct qmi_idl_service_object *qmi_idl_service_object_type;

/* ================================================================
 * QMI CCI API Types and Declarations
 *
 * These are resolved at runtime from libqmi_cci.so.1 on the modem.
 * ================================================================ */

typedef int32_t qmi_client_error_type;
typedef void *qmi_client_type;
typedef uint32_t qmi_service_instance;

#define QMI_NO_ERR 0
#define QMI_TIMEOUT_ERR -7

typedef struct {
    int ext_signal;
    int sig;
    int timer_sig;
} qmi_client_os_params;

typedef void (*qmi_client_ind_cb)(
    qmi_client_type client,
    unsigned int msg_id,
    void *ind_buf,
    unsigned int ind_buf_len,
    void *ind_cb_data
);

extern qmi_client_error_type qmi_client_init_instance(
    qmi_idl_service_object_type service_obj,
    qmi_service_instance instance_id,
    qmi_client_ind_cb ind_cb,
    void *ind_cb_data,
    qmi_client_os_params *os_params,
    uint32_t timeout_msecs,
    qmi_client_type *user_handle
);

extern qmi_client_error_type qmi_client_send_raw_msg_sync(
    qmi_client_type user_handle,
    unsigned int msg_id,
    void *req_buf,
    unsigned int req_buf_len,
    void *resp_buf,
    unsigned int resp_buf_len,
    unsigned int *resp_msg_len,
    unsigned int timeout_msecs
);

extern qmi_client_error_type qmi_client_release(
    qmi_client_type user_handle
);

/* ================================================================
 * QMI LOC Constants
 *
 * Service ID 0x10 is the Location service. Message IDs are from
 * libqmi's qmi-service-loc.json and Qualcomm's AOSP LOC IDL.
 * ================================================================ */

#define QMI_LOC_SERVICE_ID             0x10
#define QMI_LOC_V02_IDL_MAJOR_VERS    0x02
#define QMI_LOC_V02_IDL_MINOR_VERS    0x23   /* v02.35 from Sony AOSP */

/* LOC message IDs */
#define QMI_LOC_REG_EVENTS             0x0021
#define QMI_LOC_START                  0x0022
#define QMI_LOC_STOP                   0x0023
#define QMI_LOC_EVENT_POSITION_IND     0x0024
#define QMI_LOC_EVENT_GNSS_SV_IND     0x0025
#define QMI_LOC_EVENT_NMEA_IND         0x0026
#define QMI_LOC_GET_PREDICTED_ORBITS_DATA_SOURCE 0x0034
#define QMI_LOC_INJECT_PREDICTED_ORBITS 0x0035
#define QMI_LOC_INJECT_UTC_TIME        0x0038
#define QMI_LOC_INJECT_POSITION        0x0039
#define QMI_LOC_SET_ENGINE_LOCK        0x003C
#define QMI_LOC_GET_ENGINE_LOCK        0x003D
#define QMI_LOC_SET_NMEA_TYPES         0x003E
#define QMI_LOC_SET_OPERATION_MODE     0x004A
#define QMI_LOC_GET_OPERATION_MODE     0x004B

/* Engine lock types */
#define QMI_LOC_LOCK_NONE  0   /* No lock -- all sessions allowed */
#define QMI_LOC_LOCK_MI    1   /* Lock mobile-initiated sessions */
#define QMI_LOC_LOCK_MT    2   /* Lock mobile-terminated sessions */
#define QMI_LOC_LOCK_ALL   3   /* Lock all sessions */

/* Operation modes */
#define QMI_LOC_OPER_MODE_DEFAULT    1
#define QMI_LOC_OPER_MODE_MSB        2   /* MS-Based (A-GPS) */
#define QMI_LOC_OPER_MODE_MSA        3   /* MS-Assisted */
#define QMI_LOC_OPER_MODE_STANDALONE 4   /* Pure standalone GPS */
#define QMI_LOC_OPER_MODE_CELL_ID    5   /* Cell tower only */

/* NMEA type bitmask */
#define QMI_LOC_NMEA_TYPE_ALL   0x0000FFFFu

/* Event registration masks */
#define QMI_LOC_EVENT_MASK_POSITION     (1ULL << 0)
#define QMI_LOC_EVENT_MASK_GNSS_SV_INFO (1ULL << 1)
#define QMI_LOC_EVENT_MASK_NMEA         (1ULL << 2)
#define QMI_LOC_EVENT_MASK_ENGINE_STATE (1ULL << 7)
#define QMI_LOC_EVENT_MASK_FIX_SESSION  (1ULL << 8)

/* XTRA injection constants */
#define XTRA_MAX_PARTS          218    /* MDM9207 firmware IDL limit */
#define XTRA_CHUNK_SIZE_DEFAULT 280    /* ceil(60787/218) rounded up */
#define XTRA_MAX_CHUNK_SIZE     2048   /* Static buffer upper bound */

/* ================================================================
 * DMS Service Object (External Symbol)
 *
 * We reference the DMS service object data symbol directly from
 * libqmiservices.so.1 instead of calling the _internal getter
 * function. This avoids version parameter mismatch issues and
 * gives us access to the library_version field.
 * ================================================================ */
extern struct qmi_idl_service_object dms_qmi_idl_service_object_v01;

/* ================================================================
 * Fake LOC Service Object
 *
 * qmi_client_init_instance reads: library_version, service_id,
 *   idl_version, max_msg_len
 * qmi_client_send_raw_msg_sync does NOT use message tables.
 *
 * library_version is set to match DMS at runtime to guarantee
 * compatibility with the modem's QMI framework version.
 * ================================================================ */
static struct qmi_idl_type_table_object loc_type_table = {
    .n_types = 0,
    .n_messages = 0,
    .n_referenced_tables = 0,
    .p_types = NULL,
    .p_messages = NULL,
    .p_referenced_tables = NULL,
    .p_ranges = NULL,
};

static struct qmi_idl_service_object fake_loc_svc_obj = {
    .library_version = 0,
    .idl_version = QMI_LOC_V02_IDL_MAJOR_VERS,
    .service_id = QMI_LOC_SERVICE_ID,
    .max_msg_len = 0x4000,    /* 16 KB */
    .n_msgs = {0, 0, 0},
    .msgid_to_msg = {NULL, NULL, NULL},
    .p_type_table = &loc_type_table,
    .idl_minor_version = QMI_LOC_V02_IDL_MINOR_VERS,
    .parent_service_obj = NULL,
};

static volatile int g_running = 1;

static void sighandler(int sig) {
    (void)sig;
    g_running = 0;
}

/* ================================================================
 * Utility Functions
 * ================================================================ */

static void hexdump(const char *label, const void *data, size_t len) {
    const uint8_t *p = data;
    printf("%s (%zu bytes):", label, len);
    for (size_t i = 0; i < len && i < 256; i++) {
        if (i % 16 == 0) printf("\n ");
        printf(" %02x", p[i]);
    }
    if (len > 256) printf("\n  ...");
    printf("\n");
}

static void dump_service_object(const char *name,
                                struct qmi_idl_service_object *obj) {
    printf("%s service object at %p:\n", name, (void *)obj);
    printf("  library_version:  %u\n", obj->library_version);
    printf("  idl_version:      %u\n", obj->idl_version);
    printf("  service_id:       0x%02x\n", obj->service_id);
    printf("  max_msg_len:      %u\n", obj->max_msg_len);
    printf("  n_msgs:           req=%u resp=%u ind=%u\n",
        obj->n_msgs[0], obj->n_msgs[1], obj->n_msgs[2]);
    printf("  idl_minor_version:%u\n", obj->idl_minor_version);
}

/*
 * Parse QMI result TLV (type 0x02) from a synchronous response.
 * Every QMI response contains: [result:u16][error:u16]
 *   result 0 = SUCCESS, 1 = FAILURE
 * Returns 0 on success, -1 on failure.
 */
static int check_qmi_result(const uint8_t *resp, unsigned int resp_len,
                             const char *label, uint16_t *out_error)
{
    if (out_error) *out_error = 0;
    for (size_t i = 0; i + 6 <= resp_len; ) {
        uint8_t  type = resp[i];
        uint16_t len;
        memcpy(&len, &resp[i + 1], 2);
        i += 3;
        if (i + len > resp_len) break;
        if (type == 0x02 && len >= 4) {
            uint16_t result, error;
            memcpy(&result, &resp[i], 2);
            memcpy(&error,  &resp[i + 2], 2);
            if (out_error) *out_error = error;
            if (result != 0) {
                printf("  %s: QMI FAILURE result=%u error=0x%04x\n",
                       label, result, error);
                return -1;
            }
            return 0;
        }
        i += len;
    }
    return 0;
}

/* ================================================================
 * LOC Indication Callback
 *
 * Called by the CCI framework thread when the DSP sends an
 * asynchronous indication. We parse the raw TLV payloads here.
 * ================================================================ */
static void loc_ind_cb(
    qmi_client_type client,
    unsigned int msg_id,
    void *ind_buf,
    unsigned int ind_buf_len,
    void *ind_cb_data
) {
    (void)client;
    (void)ind_cb_data;
    printf("[LOC IND] msg_id=0x%04x len=%u\n", msg_id, ind_buf_len);
    hexdump("  payload", ind_buf, ind_buf_len);

    /* --- NMEA (0x0026) --- */
    if (msg_id == QMI_LOC_EVENT_NMEA_IND && ind_buf_len > 4) {
        const uint8_t *p = ind_buf;
        if (p[0] == 0x01 && ind_buf_len > 3) {
            uint16_t slen;
            memcpy(&slen, &p[1], 2);
            if (slen > 0 && slen + 3 <= ind_buf_len)
                printf("  NMEA: %.*s\n", slen, &p[3]);
        }
    }

    /* --- Position Indication (0x0024) --- */
    if (msg_id == QMI_LOC_EVENT_POSITION_IND) {
        const uint8_t *p  = ind_buf;
        size_t         sz = ind_buf_len;
        size_t         i  = 0;
        uint32_t sess_status = 0xFF;
        double   lat = 0.0, lon = 0.0;
        float    hacc = 0.0f;
        int      has_lat = 0, has_lon = 0, has_hacc = 0;

        while (i + 3 <= sz) {
            uint8_t  type = p[i];
            uint16_t len;
            memcpy(&len, &p[i+1], 2);
            i += 3;
            if (i + len > sz) break;
            if (type == 0x01 && len >= 4)
                memcpy(&sess_status, &p[i], 4);
            else if (type == 0x10 && len >= 8) {
                memcpy(&lat, &p[i], 8); has_lat = 1;
            } else if (type == 0x11 && len >= 8) {
                memcpy(&lon, &p[i], 8); has_lon = 1;
            } else if (type == 0x12 && len >= 4) {
                memcpy(&hacc, &p[i], 4); has_hacc = 1;
            }
            i += len;
        }

        if (sess_status == 0)
            printf("  ** POSITION FIX (SUCCESS) **\n");
        else if (sess_status == 1)
            printf("  ** POSITION UPDATE (IN_PROGRESS) **\n");
        else
            printf("  ** POSITION IND status=%u **\n", sess_status);

        if (has_lat && has_lon) {
            printf("    lat=%.6f  lon=%.6f", lat, lon);
            if (has_hacc)
                printf("  hacc=%.0f m", (double)hacc);
            printf("\n");
        }
    }

    /* --- Engine/session state changes --- */
    if (msg_id == 0x002B)
        printf("  ** ENGINE STATE CHANGE **\n");
    if (msg_id == 0x002C)
        printf("  ** FIX SESSION STATE CHANGE **\n");
    if (msg_id == QMI_LOC_EVENT_GNSS_SV_IND)
        printf("  ** GNSS SV INFO **\n");

    /* --- Assistance data acceptance indications --- */
    if (msg_id == QMI_LOC_INJECT_UTC_TIME)
        printf("  ** INJECT_UTC_TIME IND (engine acceptance) **\n");
    if (msg_id == QMI_LOC_INJECT_POSITION)
        printf("  ** INJECT_POSITION IND (engine acceptance) **\n");
    if (msg_id == QMI_LOC_SET_NMEA_TYPES)
        printf("  ** SET_NMEA_TYPES IND **\n");

    /* --- XTRA injection indication --- */
    if (msg_id == QMI_LOC_INJECT_PREDICTED_ORBITS) {
        printf("  ** INJECT_PREDICTED_ORBITS IND (XTRA) **\n");
        const uint8_t *p = ind_buf;
        size_t i = 0;
        while (i + 3 <= ind_buf_len) {
            uint8_t  tlv_type = p[i];
            uint16_t tlv_len;
            memcpy(&tlv_len, &p[i + 1], 2);
            i += 3;
            if (i + tlv_len > ind_buf_len) break;
            if (tlv_type == 0x01 && tlv_len >= 4) {
                uint32_t status;
                memcpy(&status, &p[i], 4);
                printf("  status=%u (%s)\n", status,
                       status == 0 ? "SUCCESS" : "FAILURE");
            } else if (tlv_type == 0x11 && tlv_len >= 4) {
                uint32_t max_part;
                memcpy(&max_part, &p[i], 4);
                printf("  maxPartSize=%u bytes\n", max_part);
            }
            i += tlv_len;
        }
    }
}

/* ================================================================
 * Assistance Injection Functions
 * ================================================================ */

static int inject_utc_time(qmi_client_type client)
{
    time_t now = time(NULL);
    if (now < 1577836800L) {
        printf("WARN: clock looks wrong (epoch=%ld), skipping\n", (long)now);
        return -1;
    }
    uint64_t time_ms = (uint64_t)now * 1000;
    uint32_t unc_ms  = 5000;

    uint8_t req[20];
    int pos = 0;
    req[pos++] = 0x01;
    req[pos++] = 0x08; req[pos++] = 0x00;
    memcpy(&req[pos], &time_ms, 8); pos += 8;
    req[pos++] = 0x02;
    req[pos++] = 0x04; req[pos++] = 0x00;
    memcpy(&req[pos], &unc_ms, 4); pos += 4;

    uint8_t resp[64];
    unsigned int resp_len = 0;
    qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
        client, QMI_LOC_INJECT_UTC_TIME,
        req, pos, resp, sizeof(resp), &resp_len, 5000);
    printf("LOC_INJECT_UTC_TIME: rc=%d time_ms=%llu unc_ms=%u\n",
           rc, (unsigned long long)time_ms, unc_ms);
    if (rc == 0 && resp_len > 0)
        hexdump("  response", resp, resp_len);
    return rc;
}

static int inject_position(qmi_client_type client,
                           double lat, double lon, float hacc_m)
{
    uint8_t req[64];
    int pos = 0;

    /* LOC_INJECT_POSITION uses TLV 0x10/0x11/0x12 (not 0x01/0x02/0x03) */
    req[pos++] = 0x10;
    req[pos++] = 0x08; req[pos++] = 0x00;
    memcpy(&req[pos], &lat, 8); pos += 8;

    req[pos++] = 0x11;
    req[pos++] = 0x08; req[pos++] = 0x00;
    memcpy(&req[pos], &lon, 8); pos += 8;

    req[pos++] = 0x12;
    req[pos++] = 0x04; req[pos++] = 0x00;
    memcpy(&req[pos], &hacc_m, 4); pos += 4;

    uint8_t resp[64];
    unsigned int resp_len = 0;
    qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
        client, QMI_LOC_INJECT_POSITION,
        req, pos, resp, sizeof(resp), &resp_len, 5000);
    printf("LOC_INJECT_POSITION: rc=%d lat=%.4f lon=%.4f hacc=%.0fm\n",
           rc, lat, lon, (double)hacc_m);
    if (rc == 0 && resp_len > 0)
        hexdump("  response", resp, resp_len);
    return rc;
}

/* ================================================================
 * Engine Configuration Functions
 * ================================================================ */

static void query_operation_mode(qmi_client_type client)
{
    uint8_t resp[64];
    unsigned int resp_len = 0;
    qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
        client, QMI_LOC_GET_OPERATION_MODE,
        NULL, 0, resp, sizeof(resp), &resp_len, 5000);
    printf("GET_OPERATION_MODE: rc=%d resp_len=%u\n", rc, resp_len);
    if (rc == 0 && resp_len > 0) {
        hexdump("  response", resp, resp_len);
        unsigned int off = 0;
        while (off + 3 < resp_len) {
            uint8_t  tlv_type = resp[off];
            uint16_t tlv_len;
            memcpy(&tlv_len, &resp[off+1], 2);
            if (tlv_type == 0x10 && tlv_len == 4 &&
                off + 3 + 4 <= resp_len) {
                uint32_t mode;
                memcpy(&mode, &resp[off+3], 4);
                const char *mstr = "UNKNOWN";
                switch (mode) {
                    case 1: mstr = "DEFAULT"; break;
                    case 2: mstr = "MSB"; break;
                    case 3: mstr = "MSA"; break;
                    case 4: mstr = "STANDALONE"; break;
                    case 5: mstr = "CELL_ID"; break;
                }
                printf("  ** Current operation mode: %u (%s) **\n",
                       mode, mstr);
            }
            off += 3 + tlv_len;
        }
    }
}

static int set_operation_mode(qmi_client_type client, uint32_t mode)
{
    uint8_t req[8];
    int pos = 0;
    req[pos++] = 0x01;
    req[pos++] = 0x04; req[pos++] = 0x00;
    memcpy(&req[pos], &mode, 4); pos += 4;

    uint8_t resp[64];
    unsigned int resp_len = 0;
    qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
        client, QMI_LOC_SET_OPERATION_MODE,
        req, pos, resp, sizeof(resp), &resp_len, 5000);
    const char *mstr = "UNKNOWN";
    switch (mode) {
        case 1: mstr = "DEFAULT"; break;
        case 2: mstr = "MSB"; break;
        case 3: mstr = "MSA"; break;
        case 4: mstr = "STANDALONE"; break;
        case 5: mstr = "CELL_ID"; break;
    }
    printf("SET_OPERATION_MODE(%s): rc=%d\n", mstr, rc);
    if (rc == 0 && resp_len > 0)
        hexdump("  response", resp, resp_len);
    return rc;
}

static void query_engine_lock(qmi_client_type client)
{
    uint8_t resp[64];
    unsigned int resp_len = 0;
    qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
        client, QMI_LOC_GET_ENGINE_LOCK,
        NULL, 0, resp, sizeof(resp), &resp_len, 5000);
    printf("GET_ENGINE_LOCK: rc=%d resp_len=%u\n", rc, resp_len);
    if (rc == 0 && resp_len > 0) {
        hexdump("  response", resp, resp_len);
        unsigned int off = 0;
        while (off + 3 < resp_len) {
            uint8_t  tlv_type = resp[off];
            uint16_t tlv_len;
            memcpy(&tlv_len, &resp[off+1], 2);
            if (tlv_type == 0x10 && tlv_len == 4 &&
                off + 3 + 4 <= resp_len) {
                uint32_t lock;
                memcpy(&lock, &resp[off+3], 4);
                const char *lstr = "UNKNOWN";
                switch (lock) {
                    case 0: lstr = "NONE (all allowed)"; break;
                    case 1: lstr = "MI (mobile-initiated locked)"; break;
                    case 2: lstr = "MT (mobile-terminated locked)"; break;
                    case 3: lstr = "ALL (fully locked)"; break;
                }
                printf("  ** Current engine lock: %u (%s) **\n",
                       lock, lstr);
            }
            off += 3 + tlv_len;
        }
    }
}

static int set_engine_lock(qmi_client_type client, uint32_t lock_type)
{
    uint8_t req[8];
    int pos = 0;
    req[pos++] = 0x01;
    req[pos++] = 0x04; req[pos++] = 0x00;
    memcpy(&req[pos], &lock_type, 4); pos += 4;

    uint8_t resp[64];
    unsigned int resp_len = 0;
    qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
        client, QMI_LOC_SET_ENGINE_LOCK,
        req, pos, resp, sizeof(resp), &resp_len, 5000);
    printf("SET_ENGINE_LOCK(%u): rc=%d\n", lock_type, rc);
    if (rc == 0 && resp_len > 0)
        hexdump("  response", resp, resp_len);
    return rc;
}

static int set_nmea_types(qmi_client_type client, uint32_t mask)
{
    uint8_t req[8];
    int pos = 0;
    req[pos++] = 0x01;
    req[pos++] = 0x04; req[pos++] = 0x00;
    memcpy(&req[pos], &mask, 4); pos += 4;

    uint8_t resp[64];
    unsigned int resp_len = 0;
    qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
        client, QMI_LOC_SET_NMEA_TYPES,
        req, pos, resp, sizeof(resp), &resp_len, 5000);
    printf("SET_NMEA_TYPES: rc=%d mask=0x%08x\n", rc, mask);
    if (rc == 0 && resp_len > 0)
        hexdump("  response", resp, resp_len);
    return rc;
}

/* ================================================================
 * XTRA Orbit Injection
 * ================================================================ */

static uint32_t query_xtra_max_part_size(qmi_client_type client)
{
    uint8_t resp[256];
    unsigned int resp_len = 0;
    qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
        client, QMI_LOC_GET_PREDICTED_ORBITS_DATA_SOURCE,
        NULL, 0, resp, sizeof(resp), &resp_len, 5000);

    printf("GET_PREDICTED_ORBITS_DATA_SOURCE: rc=%d resp_len=%u\n",
           rc, resp_len);
    if (rc != 0 || resp_len == 0) return 0;
    hexdump("  response", resp, resp_len);

    for (size_t i = 0; i + 3 <= resp_len; ) {
        uint8_t  type = resp[i];
        uint16_t len;
        memcpy(&len, &resp[i + 1], 2);
        i += 3;
        if (i + len > resp_len) break;
        if (type == 0x10 && len >= 8) {
            uint32_t max_file, max_part;
            memcpy(&max_file, &resp[i], 4);
            memcpy(&max_part, &resp[i + 4], 4);
            printf("  maxFileSizeInBytes=%u  maxPartSize=%u\n",
                   max_file, max_part);
            return max_part;
        }
        i += len;
    }
    return 0;
}

static int inject_xtra_data(qmi_client_type client, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("ERROR: Cannot open XTRA file: %s\n", path);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size <= 0 || file_size > 1024 * 1024) {
        printf("ERROR: Unexpected XTRA file size: %ld\n", file_size);
        fclose(f);
        return -1;
    }

    uint32_t chunk_size = query_xtra_max_part_size(client);
    if (chunk_size == 0 || chunk_size > XTRA_MAX_CHUNK_SIZE) {
        printf("Using default chunk size %d\n", XTRA_CHUNK_SIZE_DEFAULT);
        chunk_size = XTRA_CHUNK_SIZE_DEFAULT;
    }

    uint32_t total_size  = (uint32_t)file_size;
    uint16_t total_parts = (uint16_t)((file_size + chunk_size - 1)
                                       / chunk_size);

    printf("INJECT_XTRA: file=%s size=%u chunk=%u parts=%u\n",
           path, total_size, chunk_size, total_parts);

    uint8_t chunk[XTRA_MAX_CHUNK_SIZE];
    uint8_t req[XTRA_MAX_CHUNK_SIZE + 64];
    uint8_t resp[128];

    for (uint16_t part = 1; part <= total_parts; part++) {
        size_t n = fread(chunk, 1, chunk_size, f);
        if (n == 0) break;

        int pos = 0;

        /* TLV 0x01: totalSize */
        req[pos++] = 0x01;
        req[pos++] = 0x04; req[pos++] = 0x00;
        memcpy(&req[pos], &total_size, 4); pos += 4;

        /* TLV 0x02: totalParts */
        req[pos++] = 0x02;
        req[pos++] = 0x02; req[pos++] = 0x00;
        memcpy(&req[pos], &total_parts, 2); pos += 2;

        /* TLV 0x03: partNum (1-indexed) */
        req[pos++] = 0x03;
        req[pos++] = 0x02; req[pos++] = 0x00;
        memcpy(&req[pos], &part, 2); pos += 2;

        /* TLV 0x04: partData with element_count prefix */
        req[pos++] = 0x04;
        uint16_t data_len = (uint16_t)n;
        uint16_t tlv_len  = 2 + data_len;
        memcpy(&req[pos], &tlv_len, 2); pos += 2;
        memcpy(&req[pos], &data_len, 2); pos += 2;
        memcpy(&req[pos], chunk, n); pos += n;

        /* TLV 0x05 (formatType) intentionally omitted --
         * MDM9207 returns QMI_ERR_ENCODING for unknown TLVs */

        unsigned int resp_len = 0;
        qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
            client, QMI_LOC_INJECT_PREDICTED_ORBITS,
            req, pos, resp, sizeof(resp), &resp_len, 10000);

        uint16_t qmi_error = 0;
        int qrc = check_qmi_result(resp, resp_len, "XTRA_PART",
                                    &qmi_error);

        if (part == 1 || part == total_parts ||
            rc != 0 || qrc != 0) {
            printf("  XTRA part %u/%u (%zu bytes): rc=%d qrc=%d\n",
                   part, total_parts, n, rc, qrc);
        }
        if (rc != 0 || qrc != 0) {
            printf("ERROR: XTRA injection failed at part %u\n", part);
            fclose(f);
            return -1;
        }
        usleep(50000); /* 50ms between chunks */
    }

    fclose(f);
    printf("INJECT_XTRA: all %u parts sent\n", total_parts);
    sleep(2); /* let engine validate CRC */
    return 0;
}

/* ================================================================
 * DMS Test (Framework Validation)
 * ================================================================ */

static int test_dms(void) {
    qmi_client_type client = NULL;
    qmi_idl_service_object_type svc_obj;
    qmi_client_error_type rc;

    printf("=== Testing DMS service ===\n");
    svc_obj = &dms_qmi_idl_service_object_v01;
    dump_service_object("DMS", svc_obj);

    rc = qmi_client_init_instance(svc_obj, 0, NULL, NULL,
                                   NULL, 10000, &client);
    if (rc != QMI_NO_ERR) {
        printf("ERROR: DMS init failed: %d\n", rc);
        return -1;
    }
    printf("DMS client initialized: %p\n", client);

    uint8_t resp[512];
    unsigned int resp_len = 0;

    /* DMS_GET_DEVICE_MFR (0x0021) */
    rc = qmi_client_send_raw_msg_sync(client, 0x0021, NULL, 0,
        resp, sizeof(resp), &resp_len, 5000);
    printf("DMS_GET_DEVICE_MFR: rc=%d resp_len=%u\n", rc, resp_len);
    if (rc == QMI_NO_ERR)
        hexdump("  response", resp, resp_len > 0 ? resp_len : 64);

    /* DMS_GET_DEVICE_MODEL (0x0022) */
    resp_len = 0;
    rc = qmi_client_send_raw_msg_sync(client, 0x0022, NULL, 0,
        resp, sizeof(resp), &resp_len, 5000);
    printf("DMS_GET_DEVICE_MODEL: rc=%d resp_len=%u\n", rc, resp_len);
    if (rc == QMI_NO_ERR)
        hexdump("  response", resp, resp_len > 0 ? resp_len : 64);

    qmi_client_release(client);
    printf("DMS client released\n");
    return 0;
}

/* ================================================================
 * LOC Test (GNSS Session)
 * ================================================================ */

static int test_loc(const char *xtra_path) {
    qmi_client_type client = NULL;
    qmi_idl_service_object_type svc_obj;
    qmi_client_error_type rc;

    printf("\n=== Testing LOC service (fake service object) ===\n");

    /* Copy library_version from DMS */
    fake_loc_svc_obj.library_version =
        dms_qmi_idl_service_object_v01.library_version;
    svc_obj = &fake_loc_svc_obj;
    dump_service_object("LOC (fake)", svc_obj);

    rc = qmi_client_init_instance(svc_obj, 0, loc_ind_cb, NULL,
                                   NULL, 10000, &client);
    if (rc == QMI_TIMEOUT_ERR) {
        printf("LOC init TIMED OUT - service 0x10 may not exist\n");
        return -1;
    }
    if (rc != QMI_NO_ERR) {
        printf("ERROR: LOC init failed: %d\n", rc);
        return -1;
    }
    printf("LOC client initialized: %p\n", client);

    /* Configure NMEA types, engine mode, and inject assists */
    set_nmea_types(client, QMI_LOC_NMEA_TYPE_ALL);

    printf("\n--- Querying engine configuration ---\n");
    query_operation_mode(client);
    query_engine_lock(client);

    printf("\n--- Configuring engine ---\n");
    set_engine_lock(client, QMI_LOC_LOCK_NONE);
    set_operation_mode(client, QMI_LOC_OPER_MODE_STANDALONE);

    inject_utc_time(client);
    inject_position(client, YOUR_LAT, YOUR_LON, 10000.0f);

    if (xtra_path) {
        int xrc = inject_xtra_data(client, xtra_path);
        if (xrc != 0)
            printf("WARN: XTRA injection failed, continuing\n");
    }

    /* Register for events */
    uint8_t reg_req[16];
    memset(reg_req, 0, sizeof(reg_req));
    reg_req[0] = 0x01;
    reg_req[1] = 0x08; reg_req[2] = 0x00;
    uint64_t mask = QMI_LOC_EVENT_MASK_POSITION |
                    QMI_LOC_EVENT_MASK_GNSS_SV_INFO |
                    QMI_LOC_EVENT_MASK_NMEA |
                    QMI_LOC_EVENT_MASK_ENGINE_STATE |
                    QMI_LOC_EVENT_MASK_FIX_SESSION;
    memcpy(&reg_req[3], &mask, 8);

    uint8_t resp[512];
    unsigned int resp_len = 0;
    rc = qmi_client_send_raw_msg_sync(client, QMI_LOC_REG_EVENTS,
        reg_req, 11, resp, sizeof(resp), &resp_len, 5000);
    printf("LOC_REG_EVENTS: rc=%d\n", rc);

    /* Start positioning session */
    uint8_t start_req[48];
    int pos = 0;
    memset(start_req, 0, sizeof(start_req));

    start_req[pos++] = 0x01;  /* sessionId */
    start_req[pos++] = 0x01; start_req[pos++] = 0x00;
    start_req[pos++] = 0x01;

    start_req[pos++] = 0x10;  /* fixRecurrence = PERIODIC */
    start_req[pos++] = 0x04; start_req[pos++] = 0x00;
    uint32_t recurrence = 1;
    memcpy(&start_req[pos], &recurrence, 4); pos += 4;

    start_req[pos++] = 0x11;  /* horizontalAccuracyLevel = LOW */
    start_req[pos++] = 0x04; start_req[pos++] = 0x00;
    uint32_t accuracy = 1;
    memcpy(&start_req[pos], &accuracy, 4); pos += 4;

    start_req[pos++] = 0x13;  /* minInterval = 1000ms */
    start_req[pos++] = 0x04; start_req[pos++] = 0x00;
    uint32_t interval = 1000;
    memcpy(&start_req[pos], &interval, 4); pos += 4;

    resp_len = 0;
    rc = qmi_client_send_raw_msg_sync(client, QMI_LOC_START,
        start_req, pos, resp, sizeof(resp), &resp_len, 5000);
    printf("LOC_START: rc=%d\n", rc);

    /* Wait for indications (up to 300 seconds) */
    printf("\nWaiting for indications (Ctrl+C to stop)...\n\n");
    int seconds = 0;
    while (g_running && seconds < 300) {
        sleep(1);
        seconds++;
        if (seconds % 30 == 0)
            printf("[%ds] Waiting...\n", seconds);
    }

    /* Stop session */
    uint8_t stop_req[4] = {0x01, 0x01, 0x00, 0x01};
    resp_len = 0;
    rc = qmi_client_send_raw_msg_sync(client, QMI_LOC_STOP,
        stop_req, 4, resp, sizeof(resp), &resp_len, 5000);
    printf("LOC_STOP: rc=%d\n", rc);

    qmi_client_release(client);
    printf("LOC client released\n");
    return 0;
}

/* ================================================================
 * Entry Point
 * ================================================================ */

int main(int argc, char *argv[]) {
    const char *xtra_path = (argc > 1) ? argv[1] : NULL;

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    printf("QMI LOC Test - fake LOC service object on MDM9207\n");
    if (xtra_path)
        printf("XTRA file: %s\n", xtra_path);
    printf("\n");

    if (test_dms() != 0) {
        printf("DMS test failed - QMI CCI not working\n");
        return 1;
    }
    printf("\nDMS test passed\n");

    test_loc(xtra_path);
    return 0;
}
```

## Appendix: QMI LOC Message Reference

| Msg ID | Name | Direction | Purpose |
|--------|------|-----------|---------|
| 0x0021 | REG_EVENTS | Request | Register for async indications |
| 0x0022 | START | Request | Begin positioning session |
| 0x0023 | STOP | Request | End positioning session |
| 0x0024 | EVENT_POSITION_IND | Indication | Position update (lat/lon/accuracy) |
| 0x0025 | EVENT_GNSS_SV_IND | Indication | Satellite visibility data |
| 0x0026 | EVENT_NMEA_IND | Indication | NMEA sentence output |
| 0x002B | ENGINE_STATE_IND | Indication | Engine ON/OFF |
| 0x002C | FIX_SESSION_STATE_IND | Indication | Session STARTED/ENDED |
| 0x0034 | GET_PREDICTED_ORBITS_DATA_SOURCE | Request | Query XTRA chunk size |
| 0x0035 | INJECT_PREDICTED_ORBITS | Request | Inject XTRA orbit data |
| 0x0038 | INJECT_UTC_TIME | Request | Inject current UTC time |
| 0x0039 | INJECT_POSITION | Request | Inject coarse position seed |
| 0x003C | SET_ENGINE_LOCK | Request | Lock/unlock positioning sessions |
| 0x003D | GET_ENGINE_LOCK | Request | Query lock state |
| 0x003E | SET_NMEA_TYPES | Request | Configure NMEA sentence types |
| 0x004A | SET_OPERATION_MODE | Request | Set standalone/MSB/MSA mode |
| 0x004B | GET_OPERATION_MODE | Request | Query positioning mode |

## References

### Source Code

- Joey Hewitt (scintill), "qmiserial2qmuxd." GPLv3. https://github.com/nickt1/qmiserial2qmuxd — Proxy bridging libqmi's serial QMI protocol to Qualcomm's qmuxd Unix domain socket protocol. Our `qmiserial2qmuxd.c` is a modified version. Used in the initial failed bridge approach.

- hades2013, "qmi-framework." https://github.com/nickt1/qmi-framework — Qualcomm QMI framework source. Key files: `inc/qmi_idl_lib_internal.h` (`struct qmi_idl_service_object` definition), `qcci/src/qmi_cci_common.c` (`qmi_client_init_instance` implementation showing which service object fields are validated). Provided the struct layout for constructing the fake LOC service object.

- sonyxperiadev, "vendor-qcom-opensource-location." https://github.com/nickt1/vendor-qcom-opensource-location — Qualcomm Location Service IDL (LOC v02). Key files: `loc_api/loc_api_v02/location_service_v02.h` (message structs, TLV definitions, enum values), `loc_api/loc_api_v02/location_service_v02.c` (IDL-generated TLV encoding tables). Provided correct TLV formats for QMI_LOC messages.

- linux-mobile-broadband, "libqmi." https://github.com/nickt1/libqmi — freedesktop.org QMI library. Key files: `data/qmi-service-loc.json` (machine-readable LOC service definition), `src/libqmi-glib/qmi-enums-loc.h` (LOC enum definitions), `src/libqmi-glib/qmi-flags64-loc.h` (event registration flag bits). Used as cross-reference for LOC message IDs, TLV types, and enum values.

- Qualcomm, "GobiAPI" (GobiNet driver). Key file: `GobiAPI/Core/Socket.cpp` (`sQMUXDHeader` struct definition). Confirmed the 40-byte qmuxd socket header format.

- EFF, "rayhunter." https://github.com/EFForg/rayhunter — IMSI catcher detection tool for the Orbic RC400L. Provides `/bin/rootshell` for root access on the device.

- comma.ai, "qcomgpsd." MIT. https://github.com/commaai/openpilot/tree/master/system/qcomgpsd — Qualcomm GPS daemon for the openpilot autonomous driving platform. Uses the DIAG protocol (rather than QMI LOC) to extract raw satellite measurements and position reports from Qualcomm modems. Reference for an alternative approach to accessing GNSS on Qualcomm SoCs.

### Documentation

- freedesktop.org. *libqmi-glib 1.20.0 API Reference.* https://www.freedesktop.org/software/libqmi/libqmi-glib/1.20.0/

- Osmocom. "QMI." *Quectel Modems Wiki.* https://projects.osmocom.org/projects/quectel-modems/wiki/QMI — General QMI architecture documentation.

### Standards

- NMEA 0183. *Standard for Interfacing Marine Electronic Devices.* National Marine Electronics Association. — Defines PRN numbering conventions: 1-32 (GPS), 33-64 (SBAS, SV ID = PRN + 87), 65-96 (GLONASS slot = PRN - 64).

### Assistance Data

- Qualcomm XTRA predicted orbit service: `http://xtrapath4.izatcloud.net/xtra2.bin` — XTRA2 format satellite orbit predictions, valid ~72 hours. Auto-detected by the MDM9207 engine without a format type TLV.
