//-----------------------------------------------------------------------
//OPBXXL_ESP32 Definitions
// // Copyright (c) 2025 F. Mispelaer
//-----------------------------------------------------------------------
#ifndef _Definitions_h
#define _Definitions_h

#include <Arduino.h>
// ASCOM CONST VARS
#define ASCOMGUID "698f6e8c-652d-4e92-bf19-e24a9e29c3aa"
#define ASCOMMAXIMUMARGS 10
#define ASCOMNOTIMPLEMENTED 0x400

// ASCOM MESSAGES
#define ASCOMDESCRIPTION "\"Alpaca interface for the project Open Power Box XXL. This is a 12V power box with 7 DC switches, 3 PWM outputs, 1 bank of 4 DC ouputs for stuff that don't need control, 1 output with variable voltage.\""
#define ASCOMDRIVERINFO "\"Open Power Box ALPACA SERVER (c) F. Mispelaer. 2025\""
#define ASCOMMANAGEMENTINFO "{\"ServerName\":\"OPBXXL\",\"Manufacturer\":\"F. Mispelaer\",\"ManufacturerVersion\":\"v1.0\",\"Location\":\"Nobody_cares\"}"
#define ASCOMNAME "\"OPBXXL_Server\""
#define ASCOMSERVERNOTFOUNDSTR "<html><head><title>ASCOM ALPACA Server</title></head><body><p>File system not started</p><p><a href=\"/setup/v1/focuser/0/setup\">Setup page</a></p></body></html>"
#define T_NOTIMPLEMENTED "not implemented"

// PORTS
#define ASCOMSERVERPORT 4040      // ASCOM Remote port
#define ASCOMDISCOVERYPORT 32227  // UDP

//EEPROM
#define EEPROM_SIZE 1000
#define EEPROM_FLAG_ADD 6 //: Flag for first EEPROM initialization. the setup() will write factory default values to the EEPROM unless this flag is set to 1. So at upload those values are written and the flag set to 1 so that subsequent startup of the board will ignore this part and won't overwrite uneceserraily.
#define EEPROM_SSID_ADD 7 //: Wifi SSID - 20 character strings
#define EEPROM_PWD_ADD 27 //: Wifi PWD - 20 character strings
#define EEPROM_RDC_ADD 47 //: ReverseDC - byte
#define EEPROM_RREL_ADD 48 //: ReverseRelay - byte
#define EEPROM_RON_ADD 49 //: ReverseOn - byte
#define EEPROM_RPWM_ADD 50 //: ReversePWM - byte
#define EEPROM_RUSB_ADD 51 //: ReversePWM - byte
#define EEPROM_LDC_ADD 52 //: DClimit - 4 character string
#define EEPROM_LREL_ADD 72 //: ADJlimit - 4 character string
#define EEPROM_LON_ADD 92 //: Onlimit - 4 character string
#define EEPROM_LPWM_ADD 112 //: PWMlimit - 4 character string
#define EEPROM_LTDC_ADD 132 //: TotalDClimit - 4 character string
#define EEPROM_LTPWM_ADD 152 //: TotalPWMlimit - 4 character string
#define EEPROM_LT_ADD 172 //: Totallimit - 4 character string
#define EEPROM_VIS_ADD 192 //: Visibility- 100 bytes to be sure but only 1 byte per switch is used. 
#define EEPROM_NAME_ADD 292 //: Name1 - 20 character strings
//78 : Name2 (20 char max)
//98 : Name3 (20 char max)

// SERIAL PORT
#define SERIALPORTSPEED 115200  // 9600, 14400, 19200, 28800, 38400, 57600, 115200

// defines for ASCOMSERVER, WEBSERVER
#define NORMALWEBPAGE 200
//#define FILEUPLOADSUCCESS 300
#define BADREQUESTWEBPAGE 400
#define NOTFOUNDWEBPAGE 404
#define INTERNALSERVERERROR 500

#define RAINPIN 26

extern const char* program_version;

extern double timesincelastupdatelist[13];
extern String webpage1,webpage2;

extern const char* JSONPAGETYPE;
extern String ssid1, pwd1;
extern String SwitchErrorMessage;

extern char _packetBuffer[255];
extern unsigned int _ASCOMClientID ;
extern unsigned int _ASCOMClientTransactionID ;
extern unsigned int _ASCOMServerTransactionID ;
extern int _ASCOMErrorNumber ;
extern String _ASCOMErrorMessage;
extern byte _ASCOMConnectedState ;

extern int ASCOM_Mode; // 0 = Compact ( Vin, Totl curr, Total curr DC, Total curr PWM,), 1 = Full (all switches, sensors, etc.)
#endif