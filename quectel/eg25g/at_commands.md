# Quectel EG25 AT Commands (Sample List)

This document provides a sample list of AT commands supported by the Quectel EG25 modem module. This list is **not exhaustive** and is intended as a starting point. For complete and detailed information, please refer to the official Quectel AT command manuals.

## Basic & General Commands

| Command        | Function                                                                 | Source Hint (Manual)          |
|----------------|--------------------------------------------------------------------------|-------------------------------|
| AT             | Attention command, used to check if the modem is responsive.             | General AT Commands           |
| ATE[0/1]       | Enable/Disable command echo. ATE0 disables echo, ATE1 enables it.        | General AT Commands           |
| ATI            | Display product identification information.                              | General AT Commands           |
| AT+GMI         | Request Manufacturer Identification.                                     | General AT Commands           |
| AT+GMM         | Request Model Identification.                                            | General AT Commands           |
| AT+GMR         | Request Revision Identification (firmware version).                      | General AT Commands           |
| AT+GSN         | Request Product Serial Number Identification (IMEI or MEID).             | General AT Commands           |
| AT+CGSN        | Request Product Serial Number Identification (IMEI).                     | General AT Commands           |
| AT+QGSN        | Get module GSN number.                                                   | General AT Commands           |
| AT+QIMI        | Get IMEI number.                                                         | General AT Commands           |
| AT&F[n]        | Set to factory defined configuration.                                    | General AT Commands           |
| ATZ[n]         | Reset to default configuration profile.                                  | General AT Commands           |
| AT+CFUN=[<fun>[,<rst>]] | Set Phone Functionality. E.g., `AT+CFUN=1` for full functionality. | General AT Commands           |
| AT+CMEE=[<n>]  | Report Mobile Equipment Error. `AT+CMEE=1` or `AT+CMEE=2` for verbose errors. | General AT Commands           |
| AT+CPAS        | Phone Activity Status.                                                   | General AT Commands           |
| AT+QPOWD=[<n>] | Power Off the Module.                                                    | General AT Commands           |
| AT+IPR?        | Query current DTE-DCE baud rate.                                         | General AT Commands           |
| AT+IPR=<rate>  | Set DTE-DCE baud rate.                                                   | General AT Commands           |
| AT+QFORMATUSRDATA | Format user data partition.                                           | General AT Commands / File    |

## Configuration Commands (`AT+QCFG`)

The `AT+QCFG` command is a versatile command used for various module configurations. It has many sub-parameters. Below are a few examples found:

| Command        | Function                                                                 | Source Hint (Manual)          |
|----------------|--------------------------------------------------------------------------|-------------------------------|
| AT+QCFG="agps/string" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="airplanecontrol" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="amrcodec" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="apready" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="ApRstLevel" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="band" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="bip/auth" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="bootup" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="call_control" | Control voice MO (Mobile Originated) and MT (Mobile Terminated) functions. | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="cdmaruim" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="cdmasms/cmtformat" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="cmux/urcport" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="codec/powsave" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="cops_no_mode_change" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="dhcppktfltr" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="disrplmn" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="divctl" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="diversity" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="efratctl" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="ehrpd" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="eps/guti_enable" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="fast_dormancy" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="fast/poweroff" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="Feature_Switch_Flag" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="freezeband" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="ftm/mbim" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="gprsattach" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="gpsweek" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="hsdpacat" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="hsupacat" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="icf" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="ims" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="imsreg/iptype" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="iproute_enable" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="iprulectl" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="lanip" | Modify the range of LAN IP (e.g., for tethering/router mode).            | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual    |
| AT+QCFG="ledmode" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="lpm/dataind" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="lte/bandprior" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="lte/preferfre" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="ltectcc/smsstorage" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="ltesms/format" | Send SMS in 3GPP2 format only and receive SMS in 3GPP2/3GPP.    | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual    |
| AT+QCFG="mms_rec_control" | Control whether to save received MMS.                           | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual    |
| AT+QCFG="ModemRstLevel" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="msc" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="multi_ip_package" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="noauthcheck" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="ntp" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="nwoptmz/acq" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="nwscanmode" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="nwscanmodeex" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="nwscanseq" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="oostimer" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="pcmclk" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="pdp/duplicatechk" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="plmn/addinfbdn" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="plmn/autoblock" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="poaccept" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="ppp/sleep_ri" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="ppp/termframe" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="ppp/v4v6" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="qcautoconnect" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="qmisync" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="qoos" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="rf/sar/gpioctl" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="risignaltype" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="roamservice" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="rrc" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="rrc/control" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="rssi" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="sarcfg" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="secbootstat" | Get whether secure boot is enabled.                               | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual    |
| AT+QCFG="servicedomain" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="sgsn" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="sim/clk_freq" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="sim/onchip" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="sim/recovery" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="siminvalirecovery" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="sleep/datactrl" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="sleepind/level" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="sms_control" | Control SMS sending and reception.                                | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual    |
| AT+QCFG="SMS/ListMsgMap" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="spi/set" | Write a mark in rawdata to control initialization of SPI6/UART6.       | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual    |
| AT+QCFG="stkauto/setupmenutr" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="TCP/SendMode" | Configure TCP sending mode.                                      | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual    |
| AT+QCFG="tcp/windowsize" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="tdscsq" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="thermal/limit_rates" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="thermal/modem" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="thermal/txpwrlmt" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="tone/incoming" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="urc_cause_support" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="urc/cache" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="urc/delay" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="urc/ri/other" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="urc/ri/pin" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="urc/ri/ring" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="urc/ri/smsincoming" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="urcport/sms" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="usage/apfs" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="usage/apmem" | Get device memory usage.                                          | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual    |
| AT+QCFG="usbcfg" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="usbee" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="usbid" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="usbmode" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="usbnet" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="voice_busytone" | Enable/disable busytone playback.                             | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual    |
| AT+QCFG="volte/disable" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="vts/async" |  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual |
| AT+QCFG="wakeupin/level" | Set the wakeup_in pin level.                                  | EC2x&EG2x&EG9x&EM05 Series QCFG AT Commands Manual    |

**Note on `AT+QCFG`:** The `AT+QCFG` command has numerous sub-parameters for configuring various aspects of the module. The examples above are just a small selection. Refer to the "Quectel_EC2x&EG9x&EG2x-G&EM05_Series_QCFG_AT_Commands_Manual" (or similar specific QCFG manual if available) for a comprehensive list.


## SIM Card Commands

| Command        | Function                                                                 | Source Hint (Manual)          |
|----------------|--------------------------------------------------------------------------|-------------------------------|
| AT+CPIN?       | Query SIM card status (e.g., READY, SIM PIN, SIM PUK).                   | General AT Commands           |
| AT+CPIN=<pin>[,<newpin>] | Enter SIM PIN or PUK, or change PIN.                            | General AT Commands           |
| AT+QSIMSTAT?   | Query SIM card insertion status.                                         | General AT Commands           |
| AT+QCCID       | Read ICCID from SIM card.                                                | General AT Commands           |
| AT+QSIMCHK     | Send UIM_AUTHENTICATE_REQ (UIM authentication).                          | General AT Commands / QCFG    |
| AT+QSIMWRFREQ  | Record the frequency of EF files written to SIM card.                    | General AT Commands / QCFG    |
| AT+QSIMVOL     | Configure SIM card supply voltage.                                       | General AT Commands / QCFG    |

## Network Service Commands

| Command        | Function                                                                 | Source Hint (Manual)          |
|----------------|--------------------------------------------------------------------------|-------------------------------|
| AT+COPS?       | Query current operator.                                                  | General AT Commands           |
| AT+COPS=[<mode>[,<format>[,<oper>[,<AcT>]]]] | Set operator (manual/automatic selection). | General AT Commands           |
| AT+CREG?       | Network Registration status.                                             | General AT Commands           |
| AT+CGREG?      | GPRS Network Registration status.                                        | General AT Commands           |
| AT+CEREG?      | EPS (LTE) Network Registration status.                                   | General AT Commands           |
| AT+CSQ         | Signal Quality Report (RSSI, BER).                                       | General AT Commands           |
| AT+QNWINFO     | Query Network Information (Access Technology, Operator, Band).           | General AT Commands           |
| AT+QENG="servingcell" | Query serving cell information.                                   | General AT Commands           |
| AT+QOPS        | Scan band.                                                               | General AT Commands / QCFG    |
| AT+QOPSCFG="scancontrol" | Configure bands to be scanned under 2G/3G/4G.                  | General AT Commands / QCFG    |
| AT+QOPSCFG="displayrssi" | Enable/disable display of RSSI under LTE.                      | General AT Commands / QCFG    |
| AT+QEGPAPN     | Query the actual APN and the configured APN.                             | General AT Commands / QCFG    |

## Packet Domain (Data) Commands

| Command        | Function                                                                 | Source Hint (Manual)          |
|----------------|--------------------------------------------------------------------------|-------------------------------|
| AT+CGDCONT?    | Show PDP Context definitions.                                            | General AT Commands           |
| AT+CGDCONT=[<cid>[,<PDP_type>[,<APN>...]]] | Define PDP Context.                         | General AT Commands           |
| AT+CGACT=[<state>[,<cid>]] | Activate or Deactivate PDP Context.                         | General AT Commands           |
| AT+CGATT=[<state>] | Attach or Detach from GPRS Service.                                  | General AT Commands           |
| AT+QICSGP=[<contextID>[,<context_type>...]] | Configure Parameters of a TCP/IP Context.   | General AT Commands (TCP/IP)  |
| AT+QIACT=[<contextID>] | Activate a PDP Context.                                         | General AT Commands (TCP/IP)  |
| AT+QIDEACT=[<contextID>] | Deactivate a PDP Context.                                     | General AT Commands (TCP/IP)  |
| AT+QICFG="tcp/keepalive" | Configure whether to enable TCP keepalive function.            | General AT Commands / QCFG    |
| AT+QIKALIVE    | Change to the minimum keepalive value.                                   | General AT Commands / QCFG    |
| AT+QSSLRECV    | Read the remaining SSL data.                                             | General AT Commands / QCFG    |
| AT+QSSLCFG="alpn" | Set SSL ALPN (Application-Layer Protocol Negotiation).                | General AT Commands / QCFG    |

## SMS Commands

| Command        | Function                                                                 | Source Hint (Manual)          |
|----------------|--------------------------------------------------------------------------|-------------------------------|
| AT+CMGF=[<mode>] | Set Message Format (0 for PDU mode, 1 for Text mode).                  | General AT Commands           |
| AT+CMGS=<da>[,<toda>] (Text Mode) | Send Message.                                         | General AT Commands           |
| AT+CMGS=<length> (PDU Mode) | Send Message.                                               | General AT Commands           |
| AT+CMGL=[<stat>] | List Messages from preferred storage.                                  | General AT Commands           |
| AT+CMGR=<index>  | Read Message.                                                            | General AT Commands           |
| AT+CMGD=<index>[,<delflag>] | Delete Message.                                             | General AT Commands           |
| AT+CNMI=[<mode>[,<mt>[,<bm>[,<ds>[,<bfr>]]]]] | New Message Indications to TE.            | General AT Commands           |


## Voice Call Commands (Subset)

| Command        | Function                                                                 | Source Hint (Manual)          |
|----------------|--------------------------------------------------------------------------|-------------------------------|
| ATD<number>;   | Dial a voice call.                                                       | General AT Commands           |
| ATH            | Hang up call.                                                            | General AT Commands           |
| ATA            | Answer incoming call.                                                    | General AT Commands           |
| AT+CLCC        | List Current Calls.                                                      | General AT Commands           |


## File System Commands (from File AT Commands Manual)

| Command        | Function                                                                 | Source Hint (Manual)          |
|----------------|--------------------------------------------------------------------------|-------------------------------|
| AT+QFLDS="<storage>" | Get the Space Information of the Storage (e.g., "UFS", "RAM", "SD"). | File AT Commands              |
| AT+QFLST["<filename_pattern>"] | List the File Information in the Storage.                 | File AT Commands              |
| AT+QFDEL="<filename>" | Delete the File(s) in the Storage.                                | File AT Commands              |
| AT+QFUPL="<filename>",<size>[,<timeout>] | Upload a File to the Storage.                  | File AT Commands              |
| AT+QFDWL="<filename>" | Download a File from the Storage.                                 | File AT Commands              |
| AT+QFOPEN=<filename>[,<mode>] | Open/Create a file.                                      | File AT Commands              |
| AT+QFREAD=<handle>,<read_length> | Read data from file.                                  | File AT Commands              |
| AT+QFWRITE=<handle>,<write_length>[,<timeout>[,<eof>]] | Write data to file.             | File AT Commands              |
| AT+QFCLOSE=<handle> | Close file.                                                          | File AT Commands              |
| AT+QFSEEK=<handle>,<offset>,<relation> | Set file pointer position.                       | File AT Commands              |
| AT+QFDEL="*.*" | Delete all files in the current directory of default storage.            | File AT Commands              |
| AT+QFOTADL     | Download FTP file to UFS (often used for FOTA - Firmware Over The Air).  | General AT Commands / QCFG    |

## GNSS (GPS/GLONASS/BeiDou/Galileo/QZSS) Commands (from GNSS Application Note)

| Command        | Function                                                                 | Source Hint (Manual)          |
|----------------|--------------------------------------------------------------------------|-------------------------------|
| AT+QGPS=[<n>]  | Turn ON/OFF GNSS engine. `AT+QGPS=1` to turn on.                         | GNSS Application Note         |
| AT+QGPS?       | Query GNSS power status.                                                 | GNSS Application Note         |
| AT+QGPSLOC=[<mode>[,<timeout>...]] | Get GNSS Location Information.                       | GNSS Application Note         |
| AT+QGPSINFO    | Get specific GNSS information (e.g. fix status, satellites).             | GNSS Application Note         |
| AT+QGPSGNMEA="<type>" | Get NMEA sentence (e.g., "GGA", "RMC", "GSV", "GSA", "VTG").     | GNSS Application Note         |
| AT+QGPSCFG="<cfg_param>"[,<value>] | Configure GNSS parameters. Examples below:           | GNSS Application Note         |
| AT+QGPSCFG="outport",<port> | Configure NMEA sentence output port ("none", "usbnmea", "uartdebug"). | GNSS Application Note         |
| AT+QGPSCFG="nmeasrc",<enable> | Enable/Disable Acquisition of NMEA Sentences (0=disable, 1=enable). | GNSS Application Note         |
| AT+QGPSCFG="gpsnmeatype",<bitmap> | Configure type of GPS NMEA sentences to output.       | GNSS Application Note         |
| AT+QGPSCFG="glonassnmeatype",<bitmap> | Configure type of GLONASS NMEA sentences.         | GNSS Application Note         |
| AT+QGPSCFG="galileonmeatype",<bitmap> | Configure type of Galileo NMEA sentences.         | GNSS Application Note         |
| AT+QGPSCFG="beidounmeatype",<bitmap> | Configure type of BeiDou NMEA sentences.           | GNSS Application Note         |
| AT+QGPSCFG="gnssconfig",<config> | Configure combined GNSS mode (e.g., GPS+GLONASS).       | GNSS Application Note         |
| AT+QGPSCFG="odpcontrol",<enable> | Configure ODP (On-Demand Positioning) control.         | GNSS Application Note         |
| AT+QGPSCFG="dpoenable",<enable> | Enable/Disable DPO (Dynamic Power Optimization).        | GNSS Application Note         |
| AT+QGPSCFG="plane",<plane_type> | Configure GNSS positioning plane (User/Control plane).   | GNSS Application Note         |
| AT+QGPSCFG="autogps",<enable> | Configure Auto GPS (autonomous or MS-based).              | GNSS Application Note         |
| AT+QGPSCFG="suplver",<version> | Configure SUPL version for A-GPS.                         | GNSS Application Note         |
| AT+QGPSCFG="agpsposmode",<mode> | Configure A-GPS positioning mode.                        | GNSS Application Note         |
| AT+QGPSCFG="fixfreq",<freq_in_hz> | Configure positioning fix frequency (e.g., 1 Hz).     | GNSS Application Note         |
| AT+QGPSEND     | Turn OFF GNSS engine.                                                    | GNSS Application Note         |
| AT+QGPSXTRA=[<n>] | Enable/Disable XTRA function.                                         | GNSS Application Note         |
| AT+QGPSXTRADATA? | Query XTRA data information.                                           | GNSS Application Note         |
| AT+QGPSDEL=<type> | Delete GNSS aiding data (e.g., time, location, almanac, ephemeris). | GNSS Application Note         |
| AT+QGPSPAI     | Obtain GNSS positioning assistance information.                          | General AT Commands / QCFG    |
| AT+QLBSCFG="contextid",<id> | Configure QuecLocator context ID.                          | General AT Commands / QCFG    |
| AT+QLBSEX      | Enter cell parameter for positioning (QuecLocator).                      | General AT Commands / QCFG    |


## Unsorted Commands

| Command        | Function                                                                 | Source Hint (Manual)          |
|----------------|--------------------------------------------------------------------------|-------------------------------|
| AT+QNVFR | Read NV items.                                                        | General AT Commands           |
| AT+QNVR | Read NV items with a specific index.                                 | General AT Commands           |
| AT+QNVFW | Write NV items.                                                       | General AT Commands           |
| AT+QNVFD | Delete NV items.                                                     | General AT Commands           |
| AT+QMBNCFG="select" | Show which MBN is currently selected.                          | General AT Commands           |
| at+qnvfr="nv/item_files/Thin_UI/enable_thin_ui_cfg" | Check status of Thin UI. | General AT Commands           |
| at+qnvfw="nv/item_files/Thin_UI/enable_thin_ui_cfg",<value> | Set status of Thin UI. | General AT Commands           |
| at+qping=<ipaddr>[,<count>[,<size>]] | Ping a host.                                                   | General AT Commands           |
| AT+QVOLTEDBG | Dump a ton of info.                                          | General AT Commands           |
| AT+CIMI | Get the International Mobile Subscriber Identity (IMSI).       | General AT Commands           |
| at+QCPUSN? | Get the CPU serial number.                               | General AT Commands           |
| AT+QWIFICFG="bridgemode" | Support Wi-Fi bridge mode (if Wi-Fi is a feature of the specific EG25 variant). | |
| AT+QMTCFG="protocol/check" | Configure MQTT protocol check.                               |     |


## Huge List of Commands

```plaintext
AT&V
AT+CCLK?
AT+CEREG?
AT+CEREG=?
AT+CFUN?
AT+CFUN=?
AT+CGATT?
AT+CGATT=?
AT+CGDCONT?
AT+CGMI
AT+CGMI?
AT+CGMM
AT+CGMM?
AT+CGMR
AT+CGMR?
AT+CGREG?
AT+CGREG=?
AT+CGSN
AT+CGSN?
AT+CGSN=?
AT+CGSN=0
AT+CGSN=1
AT+CIMI
AT+CIND?
AT+CIND=?
AT+CLCK=?
AT+CLCK="AB",2
AT+CLCK="AC",2
AT+CLCK="AG",2
AT+CLCK="AI",2
AT+CLCK="AO",2
AT+CLCK="FD",2
AT+CLCK="IR",2
AT+CLCK="OI",2
AT+CLCK="OX",2
AT+CLCK="PC",2
AT+CLCK="PF",2
AT+CLCK="PN",2
AT+CLCK="PP",2
AT+CLCK="PU",2
AT+CLCK="SC",2
AT+CMEE?
AT+CMEE=?
AT+COPN
AT+COPS?
AT+COPS=?
AT+CPIN?
AT+CPOL?
AT+CPOL=?
AT+CREG?
AT+CREG=?
AT+CSCS?
AT+CSQ?
AT+CSQ=?
AT+CTZU?
AT+CTZU=?
AT+GMI
AT+GMM
AT+GMR
AT+GSN
AT+GSN=?
AT+GSN=0
AT+GSN=1
AT+QAUDCFG=?
AT+QAUDCFG="alc5616/dlgain"
AT+QAUDCFG="alc5616/pwrctr"
AT+QAUDCFG="alc5616/ulgain"
AT+QAUDCFG="decgain"
AT+QAUDCFG="encgain"
AT+QAUDCFG="fns"
AT+QAUDCFG="nau8810/config"
AT+QAUDCFG="nau8814/aoutput"
AT+QAUDCFG="nau8814/dlgain"
AT+QAUDCFG="tonevolume"
AT+QAUDCFG="voltedtmfcfg"
AT+QBTLEADDR?
AT+QBTNAME?
AT+QBTPWR?
AT+QBTPWR=?
AT+QCCID
AT+QCFG=?
AT+QCFG="agps/string"
AT+QCFG="airplanecontrol"
AT+QCFG="amrcodec"
AT+QCFG="apready"
AT+QCFG="ApRstLevel"
AT+QCFG="band"
AT+QCFG="bip/auth"
AT+QCFG="bootup"
AT+QCFG="call_control"
AT+QCFG="cdmaruim"
AT+QCFG="cdmasms/cmtformat"
AT+QCFG="cmux/urcport"
AT+QCFG="codec/powsave"
AT+QCFG="cops_no_mode_change"
AT+QCFG="dhcppktfltr"
AT+QCFG="disrplmn"
AT+QCFG="divctl"
AT+QCFG="diversity"
AT+QCFG="efratctl"
AT+QCFG="ehrpd"
AT+QCFG="eps/guti_enable"
AT+QCFG="fast_dormancy"
AT+QCFG="fast/poweroff"
AT+QCFG="Feature_Switch_Flag"
AT+QCFG="freezeband"
AT+QCFG="ftm/mbim"
AT+QCFG="gprsattach"
AT+QCFG="gpsweek"
AT+QCFG="hsdpacat"
AT+QCFG="hsupacat"
AT+QCFG="icf"
AT+QCFG="ims"
AT+QCFG="imsreg/iptype"
AT+QCFG="iproute_enable"
AT+QCFG="iprulectl"
AT+QCFG="lanip"
AT+QCFG="ledmode"
AT+QCFG="lpm/dataind"
AT+QCFG="lte/bandprior"
AT+QCFG="lte/preferfre"
AT+QCFG="ltectcc/smsstorage"
AT+QCFG="ltesms/format"
AT+QCFG="mms_rec_control"
AT+QCFG="ModemRstLevel"
AT+QCFG="msc"
AT+QCFG="multi_ip_package"
AT+QCFG="noauthcheck"
AT+QCFG="ntp"
AT+QCFG="nwoptmz/acq"
AT+QCFG="nwscanmode"
AT+QCFG="nwscanmodeex"
AT+QCFG="nwscanseq"
AT+QCFG="oostimer"
AT+QCFG="pcmclk"
AT+QCFG="pdp/duplicatechk"
AT+QCFG="plmn/addinfbdn"
AT+QCFG="plmn/autoblock"
AT+QCFG="poaccept"
AT+QCFG="ppp/sleep_ri"
AT+QCFG="ppp/termframe"
AT+QCFG="ppp/v4v6"
AT+QCFG="qcautoconnect"
AT+QCFG="qmisync"
AT+QCFG="qoos"
AT+QCFG="rf/sar/gpioctl"
AT+QCFG="risignaltype"
AT+QCFG="roamservice"
AT+QCFG="rrc"
AT+QCFG="rrc/control"
AT+QCFG="rssi"
AT+QCFG="sarcfg"
AT+QCFG="secbootstat"
AT+QCFG="servicedomain"
AT+QCFG="sgsn"
AT+QCFG="sim/clk_freq"
AT+QCFG="sim/onchip"
AT+QCFG="sim/recovery"
AT+QCFG="siminvalirecovery"
AT+QCFG="sleep/datactrl"
AT+QCFG="sleepind/level"
AT+QCFG="sms_control"
AT+QCFG="SMS/ListMsgMap"
AT+QCFG="spi/set"
AT+QCFG="stkauto/setupmenutr"
AT+QCFG="TCP/SendMode"
AT+QCFG="tcp/windowsize"
AT+QCFG="tdscsq"
AT+QCFG="thermal/limit_rates"
AT+QCFG="thermal/modem"
AT+QCFG="thermal/txpwrlmt"
AT+QCFG="tone/incoming"
AT+QCFG="urc_cause_support"
AT+QCFG="urc/cache"
AT+QCFG="urc/delay"
AT+QCFG="urc/ri/other"
AT+QCFG="urc/ri/pin"
AT+QCFG="urc/ri/ring"
AT+QCFG="urc/ri/smsincoming"
AT+QCFG="urcport/sms"
AT+QCFG="usage/apfs"
AT+QCFG="usage/apmem"
AT+QCFG="usbcfg"
AT+QCFG="usbee"
AT+QCFG="usbid"
AT+QCFG="usbmode"
AT+QCFG="usbnet"
AT+QCFG="voice_busytone"
AT+QCFG="volte/disable"
AT+QCFG="vts/async"
AT+QCFG="wakeupin/level"
AT+QCSQ
AT+QCSQ?
AT+QCSQ=?
AT+QENG="3gcomm"
AT+QENG="neighbourcell"
AT+QENG="servingcell"
AT+QFLDS
AT+QFLDS="RAM"
AT+QFLDS="SD"
AT+QFLDS="UFS"
AT+QFLST="RAM:*"
AT+QFLST="SD:*"
AT+QFLST="UFS:*"
AT+QFPLMNCFG=?
AT+QFPLMNCFG="list"
AT+QFTPCFG=?
AT+QFTPCFG="account"
AT+QFTPCFG="contextid"
AT+QFTPCFG="data_address"
AT+QFTPCFG="filetype"
AT+QFTPCFG="restenable"
AT+QFTPCFG="rsptimeout"
AT+QFTPCFG="sslctxid"
AT+QFTPCFG="ssltype"
AT+QFTPCFG="transmode"
AT+QFTPSTAT
AT+QFWDCFG=?
AT+QFWDCFG="urc_with_len"
AT+QFWDCFG="urc"
AT+QFWDCLIENT?
AT+QFWDCLIENT=?
AT+QFWDSERVER?
AT+QFWDSERVER=?
AT+QGPS?
AT+QGPS=?
AT+QGPSCFG=?
AT+QGPSCFG="1pps"
AT+QGPSCFG="agnssjamming"
AT+QGPSCFG="agnssjammingurcmode"
AT+QGPSCFG="agnssprotocol"
AT+QGPSCFG="agpsposmode"
AT+QGPSCFG="appidname"
AT+QGPSCFG="autogps"
AT+QGPSCFG="beidounmeatype"
AT+QGPSCFG="dpoenable"
AT+QGPSCFG="estimation_error"
AT+QGPSCFG="fixfreq"
AT+QGPSCFG="galileonmeatype"
AT+QGPSCFG="glonassnmeatype"
AT+QGPSCFG="gnssconfig"
AT+QGPSCFG="gnssrawdata"
AT+QGPSCFG="gpsnmeatype"
AT+QGPSCFG="gsvextnmeatype"
AT+QGPSCFG="lbsapn"
AT+QGPSCFG="nmeasrc"
AT+QGPSCFG="odpcontrol"
AT+QGPSCFG="outport"
AT+QGPSCFG="plane"
AT+QGPSCFG="suplver"
AT+QGPSGNMEA=?
AT+QGPSGNMEA="GGA"
AT+QGPSGNMEA="GNS"
AT+QGPSGNMEA="GSA"
AT+QGPSGNMEA="GSV"
AT+QGPSGNMEA="RMC"
AT+QGPSGNMEA="VTG"
AT+QGPSSUPLCA?
AT+QGPSSUPLURL?
AT+QGPSSUPLURL=?
AT+QGPSXTRA?
AT+QGPSXTRA=?
AT+QGPSXTRADATA?
AT+QGPSXTRADATA=?
AT+QGPSXTRATIME?
AT+QGPSXTRATIME=?
AT+QHTTPCFG="auth"
AT+QHTTPCFG="closed/ind"
AT+QHTTPCFG="closewaittime"
AT+QHTTPCFG="contenttype"
AT+QHTTPCFG="contextid"
AT+QHTTPCFG="custom_header"
AT+QHTTPCFG="requestheader"
AT+QHTTPCFG="responseheader"
AT+QHTTPCFG="rspout/auto"
AT+QHTTPCFG="sslctxid"
AT+QHTTPCFG="windowsize"
at+QHTTPCFGEX=?
at+QHTTPCFGEX="send_add"
at+QHTTPCFGEX="send_del"
AT+QHTTPURL?
AT+QIACT?
AT+QICFG=?
AT+QICFG="dataformat"
AT+QICFG="dns/cache"
AT+QICFG="passiveclosed"
AT+QICFG="qisend/timeout"
AT+QICFG="recv/buffersize"
AT+QICFG="recvind"
AT+QICFG="tcp/accept"
AT+QICFG="tcp/keepalive"
AT+QICFG="tcp/retranscfg"
AT+QICFG="transpktsize"
AT+QICFG="transwaittm"
AT+QICFG="udp/readmode"
AT+QICFG="udp/sendmode"
AT+QICFG="viewmode"
AT+QICFG="window/scale"
at+QICSGP=?
at+QICSGP=1
at+QICSGP=2
at+QICSGP=3
at+QICSGP=4
at+QICSGP=5
at+QICSGP=6
at+QICSGP=7
at+QICSGP=8
at+QICSGP=9
at+QICSGP=10
at+QICSGP=11
at+QICSGP=12
at+QICSGP=13
at+QICSGP=14
at+QICSGP=15
at+QICSGP=16
AT+QINDCFG=?
AT+QINDCFG="act"
AT+QINDCFG="all"
AT+QINDCFG="ccinfo"
AT+QINDCFG="csq"
AT+QINDCFG="ring"
AT+QINDCFG="smsfull"
AT+QINDCFG="smsincoming"
AT+QINISTAT
AT+QINISTAT=?
AT+QLBSCFG=?
AT+QLBSCFG="asynch"
AT+QLBSCFG="contextid"
AT+QLBSCFG="latorder"
AT+QLBSCFG="server"
AT+QLBSCFG="timeout"
AT+QLBSCFG="timeupdate"
AT+QLBSCFG="token"
AT+QLBSCFG="withtime"
AT+QLTS
AT+QLTS=?
AT+QLWCFG=?
AT+QLWCFG="apnretry"
AT+QLWCFG="epname/mode"
AT+QLWCFG="fota"
AT+QLWCFG="hostdevice"
AT+QLWCFG="maxreconntime"
AT+QLWCFG="nettype"
AT+QLWCFG="reset"
AT+QLWCFG="security"
AT+QLWCFG="server"
AT+QLWCFG="startup"
AT+QLWCFG="urc"
AT+QLWSTAT?
AT+QMBNCFG="AutoSel"
AT+QMBNCFG="List_all"
AT+QMBNCFG="List"
AT+QMBNCFG="Select"
at+QMMSCFG=?
at+QMMSCFG="character"
at+QMMSCFG="connecttimeout"
at+QMMSCFG="contextid"
at+QMMSCFG="mmsc"
at+QMMSCFG="proxy"
at+QMMSCFG="sendparam"
at+QMMSCFG="supportfield"
AT+QMTCFG=?
AT+QMTCFG="aliauth"
AT+QMTCFG="hwauth"
AT+QMTCFG="hwprodid"
AT+QMTCFG="keepalive"
AT+QMTCFG="pdpcid"
AT+QMTCFG="qmtping"
AT+QMTCFG="recv/mode"
AT+QMTCFG="send/mode"
AT+QMTCFG="session"
AT+QMTCFG="ssl"
AT+QMTCFG="timeout"
AT+QMTCFG="version"
AT+QMTCFG="will"
AT+QMTCFG="willex"
AT+QMTCONN?
AT+QMTCONN=?
AT+QMTOPEN?
AT+QMTOPEN=?
AT+QMTRECV?
AT+QMTSUB=?
AT+QNETINFO=0,1F
AT+QNETINFO=1,1F
AT+QNETINFO=2,1F
AT+QNETINFO=3,1F
AT+QNETINFO=4,1F
AT+QNETINFO=5,1F
AT+QNETINFO=6,1F
AT+QNVFR="/nv/item_files/ims/IMS_enable"
AT+QNVFR="/nv/item_files/modem/mmode/sms_only"
AT+QNVFR="/nv/item_files/modem/mmode/ue_usage_setting"
AT+QNVR=5280,0
AT+QNWCFG=?
AT+QNWINFO
AT+QNWLOCK="common/4g"
AT+QNWLOCK="common/lte"
AT+QOPS?
AT+QOPSCFG=?
AT+QOPSCFG="displaybw"
AT+QOPSCFG="displayrssi"
AT+QOPSCFG="scancontrol"
AT+QPINC=?
AT+QPINC="P2"
AT+QPINC="SC"
AT+QSCLK?
AT+QSCLK=?
AT+QSIMDET?
AT+QSIMDET=?
AT+QSIMSTAT?
AT+QSIMSTAT=?
AT+QSIMVOL?
AT+QSIMVOL=?
AT+QSMTPCFG=?
AT+QSMTPCFG="account"
AT+QSMTPCFG="contextid"
AT+QSMTPCFG="sender"
AT+QSMTPCFG="smtpserver"
AT+QSMTPCFG="sslctxid"
AT+QSMTPCFG="ssltype"
AT+QSPN
AT+QSSLCFG=?
AT+QSSLCFG="alpn",0
AT+QSSLCFG="alpn",1
AT+QSSLCFG="alpn",2
AT+QSSLCFG="alpn",3
AT+QSSLCFG="alpn",4
AT+QSSLCFG="alpn",5
AT+QSSLCFG="cacert",0
AT+QSSLCFG="cacert",1
AT+QSSLCFG="cacert",2
AT+QSSLCFG="cacert",3
AT+QSSLCFG="cacert",4
AT+QSSLCFG="cacert",5
AT+QSSLCFG="cacertex",0
AT+QSSLCFG="cacertex",1
AT+QSSLCFG="cacertex",2
AT+QSSLCFG="cacertex",3
AT+QSSLCFG="cacertex",4
AT+QSSLCFG="cacertex",5
AT+QSSLCFG="ciphersuite",0
AT+QSSLCFG="ciphersuite",1
AT+QSSLCFG="ciphersuite",2
AT+QSSLCFG="ciphersuite",3
AT+QSSLCFG="ciphersuite",4
AT+QSSLCFG="ciphersuite",5
AT+QSSLCFG="clientcert",0
AT+QSSLCFG="clientcert",1
AT+QSSLCFG="clientcert",2
AT+QSSLCFG="clientcert",3
AT+QSSLCFG="clientcert",4
AT+QSSLCFG="clientcert",5
AT+QSSLCFG="clientkey",0
AT+QSSLCFG="clientkey",1
AT+QSSLCFG="clientkey",2
AT+QSSLCFG="clientkey",3
AT+QSSLCFG="clientkey",4
AT+QSSLCFG="clientkey",5
AT+QSSLCFG="closetimemode",0
AT+QSSLCFG="closetimemode",1
AT+QSSLCFG="closetimemode",2
AT+QSSLCFG="closetimemode",3
AT+QSSLCFG="closetimemode",4
AT+QSSLCFG="closetimemode",5
AT+QSSLCFG="dtls",0
AT+QSSLCFG="dtls",1
AT+QSSLCFG="dtls",2
AT+QSSLCFG="dtls",3
AT+QSSLCFG="dtls",4
AT+QSSLCFG="dtls",5
AT+QSSLCFG="dtlsversion",0
AT+QSSLCFG="dtlsversion",1
AT+QSSLCFG="dtlsversion",2
AT+QSSLCFG="dtlsversion",3
AT+QSSLCFG="dtlsversion",4
AT+QSSLCFG="dtlsversion",5
AT+QSSLCFG="ignoreinvalidcertsign",0
AT+QSSLCFG="ignoreinvalidcertsign",1
AT+QSSLCFG="ignoreinvalidcertsign",2
AT+QSSLCFG="ignoreinvalidcertsign",3
AT+QSSLCFG="ignoreinvalidcertsign",4
AT+QSSLCFG="ignoreinvalidcertsign",5
AT+QSSLCFG="ignorelocaltime",0
AT+QSSLCFG="ignorelocaltime",1
AT+QSSLCFG="ignorelocaltime",2
AT+QSSLCFG="ignorelocaltime",3
AT+QSSLCFG="ignorelocaltime",4
AT+QSSLCFG="ignorelocaltime",5
AT+QSSLCFG="ignoremulticertchainverify",0
AT+QSSLCFG="ignoremulticertchainverify",1
AT+QSSLCFG="ignoremulticertchainverify",2
AT+QSSLCFG="ignoremulticertchainverify",3
AT+QSSLCFG="ignoremulticertchainverify",4
AT+QSSLCFG="ignoremulticertchainverify",5
AT+QSSLCFG="negotiatetime",0
AT+QSSLCFG="negotiatetime",1
AT+QSSLCFG="negotiatetime",2
AT+QSSLCFG="negotiatetime",3
AT+QSSLCFG="negotiatetime",4
AT+QSSLCFG="negotiatetime",5
AT+QSSLCFG="psk",0
AT+QSSLCFG="psk",1
AT+QSSLCFG="psk",2
AT+QSSLCFG="psk",3
AT+QSSLCFG="psk",4
AT+QSSLCFG="psk",5
AT+QSSLCFG="renegotiation",0
AT+QSSLCFG="renegotiation",1
AT+QSSLCFG="renegotiation",2
AT+QSSLCFG="renegotiation",3
AT+QSSLCFG="renegotiation",4
AT+QSSLCFG="renegotiation",5
AT+QSSLCFG="seclevel",0
AT+QSSLCFG="seclevel",1
AT+QSSLCFG="seclevel",2
AT+QSSLCFG="seclevel",3
AT+QSSLCFG="seclevel",4
AT+QSSLCFG="seclevel",5
AT+QSSLCFG="session_cache",0
AT+QSSLCFG="session_cache",1
AT+QSSLCFG="session_cache",2
AT+QSSLCFG="session_cache",3
AT+QSSLCFG="session_cache",4
AT+QSSLCFG="session_cache",5
AT+QSSLCFG="sni",0
AT+QSSLCFG="sni",1
AT+QSSLCFG="sni",2
AT+QSSLCFG="sni",3
AT+QSSLCFG="sni",4
AT+QSSLCFG="sni",5
AT+QSSLCFG="sslversion"
AT+QTEMP
AT+QTTS?
AT+QTTS=?
AT+QTTSETUP?
AT+QTTSETUP=?
AT+QTTSETUP=2,1
AT+QTTSETUP=2,2
AT+QTTSETUP=2,3
AT+QURCCFG=?
AT+QURCCFG="urcport"
AT+QWIFICFG=?
AT+QWIFICFG="auth"
AT+QWIFICFG="bandwidth"
AT+QWIFICFG="bridgemode"
AT+QWIFICFG="cc"
AT+QWIFICFG="channel"
AT+QWIFICFG="dongle_mac"
AT+QWIFICFG="macacl"
AT+QWIFICFG="maxsta"
AT+QWIFICFG="mode"
AT+QWIFICFG="powerboot"
AT+QWIFICFG="scan"
AT+QWIFICFG="scanresult"
AT+QWIFICFG="speedlimit"
AT+QWIFICFG="ssid"
AT+QWIFICFG="ssidhide"
AT+QWIFICFG="stainfo"
AT+QWIFICFG="stastatus"
AT+QWIFICFG="version"
AT+QWIFICFG="workmode"
AT+QWTTS?
AT+QWTTS=?
ATI
AT+CVERSION
AT+QNAND=?
AT+QNAND="FlashId"
AT+CSUBAT+QLINUXCMD
```

---

**Disclaimer:**

* This list is based on publicly available documents and common knowledge for this/these modules.
* Command availability and behavior is often different between firmware versions or specific module sub-variants.
* **Always refer to the official manufacturer PDF manuals for your module as the definitive reference.**
