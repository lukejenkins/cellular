# This file contains the outputs of various commands on various modules in this series

All command outputs are from the same module and firmware version. In particular I'm interested in the commands that provide a list of values to configure or set as these don't always get documented.

## Quectel EG25-G

### ATI

```plaintext
Quectel
EG25
Revision: EG25GGBR07A08M2G
```

### AT+QGMR

```plaintext
EG25GGBR07A08M2G_01.003.01.003
```

### AT+QMBNCFG=?

```plaintext
+QMBNCFG: "List"
+QMBNCFG: "Select"[,<MBN name>]
+QMBNCFG: "Deactivate"
+QMBNCFG: "AutoSel"[,(0,1)]
+QMBNCFG: "Delete","<MBN name>"
+QMBNCFG: "Add","<filename>"
```

### AT+QMBNCFG="List"

```plaintext
+QMBNCFG: "List",0,0,0,"ROW_Generic_3GPP",0x05040820,202009031
+QMBNCFG: "List",1,0,0,"VoLTE-ATT",0x0501033C,202004261
+QMBNCFG: "List",2,0,0,"hVoLTE-Verizon",0x05010141,202011021
+QMBNCFG: "List",3,0,0,"Sprint-VoLTE",0x05010205,202010201
+QMBNCFG: "List",4,1,1,"Commercial-TMO_VoLTE",0x05010505,201811231
+QMBNCFG: "List",5,0,0,"Telus-Commercial_DO",0x05800C60,202009101
+QMBNCFG: "List",6,0,0,"Commercial-SBM",0x05011C18,202004211
+QMBNCFG: "List",7,0,0,"Commercial-DT",0x05011F1C,202006281
+QMBNCFG: "List",8,0,0,"Reliance_OpnMkt",0x05011B38,202003251
+QMBNCFG: "List",9,0,0,"TF_Germany_VoLTE",0x05010C1B,202004151
+QMBNCFG: "List",10,0,0,"TF_Spain_VoLTE",0x05010CFA,201909261
+QMBNCFG: "List",11,0,0,"Volte_OpenMkt-Commercial-CMCC",0x05012071,202004161
+QMBNCFG: "List",12,0,0,"OpenMkt-Commercial-CT",0x05011322,202001171
+QMBNCFG: "List",13,0,0,"OpenMkt-Commercial-CU",0x05011505,202001171
+QMBNCFG: "List",14,0,0,"Telstra-Commercial_VoLTE",0x0580079E,202009211
+QMBNCFG: "List",15,0,0,"Commercial-KDDI",0x0501071D,202004041
+QMBNCFG: "List",16,0,0,"Commercial-SKT",0x05012715,202007071
+QMBNCFG: "List",17,0,0,"Commercial-KT",0x05012C0D,202007211
+QMBNCFG: "List",18,0,0,"Commercial-LGU",0x05012626,202007071
+QMBNCFG: "List",19,0,0,"Commercial-USCC",0x05040D20,202011091
```

### AT+QCFG="band"

```plaintext
+QCFG: "band",0xbff,0x1e00b0e18df,0x0
```

### AT+QCFG=?

```plaintext
+QCFG: "gprsattach",(0,1)
+QCFG: "nwscanmode",(0-8),(0,1)
+QCFG: "nwscanseq",(00-0102030405),(0,1)
+QCFG: "servicedomain",(0,1,2),(0,1)
+QCFG: "roamservice",(1,2,255),(0,1)
+QCFG: "band",(0-200),(0-7FFFFFFFFFFFFFFF),(0-7FFFFFFFFFFFFFFF),(0,1)
+QCFG: "rrc",(0-4)
+QCFG: "cops_no_mode_change",(0-1)
+QCFG: "urc_cause_support",(0-31)
+QCFG: "msc",(0-2)
+QCFG: "sgsn",(0-2)
+QCFG: "hsdpacat",(6,8,10-24)
+QCFG: "hsupacat",(5,6)
+QCFG: "pdp/duplicatechk",(0,1)
+QCFG: "tdscsq",(0,1)
+QCFG: "airplanecontrol",(0,2)
+QCFG: "ledmode",(0-2)
+QCFG: "ehrpd",(0,7)
+QCFG: "usbid",<vid>,<pid>
+QCFG: "usbee",<enable>
+QCFG: "usbnet",<0-3>
+QCFG: "usbcfg",<vid>,<pid>,<diag>,<nmea>,<at_port>,<modem>,<rmnet>,<adb>,<uac>
+QCFG: "spi/set",(0-2)
+QCFG: "urc/ri/ring",("off","pulse","always","auto","wave"),(1-2000),(1-10000),(1-10000),("off","on"),(1-5)
+QCFG: "urc/ri/smsincoming",("off","pulse","always"),(1-2000),(1-5)
+QCFG: "urc/ri/other",("off","pulse"),(1-2000),(1-5)
+QCFG: "risignaltype",("respective","physical")
+QCFG: "urc/delay",(0,1)
+QCFG: "agps/string",(0,1)
+QCFG: "cdmaruim",(0,1)
+QCFG: "cmux/urcport",(0-4)
+QCFG: "ModemRstLevel",(0,1)
+QCFG: "ApRstLevel",(0,1)
+QCFG: "ltectcc/smsstorage"[,(0,1)]
+QCFG: "noauthcheck",(0,1)
+QCFG: "nwscanmodeex",(1-63)
+QCFG: "oostimer",<timer1>,<timer2>,<timer3>
+QCFG: "diversity",(0-1)
+QCFG: "ppp/termframe",(0,1)
+QCFG: "ppp/v4v6",(0,1)
+QCFG: "nwoptmz/acq",(0,1),(60-16777200)
+QCFG: "ims",(0-2)
+QCFG: "pcmclk",(0,1)
+QCFG: "tone/incoming",(0,1)
+QCFG: "sim/recovery",(3-300),(0,5-300),(0,300)
+QCFG: "rssi",(0-20)
+QCFG: "cdmasms/cmtformat"[,(0,1)]
+QCFG: "ltesms/format"[,(0,1)]
+QCFG: "amrcodec",(0-255)
+QCFG: "apready",(0,1),(0,1),(100-3000)
+QCFG: "sleepind/level",(0,1)
+QCFG: "wakeupin/level",(0,1),(0,1)
+QCFG: "urc/cache",(0,1)
+QCFG: "thermal/modem"[,<level>,<trig>,<clr>]
+QCFG: "thermal/limit_rates"[,<enable>]
+QCFG: "thermal/txpwrlmt"[,<on_off>,<sensor>,<temp_threshold>,<duration>,<trig_cnt>,<crl_cnt>]
+QCFG: "lte/bandprior",(1-43),(1-43),(1-43)
+QCFG: "codec/powsave",(0,1)
+QCFG: "qmisync",(0,1)
+QCFG: "disrplmn",(0,1),(0,1)
+QCFG: "vts/async",(0,1)
+QCFG: "urc/ri/pin",("uart_ri","uart_dcd")
+QCFG: "iproute_enable",(0,1)
+QCFG: "imsreg/iptype",(0,1)
+QCFG: "stkauto/setupmenutr",(0,1)
+QCFG: "urcport/sms",(0,1)
+QCFG: "eps/guti_enable",(0,1)
+QCFG: "qcautoconnect",(0,1)
+QCFG: "usage/apmem"
+QCFG: "usage/apfs"
+QCFG: "ftm/mbim",(0,1)
+QCFG: "volte/disable",(0,1)
+QCFG: "gpsweek",(0,1)
+QCFG: "multi_ip_package",<enable>,<package_max_len>,<package_max_count_in_queue>,<timeout>
+QCFG: "sim/onchip",(0,1)
+QCFG: "siminvalirecovery",(0,1),(1,60),(1,255)
+QCFG: "sleep/datactrl",(0-7),(50-5000),(0,1)
+QCFG: "plmn/autoblock",(0,1),(1-0xFFFFFFFF)
+QCFG: "fast_dormancy",(0-3),(1-65535)
+QCFG: "sim/clk_freq",(0,1)
+QCFG: "sarcfg",("lte_wcdma","gsm","lte","wcdma"),max_power,row_grads,column_grads,[band]
+QCFG: "efratctl",(0,1)
+QCFG: "plmn/addinfbdn",(0,1)
+QCFG: "Feature_Switch_Flag",(0,1),(0x01 -- 0xFFFFFFFF)
+QCFG: "icf",(0-3),(0-3),(0-3)
+QCFG: "dhcppktfltr",(0,1)
+QCFG: "bip/auth",(0-3)
+QCFG: "ntp",(1-10),(5- 60)
+QCFG: "freezeband",(0,1)
+QCFG: "ppp/sleep_ri",(0,1),(500-50000)
+QCFG: "bootup",<name>,(0,1)
+QCFG: "poaccept",(0,1)
+QCFG: "divctl",("lte","wcdma"),(0-2)
+QCFG: "qoos",(0-2),(1-600),(1-43200),(1-600),(1-255),(0-60),(0-60),(0-5),(1-60)
+QCFG: "tcp/windowsize",(0,1),(16-100)
+QCFG: "rrc/control",(0,1),(0-60),(0-60),(0-300),(0,1),(0-60)
+QCFG: "iprulectl",(0,1)
+QCFG: "rf/sar/gpioctl",(0,1),(20~5000)
+QCFG: "usbmode",(0,1)
+QCFG: "lpm/dataind",(0-1),(0-7)
+QCFG: "fast/poweroff",(0,1)
+QCFG: "sms_control",(0,1),(0,1)
+QCFG: "call_control",(0,1)
+QCFG: "lte/preferfre",op,index,band,bandwith,earfcn,mcc,mnc
+QCFG: "voice_busytone",(0,1)
+QCFG: "SMS/ListMsgMap",("REC UNREAD","REC READ","STO UNSENT","STO SENT")
```

### AT+QAUDCFG=?

```plaintext
+QAUDCFG: "alc5616/dlgain",(0-100)
+QAUDCFG: "alc5616/ulgain",(0-100)
+QAUDCFG: "tonevolume",(0-100)
+QAUDCFG: "alc5616/pwrctr",(0-1)
+QAUDCFG: "nau8814/dlgain",(0-100)
+QAUDCFG: "nau8814/aoutput",(0-1)
+QAUDCFG: "encgain",(0-1),(0-65535)
+QAUDCFG: "voltedtmfcfg",(1-400),(1-9999)
+QAUDCFG: "decgain",(0-65535)
+QAUDCFG: "fns",(0),(0-1)
+QAUDCFG: "nau8810/config",(0-255),(0-255),...
```

### AT+QGPSCFG=?

```plaintext
+QGPSCFG: "outport",("none","usbnmea","uartdebug")
+QGPSCFG: "nmeasrc",(0,1)
+QGPSCFG: "gpsnmeatype",(0-31)
+QGPSCFG: "glonassnmeatype",(0-7)
+QGPSCFG: "galileonmeatype",(0,1)
+QGPSCFG: "beidounmeatype",(0-3)
+QGPSCFG: "gsvextnmeatype",(0,1)
+QGPSCFG: "gnssconfig",(0-6)
+QGPSCFG: "odpcontrol",(0-2)
+QGPSCFG: "dpoenable",(0-2)
+QGPSCFG: "plane",(0-2)
+QGPSCFG: "autogps",(0,1)
+QGPSCFG: "suplver",(1,2)
+QGPSCFG: "agpsposmode",(0-4294967295)
+QGPSCFG: "lbsapn",(0-31),(0-4),<apn>
+QGPSCFG: "agnssprotocol",(0-255),(0-65535)
+QGPSCFG: "fixfreq",(1,2,5,10)
+QGPSCFG: "appidname",<id>,<pwd>
+QGPSCFG: "agnssjamming",(0-4),(2-10),(1-65535)
+QGPSCFG: "agnssjammingurcmode",(0-1)
+QGPSCFG: "1pps",(0-1)
+QGPSCFG: "gnssrawdata",(0-31),(0-1)
```

### AT+QWIFICFG=?

```plaintext
+QWIFICFG: "workmode",(0-3)
+QWIFICFG: "powerboot",(0,1)
+QWIFICFG: "ssid",(0-2),<ssid>
+QWIFICFG: "ssidhide",(0,1),(0,1)
+QWIFICFG: "maxsta",(0,1),(1-16)
+QWIFICFG: "channel",(0,1),(0,13,36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184)
+QWIFICFG: "mode",(0,1),(0-5)
+QWIFICFG: "bandwidth",(0,1),<0-2>
+QWIFICFG: "auth",(0-2),(0-4)[,(0-2),<password>]
+QWIFICFG: "macacl",(0,1)(0-3)[,(0-2)][,<mac>]
+QWIFICFG: "cc",<country_code>
+QWIFICFG: "stainfo"
+QWIFICFG: "scan",[(0-1)]
+QWIFICFG: "scanresult"
+QWIFICFG: "stastatus"
+QWIFICFG: "bridgemode",(0,1)
+QWIFICFG: "dongle_mac"
+QWIFICFG: "speedlimit",[(0-100)]
+QWIFICFG: "version"
```

### AT+QFTPCFG=?

```plaintext
+QFTPCFG: "account",<username>,<password>
+QFTPCFG: "filetype",(0,1)
+QFTPCFG: "transmode",(0,1)
+QFTPCFG: "contextid",(1-16)
+QFTPCFG: "rsptimeout",(20-180)
+QFTPCFG: "ssltype",(0-2)
+QFTPCFG: "sslctxid",(0-5)
+QFTPCFG: "data_address",(0-2)
+QFTPCFG: "restenable",(0,1)
```

### AT+QLWCFG=?

```plaintext
+QLWCFG: "security",(0-3),(1-65535),<server_addr>,(0,1),(0,3),<pskID>,<psk_key>
+QLWCFG: "server",(0-3),(1-86400),(1-86400),(1-86400),(1-86400),(0,1),("U","UQ","S","SQ","US","UQS")
+QLWCFG: "epname/mode",(3,6,7)
+QLWCFG: "urc",(0,1)
+QLWCFG: "startup",(0,1)
+QLWCFG: "fota",(0,1),(0,1)
+QLWCFG: "hostdevice",(0,1),<deviceID>,<manufacturer>,<model>,<sw_version>,<fw_version>,<hw_version>,<upgrade_time>
+QLWCFG: "reset"
+QLWCFG: "nettype",(0-2)
+QLWCFG: "maxreconntime",(1-86400),(2-86400),(0-86400)
+QLWCFG: "apnretry",(0,1),(0-16),(0-86400)
```

### AT+QSMTPCFG=?

```plaintext
+QSMTPCFG: "account",<username>,<password>
+QSMTPCFG: "sender",<sender_name>,<sender_email>
+QSMTPCFG: "smtpserver",<srvaddr>,<srvport>
+QSMTPCFG: "contextid",(1-16)
+QSMTPCFG: "sslctxid",(0-5)
+QSMTPCFG: "ssltype",(0-2)
```

### AT+QSSLCFG=?

```plaintext
+QSSLCFG: "sslversion",(0-5),(0-4)
+QSSLCFG: "dtls",(0-5),(0,1)
+QSSLCFG: "dtlsversion",(0-5),(0,1)
+QSSLCFG: "ciphersuite",(0-5),(0X0035,0X002F,0X0005,0X0004,0X000A,0X003D,0XC002,0XC003,0XC004,0XC005, 0XC007,0XC008,0XC009,0XC00A,0XC011,0XC012,0XC013,0XC014,0XC00C,0XC00D,0XC00E,0XC00F,0XC023,0XC024,0XC025,0XC026,0XC027,0XC028,0XC029,0XC02A,0XC02F,0XC030,0XFFFF)
+QSSLCFG: "cacert",(0-5),<cacertpath>
+QSSLCFG: "cacertex",(0-5),<cacertexpath>
+QSSLCFG: "clientcert",(0-5),<client_cert_path>
+QSSLCFG: "clientkey",(0-5),<client_key_path>,<key_pwd>
+QSSLCFG: "seclevel",(0-5),(0-2)
+QSSLCFG: "ignorelocaltime",(0-5),(0,1)
+QSSLCFG: "negotiatetime",(0-5),(10-300)
+QSSLCFG: "sni",(0-5),(0,1)
+QSSLCFG: "psk",(0-5),<identity>,<key>
+QSSLCFG: "closetimemode",(0-5),(0,1)
+QSSLCFG: "session_cache",(0-5),(0,1)
+QSSLCFG: "alpn",(0-5),<protocolname>
+QSSLCFG: "renegotiation",(0-5),(0,1)
+QSSLCFG: "ignoremulticertchainverify",(0-5),(0,1)
+QSSLCFG: "ignoreinvalidcertsign",(0-5),(0,1)
```

### AT+QNWCFG=?

```plaintext
ERROR
```

### AT+QICFG=?

```plaintext
+QICFG: "transpktsize",(1-1460)
+QICFG: "transwaittm",(0-20)
+QICFG: "dataformat",(0,1),(0,1)
+QICFG: "viewmode",(0,1)
+QICFG: "tcp/retranscfg",(3-20),(5-1000)
+QICFG: "dns/cache",(0,1)
+QICFG: "qisend/timeout",(0-3600)
+QICFG: "passiveclosed",(0,1)
+QICFG: "udp/readmode",(0,1)
+QICFG: "udp/sendmode",(0,1)
+QICFG: "tcp/accept",(0,1)
+QICFG: "recvind",(0,1)
+QICFG: "tcp/keepalive",(0,1),(1-120),(25-100),(3-10)
+QICFG: "recv/buffersize",(10240-524288)
+QICFG: "window/scale",(0-1)
```

### AT+QOPSCFG=?

```plaintext
+QOPSCFG: "scancontrol",(0-3),(0-FFFF),(0-7FFFFDF3FFF),(0-3F)
```

### AT+QLBSCFG=?

```plaintext
+QLBSCFG: "asynch",(0,1)
+QLBSCFG: "timeout",(10-120)
+QLBSCFG: "server",<server_name>
+QLBSCFG: "token",<token_value>
+QLBSCFG: "timeupdate",(0,1)
+QLBSCFG: "withtime",(0,1)
+QLBSCFG: "latorder",(0,1)
+QLBSCFG: "contextid",(1-16)
```

### AT+QWIFICFG="version"

```plaintext
+QWIFICFG: "version"
 Goldenbin:QL_FC20_V05
 SW:null
 FW:null
 HW:null
```
