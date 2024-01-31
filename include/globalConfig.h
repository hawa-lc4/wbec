// Copyright (c) 2023 steff393, MIT license
/*
ACHTUNG! src/globalConfig.cpp nicht vergessen!
*/

#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#define WB_HOSTNAME   "wall-E"
#define WB_CNT              2   // max. possible number of wallboxes in the system (NodeMCU has Bus-ID = 0)
#define OPENWB_MAX_LP       2   // maximum supported loadpoints by openWB
#define REG_WD_TIME_OUT   257   // modbus register for "ModBus-Master Watchdog Timeout in ms"
#define REG_STANDBY_CTRL  258   // modbus register for "Standby Function Control"
#define REG_REMOTE_LOCK   259   // modbus register for "Remote lock (only if extern lock unlocked)"
#define REG_CURR_LIMIT    261   // modbus register for "Maximal current command"
#define REG_CURR_LIMIT_FS 262   // modbus register for "FailSafe Current configuration (in case loss of Modbus communication)"

#define CURR_ABS_MIN       60   // absolute possible lower limit for current
#define CURR_ABS_MAX      160   // absolute possible upper limit for current

#if WALLE_VERSION_MAJOR == 1
  #define PIN_DI              1   // GPIO1, NodeMCU UART0 pin TX, used by ModBus
  #define PIN_RO              3   // GPIO3, NodeMCU UART0 pin RX, used by ModBus
  #define PIN_DE_RE          -1   // not used; ModBus TxEnable
  // #define PIN_RST             0   // GPIO0, NodeMCU pin D3 
  // #define PIN_PV_SWITCH      13   // GPIO0, NodeMCU pin D7 
  // #define PIN_SS             15   // GPIO15,NodeMCU pin D8
#endif
#if WALLE_VERSION_MAJOR == 2
  #define PIN_DI             14   // GPIO14, NodeMCU pin D5, used by ModBus Tx + Duo-LED rt
  #define PIN_RO             12   // GPIO12, NodeMCU pin D6, used by ModBus Rx + Duo-LED gn
  #define PIN_DE_RE          -1   // not used; ModBus TxEnable
  // #define PIN_RST             0   // GPIO0, NodeMCU pin D3
  // #define PIN_PV_SWITCH      13   // GPIO13, NodeMCU pin D7
  #define PIN_XXXX            2   // GPIO2, NodeMCU pin D4, LED rt (+ interne LED bl)
  #define PIN_PHASECTRL      15   // GPIO15,NodeMCU pin D8, Phasenumschaltrelais direkt + LED ge
#endif

extern char     cfgWbecVersion[];	           // wbec version
extern char     cfgBuildDate[];	          	 // wbec build date
 
extern char     cfgOTAUsername[16];          // OTA user name for this project
extern char     cfgOTAPasswd[16];            // OTA password for this project
extern char     cfgApSsid[16];               // SSID of the initial Access Point
extern char     cfgApPass[32];               // Password of the initial Access Point
extern uint8_t  cfgCntWb;                    // number of connected wallboxes in the system
extern uint8_t  cfgMbCycleTime;              // cycle time of the modbus (in seconds)
extern uint16_t cfgMbDelay;                  // delay time of the modbus before sending new message (in milliseconds)
extern uint16_t cfgMbTimeout;                // Reg. 257: Modbus timeout (in milliseconds)
extern uint16_t cfgStandby;                  // Reg. 258: Standby Function Control: 0 = enable standby, 4 = disable standby
extern uint16_t cfgFailsafeCurrent;          // <don't use - still beta> Reg. 262: Failsafe Current configuration in case of loss of Modbus communication (in 0.1A)
extern char     cfgMqttIp[16];               // IP address of MQTT broker, "" to disable MQTT
extern uint16_t cfgMqttPort;                 // Port of MQTT broker (optional)
extern char     cfgMqttUser[32];             // MQTT: Username
extern char     cfgMqttPass[32];             // MQTT: Password
extern uint8_t  cfgMqttLp[WB_CNT];           // Array with assignments to openWB loadpoints, e.g. [4,2,0,1]: Box0 = LP4, Box1 = LP2, Box2 = no MQTT, Box3 = LP1
extern char     cfgMqttWattTopic[60];        // MQTT: Topic for setting the watt value for PV charging, default: "wbec/pv/setWatt"
extern char     cfgMqttWattJson[30];         // MQTT: Optional: Element in a JSON string, which contains the power in watt, default: ""
extern char     cfgNtpServer[30];            // NTP server
extern char     cfgFoxUser[32];              // powerfox: Username
extern char     cfgFoxPass[16];              // powerfox: Password
extern char     cfgFoxDevId[16];             // powerfox: DeviceId
extern uint8_t  cfgPvActive;                 // PV charging: Active (1) or inactive (0)
extern uint8_t  cfgPvCycleTime;              // PV charging: cycle time (in seconds)
extern uint8_t  cfgPvLimStart;               // PV charging: Target current needed for starting (in 0.1A), e.g. 61=6.1A
extern uint8_t  cfgPvLimStop;                // PV charging: Target current to stop charging when below (in 0.1A)
extern uint8_t  cfgPvPhFactor;               // PV charging: Power/Current factor, e.g. 69: 1A equals 690W at 3phases, 23: 1A equals 230W at 1phase
extern uint16_t cfgPvOffset;                 // PV charging: Offset for the available power calculation (in W); can be used to assure that no/less current is consumed from net
extern uint8_t  cfgPvInvert;                 // PV charging: Invert the watt value (pos./neg.)
extern uint8_t  cfgPvMinTime;                // PV charging: Minimum activation time (in minutes), 0 to disable
extern uint16_t cfgTotalCurrMax;             // Total current limit for load management (in 0.1A) - !! Additional fuse mandatory !!
extern uint8_t  cfgHwVersion;                // Selection of the used HW
extern uint8_t  cfgWifiSleepMode;            // Set sleep type for power saving, recomendation is 255 (=no influence) or 0 (=WIFI_NONE_SLEEP)
extern uint8_t  cfgLoopDelay;                // Delay [ms] at end of main loop, might have an impact on web server reactivitiy, default: 255 = inactive
extern uint16_t cfgKnockOutTimer;            // Interval[min] after which wbec knocks itself out, i.e. triggers a reset, default: 0 = inactive; values < 20min not allowed
extern char     cfgShellyIp[16];             // IP address of Shelly 3em, "" to disable 
extern char     cfgInverterIp[16];           // IP address of Inverter, "" to disable 
extern uint8_t  cfgInverterType;             // 0=off, 1=SolarEdge, 2=Fronius, 3=Kostal
extern uint16_t cfgInverterPort;             // Overwrite default inverter port setting
extern uint16_t cfgInverterAddr;             // Overwrite default inverter address setting
extern uint16_t cfgInvSmartAddr;             // Overwrite default smart meter address setting
extern uint16_t cfgBootlogSize;              // Size of the bootlog buffer for debugging, e.g. 5000 bytes
extern uint16_t cfgBtnDebounce;              // Debounce time for button [ms]
extern uint16_t cfgWifiConnectTimeout;       // Timeout in seconds to connect to Wifi before change to AP-Mode
extern uint8_t  cfgResetOnTimeout;           // Set (some) Modbus values to 0 after 10x message timeout


extern void loadConfig();

#endif	/* GLOBALCONFIG_H */
