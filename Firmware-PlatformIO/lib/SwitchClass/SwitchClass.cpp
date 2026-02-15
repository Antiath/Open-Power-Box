#include "SwitchClass.h"
#include <Arduino.h>
#include <vector>
#include "config.h"
#include "EEPROMManager.h"
#include <EEPROM.h>
#include "Definitions.h"

// Constructor
std::vector<INA219> sensors;
Adafruit_MCP23X17 mcp;
Adafruit_DS3502 ds3502 = Adafruit_DS3502();
Adafruit_SHT31 sht31 = Adafruit_SHT31();

Switch::Switch()
{
}

Switch::Switch(int a)
{
  //Initialize Error flags to false
  for(int i=0; i<30;i++)ASCOM_err_flag[i]=false;
  // Setting EEPROM adresses for the names
  for (int i = 0; i < DCOutput_Num; i++)
  {
    addrnames[i] = EEPROM_NAME_ADD + i * 20; // DC
  }
  for (int i = 0; i < PWMOutput_Num; i++)
  {
    addrnames[DCOutput_Num +  i] = EEPROM_NAME_ADD + (DCOutput_Num + i) * 20; // PWM
  }
  
  for (int i = 0; i < PWMOutput_Num; i++)
  {
    addrnames[DCOutput_Num +PWMOutput_Num+ i] = EEPROM_NAME_ADD + (DCOutput_Num+PWMOutput_Num + i) * 20; // Ren
  }
  
  addrnames[DCOutput_Num+2*PWMOutput_Num] = EEPROM_NAME_ADD + (DCOutput_Num + 2*PWMOutput_Num) * 20; // On
  
  addrnames[DCOutput_Num +2*PWMOutput_Num + OnOutput_Num] = EEPROM_NAME_ADD + (DCOutput_Num + 2*PWMOutput_Num+ OnOutput_Num) * 20; // Relay
  
  for (int i = 0; i < USBOutput_Num; i++)
  {
    addrnames[DCOutput_Num +2* PWMOutput_Num + OnOutput_Num+1 +  i] = EEPROM_NAME_ADD + (DCOutput_Num + 2*PWMOutput_Num+ OnOutput_Num+1+ i) * 20; // USB
  }

  
  // Initialize EEPROM iF first upload. Won't be executed after the first time.
  Serial.println(readStoredbyte(EEPROM_FLAG_ADD));
  if (readStoredbyte(EEPROM_FLAG_ADD) != 1)
  {
    Serial.println("Initializing EEPROM");
    EEPROM.put(EEPROM_FLAG_ADD, byte(1));
    StoreString(EEPROM_SSID_ADD, "");
    StoreString(EEPROM_PWD_ADD, "");
    EEPROM.put(EEPROM_RDC_ADD, 1);
    EEPROM.put(EEPROM_RREL_ADD, 1);
    EEPROM.put(EEPROM_RON_ADD, 1);
    EEPROM.put(EEPROM_RPWM_ADD, 0);
    EEPROM.put(EEPROM_RUSB_ADD, 1);
    StoreString(EEPROM_LDC_ADD, "6.00");
    StoreString(EEPROM_LON_ADD, "5.00");
    StoreString(EEPROM_LPWM_ADD, "2.00");
    StoreString(EEPROM_LTDC_ADD, "10.0");
    StoreString(EEPROM_LTPWM_ADD, "6.00");
    StoreString(EEPROM_LT_ADD, "20.0");

    for(int i = 0; i < totalswitches+4; i++)EEPROM.put(EEPROM_VIS_ADD+i, 1);
    for(int i = totalswitches+4; i < TotalOutputNum; i++)EEPROM.put(EEPROM_VIS_ADD+i, 0);
    String n;

    for (int i = 0; i < DCOutput_Num; i++)
    {
      n = "DC_Switch_";
      n = n + String(i + 1);
      StoreString(addrnames[i], n);
    }

    for (int i = 0; i < PWMOutput_Num; i++)
    {
      n = "PWM_Switch_";
      n = n + String(i + 1);
      StoreString(addrnames[DCOutput_Num + i], n);
    }
    for (int i = 0; i < PWMOutput_Num; i++)
    {
      n = "Auto_Dew_";
      n = n + String(i + 1);
      StoreString(addrnames[DCOutput_Num+ PWMOutput_Num + i], n);
    }
    
    StoreString(addrnames[DCOutput_Num +2* PWMOutput_Num], "DC_Rail_Switch");
    StoreString(addrnames[DCOutput_Num +2* PWMOutput_Num + OnOutput_Num], "Relay_Switch");

    if(USBOutput_Num>0){
      for (int i = 0; i < USBOutput_Num; i++)
      {
        n = "USB_";
        n = n + String(i + 1);
        StoreString(addrnames[DCOutput_Num +2* PWMOutput_Num + OnOutput_Num+ i+1], n);
      }
    }
    EEPROM.commit();
 }

  // Reading All EEPROM parameters
  Serial.println("Reading EEPROM");
  ssid1 = readStoredString(EEPROM_SSID_ADD);
  pwd1 = readStoredString(EEPROM_PWD_ADD);
  ReverseDC = readStoredbyte(EEPROM_RDC_ADD);
  ReverseRelay = readStoredbyte(EEPROM_RREL_ADD);
  ReverseOn = readStoredbyte(EEPROM_RON_ADD);
  ReversePWM = readStoredbyte(EEPROM_RPWM_ADD);
  ReverseUSB = readStoredbyte(EEPROM_RUSB_ADD);
  DClimit = readStoredString(EEPROM_LDC_ADD).toFloat();
  Onlimit = readStoredString(EEPROM_LON_ADD).toFloat();
  PWMlimit = readStoredString(EEPROM_LPWM_ADD).toFloat();
  TotalDClimit = readStoredString(EEPROM_LTDC_ADD).toFloat();
  TotalPWMlimit = readStoredString(EEPROM_LTPWM_ADD).toFloat();
  Totallimit = readStoredString(EEPROM_LT_ADD).toFloat();

  for(int i = 0; i < TotalOutputNum-3*(short)Ren; i++)Visible[i]=readStoredbyte(EEPROM_VIS_ADD+i);
  
if(Ren){
    Visible[TotalOutputNum-3]=true;
    Visible[TotalOutputNum-2]=true;
    Visible[TotalOutputNum-1]=true;
}
  for (int i = 0; i < DCOutput_Num; i++)
  {
    nameswitches[i] = readStoredString(addrnames[i]);
  }
  for (int i = 0; i < PWMOutput_Num; i++)
  {
    nameswitches[DCOutput_Num + i] = readStoredString(addrnames[DCOutput_Num + i]);
  }
    for (int i = 0; i < PWMOutput_Num; i++)
  {
    nameswitches[DCOutput_Num + PWMOutput_Num + i] = readStoredString(addrnames[DCOutput_Num + PWMOutput_Num + i]);
  }
  
  nameswitches[DCOutput_Num + 2*PWMOutput_Num] = readStoredString(addrnames[DCOutput_Num + 2*PWMOutput_Num]);
  nameswitches[DCOutput_Num + 2*PWMOutput_Num + OnOutput_Num] = readStoredString(addrnames[DCOutput_Num + 2*PWMOutput_Num + OnOutput_Num]);
  for (int i = 0; i <USBOutput_Num; i++)
  {
    nameswitches[DCOutput_Num + 2*PWMOutput_Num + OnOutput_Num+i+1] = readStoredString(addrnames[DCOutput_Num + 2*PWMOutput_Num + OnOutput_Num+i+1]);
  }

  // Setting up I2C
  Wire.begin();

  if (!mcp.begin_I2C())
  {
    // if (!mcp.begin_SPI(CS_PIN)) {
    Serial.println("Error.");
    while (1)
      ;
  }

    // Temperature control check
  if (Ren) {
      if (!sht31.begin(0x44))
  {
    // if (!mcp.begin_SPI(CS_PIN)) {
    Serial.println("Error setting up SHT31 sensor.");
    while (1)
      ;
  }
  }
  

  int j = 0;
  for (int i = 0; i < DCOutput_Num; i++)
  {
    DClimitflag[i] = 0;
    Sensors_addr[j] = sensorDC[i];
    j++;
  }
  for (int i = 0; i < PWMOutput_Num; i++)
  {
    PWMlimitflag[i] = 0;
    Sensors_addr[j] = sensorPWM[i];
    j++;
  }
  Sensors_addr[j] = sensorOn;

  if (Sensor_Num > 0)
  {
    for (int i = 0; i < Sensor_Num; i++)
    {
      INA219 sens(Sensors_addr[i]);
      sensors.push_back(sens);
    }

    for (int i = 0; i < Sensor_Num; i++)
    {
      if (!sensors[i].begin())
      {
        Serial.println("Could not connect. Fix and Reboot");
      }
      sensors[i].setMaxCurrentShunt(30, 0.001);
      delay(100);
    }
  }

  for (int i = 0; i < DCOutput_Num; i++)
  {
    pinMode(DCOutput_Pin[i], OUTPUT);
  }
  pinMode(RelayOutput_Pin, OUTPUT);
  mcp.pinMode(OnOutput_Pin - 100, OUTPUT);
  for (int i = 0; i < PWMOutput_Num; i++)
  {
    pinMode(PWMOutput_Pin[i], OUTPUT);
    ledcAttach(PWMOutput_Pin[i], 10, 8); 
    ledcWrite(i, 0);
  }
  for (int i = 0; i < USBOutput_Num; i++)
  {
   mcp.pinMode(USBOutput_Pin[i]-100, OUTPUT);
  }

  for (int i = 0; i < DCOutput_Num; i++)
  {
    switchDC[i] = 0;
    setswitch(i, 0);
  }
  switchRelay = 0;
  switchOn = 1;

  for (int i = 0; i < PWMOutput_Num; i++)
  {
    switchPWM[i] = 0;
    switchRen[i]=0;
  }

    for (int i = 0; i < USBOutput_Num; i++)
  {
    switchUSB[i] = 1;
    setswitch(i-DCOutput_Num-2*PWMOutput_Num-OnOutput_Num-RelayOutput_Num, 1);
  }

  //Array to tell if sensor is voltage or current. Not including input voltage and total currents. Only INA sensors.
  sensortype[0] = 0;
  for (int i = 1; i < 2 * Sensor_Num; i++)
    sensortype[i] = !(sensortype[i-1]);

}

int Switch::maxswitch()
{
 // This will only be served to the ALPACA driver to know how many switches to expect.
 // Normal serial drivers get the full array and decide what to do with it on their own

  int out=totalswitches+4; // Default to Compact mode 
  int j=0;
    
    for (int i=0;i<TotalOutputNum;i++)
    {
      if (Visible[i] == true)j++;
    }
    out = j; 

    return out;
}

bool Switch::canwrite(int switchNum)
{
  int type = switchtype(switchNum);

  if (type == 0)
    return true;
  else if (type == 1)
    return true;
  else if (type == 2)
    return true;
  else if (type == 3)
    return true;
  else if (type == 4)
    return true;
    else if (type == 5)
  {
      return false;
  }
    else if (type == 6)
  {
      return false;
  }
  else
    return false;
}

bool Switch::getswitch(int switchNum)
{
  int type = switchtype(switchNum);
  bool a = 0;

  switch (type)
  {
  case 0:
    a = bool(switchDC[switchNum]);
    break;

    case 1:
      if(switchNum-DCOutput_Num < PWMOutput_Num)
      {
            if (switchPWM[switchNum - DCOutput_Num ] == 0)a = false;
            else a = true;
        break;
      }
      else a = bool(switchRen[switchNum - DCOutput_Num-PWMOutput_Num]); 

    case 2:
    a = bool(switchOn);
    break;

    case 3:
    a = bool(switchRelay);
    break;

    case 4:
    a = bool(switchUSB[switchNum-DCOutput_Num-2*PWMOutput_Num-OnOutput_Num-RelayOutput_Num]);
    break;

    case 5:
    a = false;
    break;

    case 6:
    a = false;
    break;

  }
  return a;
}

String Switch::getswitchdescription(int switchNum)
{
  int type = switchtype(switchNum);
  if (type == 0)
    return DC_description;
  else if (type == 1)
  {
     if (switchNum-DCOutput_Num < PWMOutput_Num)    return PWM_description;
     else return Ren_description; 
  }

  else if (type == 2)
    return On_description;
  else if (type == 3)
    return Relay_description;
  else if (type == 4)
    return USB_description;
    else if (type == 5)
    return " ";
  else if (type == 6)
    return HeaterEn_description;
  else
    return "Unknown";
}

String Switch::getswitchname(int switchNum)
{
  String n;
  int type = switchtype(switchNum);
  if(type==6)
  {
    if(switchNum==TotalOutputNum-3) return "Temperature (C)";
    else if(switchNum==TotalOutputNum-2) return "Humidity (%)";
    else if(switchNum==TotalOutputNum-1) return "Dew Point (C)";
  }
  else  if (type<=4)
    return nameswitches[switchNum];
  else if (switchNum == totalswitches)
    return "Input Voltage";
  else if (switchNum == totalswitches+1)
    return "Total Current";
  else if (switchNum == totalswitches+2)
    return "Total DC Current";
  else if (switchNum == totalswitches+3)
    return "Total PWM Current";
  else
  {
    n = "S-";
    Serial.println(sensortype[switchNum - totalswitches-4]);
    if (sensortype[switchNum - totalswitches-4] == 0)
    {
      n = n + nameswitches[(switchNum - totalswitches-4-1) / 2];
      n = n + "-V";
      return n;
    }
    else
    {
      n = n + nameswitches[(switchNum - totalswitches-4) / 2];
      n = n + "-A";
      return n;
    }
  }
}

int Switch::getswitchtype(int switchNum)
{
  return switchtype(switchNum);
}

float Switch::getswitchvalue(int switchNum)
{
  int type = switchtype(switchNum);

  float a = 0;

  switch (type)
  {
  case 0:
    a = float(switchDC[switchNum]);
    break;
      case 1:
      if(switchNum-DCOutput_Num < PWMOutput_Num){a = switchPWM[switchNum - DCOutput_Num];}
      else a = switchRen[switchNum - DCOutput_Num-PWMOutput_Num]; 
    break;
      case 2:
    a = float(switchOn);
    break;
  case 3:
    a = float(switchRelay);
    break;
  case 4:
    a = switchUSB[switchNum-DCOutput_Num-2*PWMOutput_Num-OnOutput_Num-RelayOutput_Num];
    break;
    case 5:
    a = Sensor[switchNum - totalswitches];
    break;
    case 6:

    if(switchNum==TotalOutputNum-3) a = Temperature;
    else if(switchNum==TotalOutputNum-2)a = Humidity;
    else if(switchNum==TotalOutputNum-1)a = DewPoint();
    break;
  }
  return a;
}

int Switch::minswitchvalue(int switchNum)
{
  int answer = 1000; // nonsense answer by default to avoid errors
  int type = switchtype(switchNum);
  if (type == 0)
    answer = 0; // on/Off
  else if (type == 1)
    answer = 0; 
  else if (type == 2)
    answer = 0; // on/Off
  else if (type == 3)
    answer = 0; // on/Off
  else if (type == 4)
    answer = 0; // on/Off    
  else if (type == 5)
    answer = -50; // sensors
    else if (type == 6)
    answer = -50; // Ren
  return answer;
}

int Switch::maxswitchvalue(int switchNum)
{
  int answer = 1000; // nonsense answer by default to avoid errors
  int type = switchtype(switchNum);

  if (type == 0)
    answer = 1; // on/Off
  else if (type == 1)
    {
      if(switchNum-DCOutput_Num < PWMOutput_Num){answer = 100;}
      else answer = 1; // Ren 0-1 but will be treated as a boolean in the driver. This is to avoid confusion in the ALPACA driver that would expect a range for a regul output.
    } // 
  else if (type == 2)
    answer = 1; // on/Off
  else if (type == 3)
    answer = 1; // on/Off
  else if (type == 4)
    answer = 1; // on/Off
  else if (type == 5)
    answer = 50; // sensors
  else if (type == 6)
    answer = 50; // Ren
  return answer;
}

bool Switch::getreverse(int index)
{
  int Reversearray[5] = {ReverseDC, ReversePWM, ReverseOn, ReverseRelay, ReverseUSB};
  bool answer = 0;
if(index<5) answer = Reversearray[index];
  return answer;
}

void Switch::setreverse(int index, bool value)
{
  if (index == 0)
    ReverseDC = value; 
  else if (index == 1)
    ReversePWM = value; 
  else if (index == 2)
    ReverseOn = value; 
  else if (index == 3)
    ReverseRelay = value; 
  else if (index == 4)
    ReverseUSB = value; 
    return;
}

float Switch::getlimit(int index)
{
    float Limitarray[6] = {DClimit, PWMlimit, Onlimit, TotalDClimit, TotalPWMlimit, Totallimit};
  float answer = 0;
if(index<6) answer = Limitarray[index];
  return answer;
}

void Switch::setlimit(int index, float value)
{
  if (index == 0)
    DClimit = value; 
  else if (index == 1)
    PWMlimit = value;  
  else if (index == 2)
    Onlimit = value; 
  else if (index == 3)
    TotalDClimit = value; 
  else if (index == 4)
    TotalPWMlimit = value;   
  else if (index == 5)
    Totallimit = value;   
  return;
}
void Switch::setswitch(int switchNum, bool state)
{
  bool state1 = state;
  bool state2 = state;
  int type = switchtype(switchNum);
if(InputVoltage<10.0  && state==1) 
{
  SwitchErrorMessage = ASCOM_err[9];
  Serial.println("Undervoltage detected. Cannot Switch ON outputs. Input voltage: "+String(InputVoltage)+"V");
  return;
} 
  switch (type)
  {
  case 0: // DC
    if (ReverseDC)
      state1 = !state;
    set_Pin(DCOutput_Pin[switchNum], state1);
    switchDC[switchNum] = state2;
    break;

  case 1: // PWM
    regul = 0;
    if (ReversePWM)
      state1 = !state;
    if (state2 == 0)
    {
      if(switchNum-DCOutput_Num < PWMOutput_Num)
      {
        setpwm(switchNum - DCOutput_Num, 0);
      //switchPWM[switchNum-DCOutput_Num] = 0;
      //ledcWrite(PWMOutput_Pin[switchNum-DCOutput_Num], 0);
      switchRen[switchNum - DCOutput_Num] = 0;
      }
      else      
      {
        switchRen[switchNum - DCOutput_Num - PWMOutput_Num] = 0;
      }

    }
    else if (state2 == 1)
    {
      if(switchNum-DCOutput_Num < PWMOutput_Num)
      {
      setpwm(switchNum - DCOutput_Num, 100);
      switchRen[switchNum - DCOutput_Num] = 0;
      //switchPWM[switchNum-DCOutput_Num] = 100;
      //ledcWrite(PWMOutput_Pin[switchNum-DCOutput_Num], 255);
      }
      else      
      {
        switchRen[switchNum - DCOutput_Num - PWMOutput_Num] = 1;
      }

    }
    break;

  case 2: // On
    if (ReverseOn)
      state1 = !state;
    set_Pin(OnOutput_Pin, state1);
    switchOn = state2;
    break;

    case 3: // Relay
    if (ReverseRelay)
      state1 = !state;
    set_Pin(RelayOutput_Pin, state1);
    switchRelay = state2;
    break;

    case 4: // USB
    if (ReverseUSB)
      state1 = !state;
    set_Pin(USBOutput_Pin[switchNum-DCOutput_Num-2*PWMOutput_Num-OnOutput_Num-RelayOutput_Num], state1);
    switchUSB[switchNum-DCOutput_Num-2*PWMOutput_Num-OnOutput_Num-RelayOutput_Num] = state2;
    break;

  case 5: // sensor
    SwitchErrorMessage = ASCOM_err[2];
    Serial.println(ASCOM_err[2]);
    break;
  case 6: // Ren
  
    //switchRen = bool(value);
    //regul = bool(value);
   break;
  }
}

void Switch::setswitchname(int switchNum, String name)
{
  if (name.length() <= 20)
  {
    if (switchNum < totalswitches)
    {
      if (name != readStoredString(addrnames[switchNum]))
      {
        nameswitches[switchNum] = name;
        StoreString(addrnames[switchNum], name);
      }
    }
  }
}

void Switch::setswitchvalue(int switchNum, int value)
{
  bool state, state2;
  int type = switchtype(switchNum);
  int maxval = maxswitchvalue(switchNum);
  if(InputVoltage<10.0 && value>0) 
  {
    SwitchErrorMessage = ASCOM_err[9];
    Serial.println("Undervoltage detected. Cannot Switch ON outputs. Input voltage: "+String(InputVoltage)+"V");
    return;
  }; 
  if (type == 0)
  {
    state = value;
    state2 = state;
    if (ReverseDC)
      state = !state;
    if (value <= maxval)
      set_Pin(DCOutput_Pin[switchNum], state);
    switchDC[switchNum] = state2;
  }

  else if (type == 1)
  {
    if (value <= maxval)
    {
      //regul = 0;
      //switchPWM[switchNum - DCOutput_Num ] = 0;
      if(switchNum-DCOutput_Num < PWMOutput_Num)
      {
        setpwm(switchNum - DCOutput_Num, value);
      //switchPWM[switchNum - DCOutput_Num ] = value;
      //ledcWrite(PWMOutput_Pin[switchNum - DCOutput_Num ], int(float(value) * 255.0 / 100.0));
      switchRen[switchNum - DCOutput_Num] = 0;
      }
      else      
      {
        switchRen[switchNum - DCOutput_Num - PWMOutput_Num] = value;
      }
      
    }
  }

    else if (type == 2)
  {

    state = value;
    state2 = state;
    if (ReverseOn)
      state = !state;
    if (value <= maxval)
      set_Pin(OnOutput_Pin, state);
    switchOn = state2;
  }

    else if (type == 3)
  {

    state = value;
    state2 = state;
    if (ReverseRelay)
      state = !state;
    if (value <= maxval)
      set_Pin(RelayOutput_Pin, state);
    switchRelay = state2;
  }

    else if (type == 4)
  {

    state = value;
    state2 = state;
    if (ReverseUSB)
      state = !state;
    if (value <= maxval)
      set_Pin(USBOutput_Pin[switchNum-DCOutput_Num-2*PWMOutput_Num-OnOutput_Num-RelayOutput_Num], state);
    switchUSB[switchNum-DCOutput_Num-2*PWMOutput_Num-OnOutput_Num-RelayOutput_Num] = state2;
  }

  else if (type == 5)
    Serial.println(ASCOM_err[2]);

  else if (type == 6)
  {
    //switchRen = bool(value);
    //regul = bool(value);
  }
}

void Switch::setpwm(int index, int value)
{
      switchPWM[index] = value;
      ledcWrite(PWMOutput_Pin[index], int(float(value) * 255.0 / 100.0));
}

int Switch::switchstep(int switchNum)
{
  return 1;
}

void Switch::TempEnv()
{
  Temperature = sht31.readTemperature();
}

void Switch::HumiEnv()
{
    Humidity = sht31.readHumidity();
}

float Switch::DewPoint()
{
  float T = Temperature;
  float RH = Humidity;
  float a = 17.625;
  float b = 243.04;
  float alpha = log(RH / 100.0) + a * T / (b + T);
  float dew = (b * alpha) / (a - alpha);
  return dew;
  // return 25;
}

void Switch::set_Pin(int x, bool y)
{
  if (x < 100)
    digitalWrite(x, y);
  else
    mcp.digitalWrite(x - 100, y);
}

void Switch::getAllSensors()
{
  float total = 0;
  float totaldc = 0;
  float totalpwm = 0;
  InputVoltage = (float(analogRead(36)) * 3.3 / 4096.0);
  float div = 1200.0 / (18000.0 + 1200.0);
  InputVoltage = InputVoltage / div + 2.28; // Applying +2.28V (calibrated value) to compensate ADC offset du to non linearity. Should be fairly accurate within the typical range of input Voltage (11-14V)

  for (int i = 0; i < Sensor_Num; i++)
  {  
    
    if (i<DCOutput_Num){ // if DC
      Sensor[4+i*2] = sensors[i].getBusVoltage();
      Sensor[4+i*2 + 1] = abs(sensors[i].getCurrent());
      if (Sensor[4+2*i + 1] < 0.06) Sensor[4+2*i + 1] = 0.0;
      totaldc = totaldc + Sensor[4+i*2 + 1];
    }
    else if ((i>= (DCOutput_Num)) && (i<(DCOutput_Num + PWMOutput_Num))) // if PWM
    {
      if (getswitchvalue(i) > 0) Sensor[4+i*2] = InputVoltage - (sensors[i].getBusVoltage_uV() - sensors[i].getShuntVoltage_uV()) / 1000000;
      else Sensor[4+i*2] = 0.00;
      Sensor[4+i*2 + 1] = abs(sensors[i].getCurrent()) * getswitchvalue(i) / 100.0;
      totalpwm = totalpwm + Sensor[4+i*2 + 1];
    }
    else if (i== (DCOutput_Num + PWMOutput_Num))// if On
    {
      Sensor[4+i*2] = sensors[i].getBusVoltage();
      Sensor[4+i*2 + 1] = abs(sensors[i].getCurrent());
      if (Sensor[4+2*i + 1] < 0.06) Sensor[4+2*i + 1] = 0.0;
    }
    //if (Sensor[4+2*i + 1] < 0.06) Sensor[4+2*i + 1] = 0.0;
    total =  total + Sensor[4+2*i + 1];
  }

  if(Ren)
      {
      TempEnv();
      HumiEnv();
      }
  
  TotalCurrent = total;
  Sensor[0] = InputVoltage;
  Sensor[1] = TotalCurrent;
  Sensor[2] = totaldc;
  Sensor[3] = totalpwm;
}



/* ALPACA squeezes every entity into a big switch array (be it DC, PWM or even sensors are switches in the ALPACA world).
 But with so many sensors, that makes a lot of different entities normally visible to ALPACA. We provide 3 solutions:
 1. Compact mode: only DC, Relay, On and PWM and essentials sensors are visible to ALPACA.
 2. Full mode: all sensors are visible to ALPACA, but the switch array is much larger.
 3. Custom mode: only the switches that are set to be visible in the ALPACA driver are visible to ALPACA.
 This means that ALPACA will count the switches differently if we show it only a subset of the switches.
And so the index of the switch in the client won't correspond to the index in the switch here in the firmware.
Of course it also means that in Full mode, those 2 indexes are the exact same.
To implement mode 3, we need to a translator function that maps the ALPACA switch index (given by the client) to the actual switch number as it would be if we showed the full array.
It will return the proper switch number for the given ALPACA switch index.
To do that, we have will use a boolean array telling about the visibility of each switch. This can be set by the setupdialog of the client and is stored in EEPROM.

ATTENTION: For now the ascom driver is configured to never show visibility options. The default is compact mode and you cannot change that in the driver yet. 
Visibility options will be enabled if requested or if I see a need for it. This only affects what is shown in you regular Windows client anyway, you still have every data available in the secondary window.
On Linux we also display everything in the INDI Driver since we have some control over the GUI*/

int Switch::index_Translator(int switchNum)
{
    int n= switchNum+1;
    int size = sizeof(Visible);
    // If the user requests to find the 0th '1', that's not possible.
    if (n <= 0) {
        return -1;
    }
    
    int countOfOnes = 0;
    
    // Loop through each element of the array
    for (int i = 0; i < size; i++) {
        // Check if the current element is a '1'
        if (Visible[i] == 1) {
            countOfOnes++;
            // Check if we have found the nth '1'
            if (countOfOnes == n) {
                return i; // Return the current index
            }
        }
    }
    
    // If the loop finishes and we haven't returned, it means
    // there were not enough '1's in the array.
    return -1;
}

int Switch::switchtype(int switchNum)
{
  /*
  The order expected from the ASCOM driver is as follow :
  DCOutput > relay > OnOutput > PWMOutput > Sensors > Ren
  */
  //int Total_switch_Num = DCOutput_Num + ADJOutput_Num + OnOutput_Num + PWMOutput_Num;
  //int sensor_Num = (Total_switch_Num * 2) + 2;

  if (switchNum < DCOutput_Num)
  {
    return 0; // DCOutput
  }
  else if (switchNum < DCOutput_Num + 2*PWMOutput_Num)
  {
    return 1; // PWMOutput
  }
  else if (switchNum < DCOutput_Num + 2*PWMOutput_Num + OnOutput_Num)
  {
    return 2; // OnOutput
  }
  else if (switchNum < DCOutput_Num + 2*PWMOutput_Num + OnOutput_Num + RelayOutput_Num)
  {
    return 3; // RelayOutput
  }
    else if ((USBOutput_Num!=0)&&(switchNum < totalswitches)) //Included an extra check to avoid errors if no USB outputs are defined
  {
    return 4; // USBOutput
  }
  else if ((switchNum >= totalswitches)&&(switchNum<(TotalOutputNum-3*(short)Ren)))
  {
    return 5; // Sensors
  }
    else if (Ren &&(switchNum >=(TotalOutputNum-3*(short)Ren)))
  {
    return 6; // Env. sensor
  }
  return -1; 
}
