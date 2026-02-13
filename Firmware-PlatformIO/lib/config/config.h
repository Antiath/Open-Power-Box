#ifndef _config_h
#define _config_h

#include <Arduino.h>

//===========================================================================
//============================ CONFIGURATION FILE ===========================
// ======== Go over every part in the file to configure your device =========
//===========================================================================

//===========================================================================
//======================== Communication Bus Options ========================
//===========================================================================

/*By default USB and Uart are used as the communication carrier with the client.
Wifi and Ethernet will also be supported in the future.
If you need any of them, uncomment the corresponding enableflag below.
If your hardware supports multiple of them, you can enable more than one.*/

// #define WiFi_enableFlag
// #define Ethernet_enableFlag

//===========================================================================
//=================== Number of outputs and sensors Options =================
//===========================================================================

/*Self explanatory. Define here the number of 12V outputs, the number of PWM outputs and the number of ina219 current sensors.
Be aware of the maximum for each one. If you specify more, the firmware will ignore those entries and use the maximum allowed*/

const short DCOutput_Num = 7;  // Number of stable 12V outputs. 
const short PWMOutput_Num = 3; // Number of PWM controlled outputs.
const short RelayOutput_Num = 1; // Number of relay outputs.
const short OnOutput_Num = 1; // Number of always ON 12V switches (the whole bank of connectors is controlled by the same switch)
const short USBOutput_Num = 0; // Number of USB controlled outputs. Maximum is 7.

const short Sensor_Num = DCOutput_Num + OnOutput_Num + PWMOutput_Num; 
const short TotalOutputNum = DCOutput_Num + RelayOutput_Num + OnOutput_Num + PWMOutput_Num + USBOutput_Num + 2 * Sensor_Num +4;
const int totalswitches= DCOutput_Num + PWMOutput_Num + RelayOutput_Num + OnOutput_Num+USBOutput_Num;

const int SensorPos= DCOutput_Num + PWMOutput_Num + RelayOutput_Num + OnOutput_Num + USBOutput_Num; // Position of the first sensor in the switch array
const int sensorDC0 = SensorPos+4; // add 4 to ignore the input voltage and total currents
const int sensorPWM0 = sensorDC0 + DCOutput_Num * 2;
const int sensorOn0 = sensorPWM0 + PWMOutput_Num * 2;
//===========================================================================
//============================= Pin configguration ==========================
//===========================================================================

/*Specify the pin numbers of Stable outputs or leave default value.
N.B.: the firmware will ignore the outputs past <StableOutput_Num>. For exemple, if StableOutput_Num=3, check the first 3 outputs and number 4 to 10 will be ignored.
You only need to check the outputs from n°1 to <StableOutput_Num>.

N.B2: There may be not enough pins on the esp32. This firmware has an option to use the GPIO expander MCP23017 in order to add more control pins.
To identify them, replace the normal pinnumber with 10x, depending on the number of the I/O on the chip.
The firmware will automatically recognise the extra number as being an I/O of the expander. By default the last two on the list below ar A0 and A1.*/

/*Pin Name 	Pin ID
GPA0 	0 -> 100
GPA1 	1 -> 101
GPA2 	2 -> 102
GPA3 	3 -> 103
GPA4 	4 -> 104
GPA5 	5 -> 105
GPA6 	6 -> 106
GPA7 	7 -> 107
GPB0 	8 -> 108
GPB1 	9 -> 109
GPB2 	10 -> 110
GPB3 	11 -> 111
GPB4 	12 -> 112
GPB5 	13 -> 113
GPB6 	14 -> 114
GPB7 	15 -> 115
*/

const short DCOutput_Pin[DCOutput_Num] =
    {
        32,
        33,
        25,
        26,
        27,
        14,
        13,
};
const short RelayOutput_Pin = 12;
const short OnOutput_Pin = 108;

/*Specify the pin numbers of PWM outputs or leave default value.
N.B.: the firmware will ignore the outputs past <PWMOutput_Num>. For exemple, if PWMOutput_Num=1, check the first output and number 2 to 3 will be ignored.
You only need to check the outputs from n°1 to <PWMOutput_Num>. */
const short PWMOutput_Pin[PWMOutput_Num] =
    {
        15,
        2,
        4};

const short USBOutput_Pin[13] =
    {
        100,
        101,
        102,
        103,
        104,
        105,
        106,
};        

/*Configure the logic of your power transistors.
Reverse = 0, Output is ON when Signal is ON
Reverse = 1, Output is ON when Signal is OFF
*/



/*Set the description text*/
#define DC_description "On/Off DC Output"
#define PWM_description "Dew Heater 0-100%"
#define Relay_description "Relay output"
#define On_description "DC Rail Output"
#define Current_Sensor_description "Current (A)"
#define Voltage_Sensor_description "Voltage (V)"
#define HeaterEn_description "Enable/Disable Automatic temperature control"
#define USB_description "USB Output"

//===========================================================================
//======================== I²C addresses configguration =====================
//===========================================================================

/*Look up the datasheet of the ina219 current sensor chip. Section "Serial Bus Address".
By default, we use 14 adrdresses of the table in the datasheet.
The firmware supports BME280 with adress 0x76 or 0x76.So these two are reserved and should not be used to avoid conflicts..
Make sure to either configure  you sensors according those addresses, or to change them appropriatly.
Each sensor will be attributed those adresses in this order exactly, from first (addr1) to last (addr14).
If for a reason or another, you need spicific adresses, you must put them in the first places of the list.
If building a new pcb, I would suggest to leave this list as is and wire your sensors according to it.
N.B.: the firmware will ignore the adresses past <Sensor_Num>. For exemple, if Sensor_Num=7, check the first 7 adresses and number 8 to 14 will be ignored.
You only need to check the adresses from n°1 to <Sensor_Num>. */

const short MeasureInterval = 5000; // Interval in milliseconds at wich the sensors are read.

const short sensorDC[DCOutput_Num] =
    {
        0x41,
        0x46,
        0x45,
        0x49,
        0x43,
        0x42,
        0x47};
const short sensorPWM[PWMOutput_Num] =
    {
        0x4B,
        0x4A,
        0x4C};
//const short sensorADJ = 0x40;
const short sensorOn = 0x48;

//  0x40 //1000000     GND GND
//  0x41 //1000001     GND VS+
//  0x42 //1000010     GND SDA
//  0x43 //1000011     GND SCL
//  0x44 //1000100     VS+ GND
//  0x45 //1000101     VS+ VS+
//  0x46 //1000110     VS+ SDA
//  0x47 //1000111     VS+ SCL
//  0x48 //1001000     SDA GND
//  0x49 //1001001    SDA VS+
//  0x4A //1001010    SDA SDA
//  0x4B //1001011    SDA SCL
//  0x4C //1001100    SCL GND
//  0x4D //1001101    SCL VS+

//===========================================================================
//================ Automatic temperature control - UNFINISHED================
//===========================================================================

// The tempertature probe and evironmental sensor helps you regulate your dew heaters and consume as little power as possible.
//  To do this, you need a temperature probe in a dew heater + a temperature and humidity sensor elswhere. Both are mandatory.
//  This firmware will only supports a 10K thermistor as the dew heater probe and a SHT31 as the ambiant sensor. Future versions may also support other ambiant sensors like BME280.

// The following flag enables temperature control if set to 1. If you don't need any of this, just leave it at 0.
//  N.B. This only affects what is showed to the client. If set to 1, you still have to toggle the option on the client
#define Ren 0

// Option flag to define the type of probe you use.
// Comment out the one you don't require and vice versa. You have to chose one or the other.
// DO NOT uncomment or comment both of them.
#define ProbeType "Thermistor"

// Pin definition
#define Thermistor_pin 35 // Input pin of the thermistor. The other pin of the thermistor should be  hooked to +3.3V.

// Regulation parameters
#define Kp 10 // Regulation is done via a Proportionnal scheme where PWM Output value= Kp * ( Tprobe - setpoint). Adjust the P factor to tune temperature regulation.

// Thermistor parameters. Avoid changing these if you don't know the characteristics of your thermistor.. We assume that the fixed resistor is 10K.
//(Default values taken from the datasheet of Epcos B57703M0103G040)
#define Bcoef 3988
#define Rnom 10000
#define Tnom 25

//===========================================================================
//============================ END OF CONFIGURATION==========================
// ============ DO NOT CHANGE ANYTHING BELOW THIS LINE !!! ==================
//===========================================================================


#endif
