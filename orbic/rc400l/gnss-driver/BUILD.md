# Building and Deploying qmi_loc_test

## Prerequisites

- ARM cross-compiler: `arm-linux-gnueabihf-gcc` (any host: RPi5 aarch64, x86_64 Ubuntu, etc.)
- Modem libraries pulled from the RC400L (see below)
- ADB access to the RC400L modem

## Step 1: Pull Libraries from Modem

```bash
mkdir -p /tmp/modem_libs
for lib in libqmi_cci.so.1 libqmi_client_qmux.so.1 libqmiservices.so.1 \
           libqmi_encdec.so.1 libqmiidl.so.1 libdsutils.so.1 libdiag.so.1 \
           libconfigdb.so.0 libtime_genoff.so.1 libxml.so.0; do
    adb pull /usr/lib/$lib /tmp/modem_libs/
done
adb pull /lib/libc.so.6 /tmp/modem_libs/
adb pull /lib/ld-linux.so.3 /tmp/modem_libs/

# Create unversioned symlinks for the linker
cd /tmp/modem_libs
for f in *.so.*; do
    base=$(echo "$f" | sed 's/\.so\..*//')
    ln -sf "$f" "${base}.so"
done
```

## Step 2: Compile

```bash
arm-linux-gnueabihf-gcc -o /tmp/qmi_loc_test src/qmi_loc_test.c \
  -D_TIME_BITS=32 \
  -L/tmp/modem_libs \
  -lqmi_cci -lqmiservices -lqmi_client_qmux -lqmiidl -lqmi_encdec \
  -Wl,-rpath,/usr/lib \
  -Wl,--allow-shlib-undefined \
  -Wl,--dynamic-linker,/lib/ld-linux.so.3 \
  -nostdlib /tmp/modem_libs/libc.so.6 -lgcc \
  $(arm-linux-gnueabihf-gcc -print-file-name=crtbeginS.o) \
  $(arm-linux-gnueabihf-gcc -print-file-name=crtendS.o) \
  $(arm-linux-gnueabihf-gcc -print-file-name=crti.o) \
  $(arm-linux-gnueabihf-gcc -print-file-name=crtn.o) \
  $(arm-linux-gnueabihf-gcc -print-file-name=Scrt1.o)
```

### Why this is complex

- `-nostdlib ... libc.so.6`: Links against the modem's glibc 2.22 instead of the host's newer glibc. The host cross-compiler's glibc (2.34+) introduces `__libc_start_main@GLIBC_2.34` which doesn't exist on the modem.
- `-Wl,--allow-shlib-undefined`: QMI libraries have transitive dependencies (glib, dsutils, etc.) that resolve at runtime on the modem.
- `-Wl,--dynamic-linker,/lib/ld-linux.so.3`: The modem uses `/lib/ld-linux.so.3`, not the host's `/lib/ld-linux-armhf.so.3`.
- CRT objects (`crtbeginS.o`, etc.): Still needed from the cross-toolchain since we used `-nostdlib`.

## Step 3: Deploy and Run

```bash
adb push /tmp/qmi_loc_test /tmp/qmi_loc_test
adb shell chmod 755 /tmp/qmi_loc_test
adb shell '/bin/rootshell -c "/tmp/qmi_loc_test 2>&1"'
```

### Remote deployment (via SSH to a host with ADB)

```bash
scp /tmp/qmi_loc_test root@<host>:/tmp/qmi_loc_test
ssh root@<host> "adb push /tmp/qmi_loc_test /tmp/qmi_loc_test && \
                  adb shell chmod 755 /tmp/qmi_loc_test && \
                  adb shell '/bin/rootshell -c \"/tmp/qmi_loc_test 2>&1\"'"
```

## Expected Output

```
QMI LOC Test - using CCI library with fake LOC service object
Target: Orbic RC400L (MDM9207)

=== Testing DMS service ===
DMS service object at 0x...:
  library_version:  6
  ...
DMS client initialized: 0x1
DMS_GET_DEVICE_MFR: rc=0 resp_len=18
  ... "Reliance"
DMS_GET_DEVICE_MODEL: rc=0 resp_len=16
  ... "RC400L"
DMS client released

DMS test passed - CCI framework is working

=== Testing LOC service (fake service object) ===
LOC client initialized: 0x2
LOC_REG_EVENTS: rc=0
LOC_START: rc=0

[LOC IND] msg_id=0x002c ... FIX SESSION STATE CHANGE
[LOC IND] msg_id=0x002b ... ENGINE STATE CHANGE
[LOC IND] msg_id=0x0026 ... NMEA: $GPVTG,...
[LOC IND] msg_id=0x0026 ... NMEA: $GPGGA,...
...
```

If LOC_START returns `result=1, error=1` (MALFORMED_MSG), check the TLV encoding — particularly that `fix_recurrence` is 1 (PERIODIC) or 2 (SINGLE), not 0.
