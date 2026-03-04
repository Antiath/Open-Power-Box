//===========================================================================
//========================== Header of the Switch Class =====================
//===========================================================================

/*This class implements all the ASCOM methods and properties of the Ascom switch class plus some extras. It's the one holding all the data.
To make the ASCOM drivers more straighforward, it follows the ASCOM logic about switches. The indi driver must be worked around it. Sorry...
In ASCOM each entity of this class is a "switch". The normal on/off switch is naturally a "switch", but so is a variable analog output (like pwm outputs for dew heaters) and Sensors are also switches.
They are distinguished by their ability to have their state changed.
- ON/OFF switch : can be changed (canwrite==1). Accepts only a boolean (0/1) as input. Can be read (boolean value).
- Analog switch : can be changed (canwrite==1). Accepts any value between minswitchvalue and maxswitchvalue, provided it is a multiple of switchstep. Can be read (analog value)
- Sensor switch : cannot be changed (canwrite==0). Can be read (analog value).
Each sensor has 2 outputs ( one for current measurement, one for voltage), so we make one "sensor switch" par output.

The state of all the switches is stored in the array switches[].
REMARK: An extra toggle will allow you to enable Automatic temperature control. It is passed in the last position of switches[] and only setable if The regul flag is 1 (see config.h)

The order is as follow:
1) ON/OFF switches (also called DC switch hereafter),
2) Analog switches (PWM, but the adjustable voltage output is also an analog switch in the eyes of ASCOM),
3) DC Rail switch 
4) Relay switch
5) Sensors : -3.a) Voltage
             -3.b) Curent
4) Regulation toggle state ( if Regul flag ==1. Otherwise no 4) -----NOT IMPLEMENTED YET


Exemple 1 : if you have 1 DC switch, 1 analog switch and 2 sensors. The list of switches would be :
switches[0] = DC_0 state
switches[1] = Analog_0 sate
switches[2] = Sensor_0 state : voltage
switches[3] = Sensor_0 state : current


Exemple 2 : if you have 3 DC switch, 1 analog switch and 2 sensors. The list of switches would be :
switches[0] = DC_0 state
switches[1] = DC_1 state
switches[2] = DC_2 state
switches[3] = Analog_0 sate
switches[4] = Sensor_0 state : voltage
switches[5] = Sensor_0 state : current
switches[6] = Sensor_1 state : voltage
switches[7] = Sensor_1 state : current


Exemple 3 : if you have 3 DC switch, 1 analog switch and 2 sensors but Regul flag =0. The list of switches would be :
switches[0] = DC_0 state
switches[1] = DC_1 state
switches[2] = DC_2 state
switches[3] = Analog_0 sate
switches[4] = Sensor_0 state : voltage
switches[5] = Sensor_0 state : current
switches[6] = Sensor_1 state : voltage
switches[7] = Sensor_1 state : current

*/

#ifndef _SwitchClass_h
#define _SwitchClass_h

#include <Arduino.h>
#include "config.h"
#include "ErrorManager.h"

#include <INA219.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_DS3502.h>
#include "Adafruit_SHT31.h"

class Switch
{

public:
  Switch();
  Switch(int a);
  int maxswitch();
  bool canwrite(int switchNum);
  bool getswitch(int switchNum);
  String getswitchdescription(int switchNum);
  String getswitchname(int switchNum);
  float getswitchvalue(int switchNum);
  int getswitchtype(int switchNum);
  int minswitchvalue(int switchNum);
  int maxswitchvalue(int switchNum);
  void setswitch(int switchNum, bool state);
  void setswitchname(int switchNum, String name);
  void setswitchvalue(int switchNum, int value);
  int switchstep(int switchNum);
  float TotalCurrentDC;
  float TotalCurrentPWM;
  float TotalCurrent;
  float InputVoltage;
  float TempProbe();
  void TempEnv();
  void HumiEnv();
  float DewPoint();
  void set_Pin(int x, bool y);
  void getAllSensors();
  int index_Translator(int switchNum);
  int switchtype(int switchNum);
  void setreverse(int index, bool value);
  bool getreverse(int index);
  void setlimit(int index, float value);
  float getlimit(int index);
  void setpwm(int index, int value);

  float DClimit, Onlimit, PWMlimit, TotalDClimit, TotalPWMlimit, Totallimit;
  int ReverseDC, ReversePWM, ReverseRelay, ReverseOn, ReverseUSB;
  float Temperature,Humidity;  
  
  String nameswitches[DCOutput_Num + RelayOutput_Num + OnOutput_Num + 2*PWMOutput_Num + USBOutput_Num];

  // flags to pass to the Client to signal that a limit has been reached
  bool DClimitflag[DCOutput_Num];
  bool PWMlimitflag[PWMOutput_Num];
  bool Relaylimitflag;
  bool Onlimitflag;
  bool TotalDClimitflag;
  bool TotalPWMlimitflag;
  bool TotalCurrentflag;

  bool Visible[TotalOutputNum]; // This is used to hide the switches that are not used in the ASCOM driver. It is set in the config.h file.

private:
  bool regul;
  int switchDC[DCOutput_Num];
  int switchPWM[PWMOutput_Num];
  int switchRen[PWMOutput_Num];
  int switchRelay;
  int switchOn;
  int switchUSB[USBOutput_Num];
  
  
  uint8_t Sensors_addr[Sensor_Num];
  float Sensor[2 * Sensor_Num+4];
  bool sensortype[2*Sensor_Num]; 

};

#endif