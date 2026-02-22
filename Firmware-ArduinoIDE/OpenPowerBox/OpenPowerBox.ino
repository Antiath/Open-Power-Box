#include <Arduino.h>
#include "config.h"
#include "SwitchClass.h"
#include <SerialCommand.h>
#include "ErrorManager.h"
#include <Adafruit_MCP23X17.h>
#include "Definitions.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "EEPROMManager.h"


Switch _switch;

// Server declarations
WebServer *_ascomserver;                           // ASCOM server used for the ASCOM/ALAPCA client interface
WebSocketsServer webSocket = WebSocketsServer(81); // the websocket uses port 81 (standard port for websockets)
WiFiUDP _ASCOMDISCOVERYUdp;                        // UDP server for ASCOM discovery protocol. An Alpaca driver setup dialogue will probe the local network through UDP in order to find every devices that uses the ALPACA API.

int total;
int auto_out = 0;
int sensetimer, sensetimer2;
bool measureflag = 0;

bool UIChanged = false;

SerialCommand sCmd; // The demo SerialCommand object

double timesincelastupdate = 0;
int interval = 1000;              // send data to the client every 1000ms -> 1s
unsigned long previousMillis = 0; // we use the "millis()" command for time reference and this will output an unsigned long

// functions declaration for PLATFORMIO.
// Comment out this section if building in Arduino IDE
bool errortest(int, int);

void currentLimiter();

void processCommand();

void UpdateWebpageReadings();
void UpdatewifiParam();
void UpdateNameParam();
void UpdateLimitParam();
void UpdatePolarityParam();
void webSocketEvent(byte, WStype_t, uint8_t *, size_t);
void checkASCOMALPACADiscovery();
void getURLParameters();
int getURLindex();

void get_setup();
void get_man_version();
void get_man_description();
void get_man_configureddevices();
void get_description();
void get_name();
void get_driverinfo();
void get_driverversion();
void set_connected();
void set_connect();
void set_disconnect();
void get_connected();
void get_connecting();
String addclientinfo(String);
void get_supportedactions();
void sendreply(int, String, String);
void get_switch();
void set_switch();
void get_switchvalue();
void set_switchvalue();
void get_maxswitch();
void get_canwrite();
void get_switchdescription();
void get_switchname();
void get_minswitchvalue();
void get_maxswitchvalue();
void get_switchstep();
void set_averageperiod();
void get_notfound();
void get_interfaceversion();
void get_async();
void set_async();
void set_cancelasync();
void get_statechangecomplete();
//================================================

//===========================================================================
//===================== MAIN MICROCONTROLLER FUNCTIONS ======================
//===========================================================================

void setup()
{

  // Initializing the Serial port
  Serial.begin(115200);
  delay(2000);
  EEPROM.begin(EEPROM_SIZE);
  _switch = Switch(0);
  // Initializing the ascom server
  _ascomserver = new WebServer(4040);

  //Initializing WIFI-------------
    //ssid1=""; 
    //pwd1=""; 

  WiFi.begin(ssid1, pwd1);                                               // start WiFi interface
  Serial.println("Establishing connection to WiFi with SSID: " + ssid1); // print SSID to the serial interface for debugging
  Serial.println(WiFi.macAddress());
  
int x=0;
  while (WiFi.status() != WL_CONNECTED)
  { // wait until WiFi is connected
    delay(500);
    Serial.print(".");
    if(x>10) 
    {
      Serial.println("Failed to connect to wifi. Moving on");
      break;
    }
    x++;
  }

  Serial.print("Connected to network with IP address: ");
  Serial.println("#i0:" + WiFi.localIP().toString() + ";");
  // WIFI-------------

  if(WiFi.status() == WL_CONNECTED){
  // Initializing the ASCOM UDP server
  _ASCOMDISCOVERYUdp.begin(ASCOMDISCOVERYPORT);

  // Implementing the ALPACA API for a switch device.
  // Reference specifications : https://ascom-standards.org/api/#/ASCOM%20Methods%20Common%20To%20All%20Devices

  _ascomserver->on("/", get_setup);
  _ascomserver->on("/setup", get_setup);

  // // handle Management requests

  _ascomserver->on("/management/apiversions", get_man_version);
  _ascomserver->on("/management/v1/description", get_man_description);
  _ascomserver->on("/management/v1/configureddevices", get_man_configureddevices);

  // // handle ASCOM driver client requests

  _ascomserver->on("/api/v1/switch/0/connecting", HTTP_GET, get_connecting);
  _ascomserver->on("/api/v1/switch/0/connected", HTTP_GET, get_connected);
  _ascomserver->on("/api/v1/switch/0/connected", HTTP_PUT, set_connected);
  _ascomserver->on("/api/v1/switch/0/connect", HTTP_PUT, set_connect);
  _ascomserver->on("/api/v1/switch/0/disconnect", HTTP_PUT, set_disconnect);
  _ascomserver->on("/api/v1/switch/0/canasync", HTTP_GET, get_async);
  _ascomserver->on("/api/v1/switch/0/setasync", HTTP_PUT, set_async);
  _ascomserver->on("/api/v1/switch/0/setasyncvalue", HTTP_PUT, set_async);
  _ascomserver->on("/api/v1/switch/0/cancelasync", HTTP_PUT, set_cancelasync);
  _ascomserver->on("/api/v1/switch/0/canwrite", HTTP_GET, get_canwrite);
  _ascomserver->on("/api/v1/switch/0/statechangecomplete", HTTP_GET, get_statechangecomplete);
  _ascomserver->on("/api/v1/switch/0/interfaceversion", HTTP_GET, get_interfaceversion);
  _ascomserver->on("/api/v1/switch/0/name", HTTP_GET, get_name);
  _ascomserver->on("/api/v1/switch/0/description", HTTP_GET, get_description);
  _ascomserver->on("/api/v1/switch/0/driverinfo", HTTP_GET, get_driverinfo);
  _ascomserver->on("/api/v1/switch/0/driverversion", HTTP_GET, get_driverversion);
  _ascomserver->on("/api/v1/switch/0/getswitch", HTTP_GET, get_switch);
  _ascomserver->on("/api/v1/switch/0/getswitchvalue", HTTP_GET, get_switchvalue);
  _ascomserver->on("/api/v1/switch/0/setswitch", HTTP_PUT, set_switch);
  _ascomserver->on("/api/v1/switch/0/setswitchvalue", HTTP_PUT, set_switchvalue);
  _ascomserver->on("/api/v1/switch/0/maxswitch", HTTP_GET, get_maxswitch);
  _ascomserver->on("/api/v1/switch/0/getswitchdescription", HTTP_GET, get_switchdescription);
  _ascomserver->on("/api/v1/switch/0/getswitchname", HTTP_GET, get_switchname);
  //_ascomserver->on("/api/v1/switch/0/setswitchname", HTTP_PUT, set_switchname);
  _ascomserver->on("/api/v1/switch/0/minswitchvalue", HTTP_GET, get_minswitchvalue);
  _ascomserver->on("/api/v1/switch/0/maxswitchvalue", HTTP_GET, get_maxswitchvalue);
  _ascomserver->on("/api/v1/switch/0/switchstep", HTTP_GET, get_switchstep);
  _ascomserver->on("/api/v1/switch/0/supportedactions", HTTP_GET, get_supportedactions);



  // // handle url not found 404

  _ascomserver->onNotFound(get_notfound);
  _ascomserver->begin();
  webSocket.begin();                 // start websocket
  webSocket.onEvent(webSocketEvent); // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"
  }
  // Initializing the serial command parser
  sCmd.addCommand("#", processCommand);
  total = _switch.maxswitch();
  sensetimer = millis();
}

void loop()
{
  // Ascom Server execution
  if(WiFi.status() == WL_CONNECTED){
  _ascomserver->handleClient();
  // check for ALPACA discovery received packets
  checkASCOMALPACADiscovery();

  // Update function for the webSockets
  webSocket.loop();

  // Update function that sends the last mean measurtements to the webpage.
    UpdateWebpageReadings();    

  }
  // We restart the board every 48 hours to prevent any memory fragmentation.
  if (millis() > 2 * 24 * 60 * 60 * 1000)
    ESP.restart();

  // Serial bus Receiver handler
  sCmd.readSerial(); // We don't do much, just process serial commands

  if (Ren)
  {
      float delta = _switch.Temperature-_switch.DewPoint();

      for(int i=0;i<PWMOutput_Num;i++)
      {
        if(_switch.getswitchvalue(DCOutput_Num +PWMOutput_Num +i)==1) 
         {
          if(delta<2) _switch.setpwm(i, 100);
          else if ((delta>=2)&&(delta<3)) _switch.setpwm(i, 80);
          else if ((delta>=3)&&(delta<4)) _switch.setpwm(i, 60);
          else if ((delta>=4)&&(delta<5)) _switch.setpwm(i, 40);
          else if ((delta>=5)&&(delta<6)) _switch.setpwm(i, 20);
          else _switch.setpwm(i, 0);
         }
      }
  }

  // else
  // {
  //   for (int i=0; i<PWMOutput_Num; i++){
  //   _switch.setswitchvalue((DCOutput_Num+i), 0);
  //   }
  // }

  // Serial bus Transmitter handler
  sensetimer2 = millis() - sensetimer;

  if (sensetimer2 > MeasureInterval)
  {
    _switch.getAllSensors();
    currentLimiter();
    sensetimer = millis();
  }
}


//===========================================================================
//======================= USB/UART MESSAGE HANDLER ==========================
//===========================================================================

// This function is called when a command is received from the Serial port
// It processes the command and sends a response back to the Serial port
// The command format is:
// # <command> <switchNum> <value>
// For example:
// # S 1 1       Set switch 1 to value 1
// # G 4         Get switch 4 value
// if only the switch number is expected, <value> is not needed and if one is still provided, it is ignored.
// if <value> is not provided but expected, an error message is sent back to the Serial port (TO DO).
// The response format is:
// #<command><switchNum>:<value>;
// where <command> is the same as above, <switchNum> is the switch number and <value> is the value of the switch or the response to the command.
// The response is sent back to the Serial port.
// If the command is not recognized, an error message is sent back to the Serial port (TO DO).
// If the command is not valid, an error message is sent back to the Serial port (TO DO).

void processCommand()
{
  char *arg;
  char cmd;
  int i, value;
  String answer,value2;
  arg = sCmd.next();
  if (arg != NULL)
    cmd = arg[0];
  // else Serial.println("No arguments");

  arg = sCmd.next();
  if (arg != NULL)
    i = atoi(arg); // Converts a char string to an integer
  // else Serial.println("No second arguments");

  arg = sCmd.next();
  if (arg != NULL)
     {value = atol(arg);
     value2 = arg;}

  switch (cmd)
  {
  case 'S':
    if (_switch.getswitchvalue(SensorPos) < 10.0) {
    Serial.println(ASCOM_err[9]);//Checking undervoltage
    break;
    }
    UIChanged = true;
    Serial.println("Setting switch " + String(i) + " to value " + String(value));
    _switch.setswitchvalue(i, value);
    Serial.println("#G" + String(i) + ":" + String(_switch.getswitchvalue(i)) + ";");
    break;
  case 'G':
    Serial.println("#G" + String(i) + ":" + String(_switch.getswitchvalue(i)) + ";");
    break;
  case 'N':
    _switch.setswitchname(i, arg);
    Serial.println("#n" + String(i) + ":" + String(_switch.getswitchname(i)) + ";");
    break;
  case 'n':
    Serial.println("#n" + String(i) + ":" + String(_switch.getswitchname(i)) + ";");
    break;
  case 'D':
    Serial.println("#D" + String(i) + ":" + String(_switch.getswitchdescription(i)) + ";");
    break;
  case 'W':
    Serial.println("#W" + String(i) + ":" + String(_switch.canwrite(i)) + ";");
    break;
  case 'm':
    Serial.println("#m" + String(i) + ":" + String(_switch.minswitchvalue(i)) + ";");
    break;
  case 'M':
    Serial.println("#M" + String(i) + ":" + String(_switch.maxswitchvalue(i)) + ";");
    break;
  case 'X':
    Serial.println("#X0:" + String(TotalOutputNum) + ";");
    break;
  case 'Z':
    answer = "#Z0:";
    answer += String(DCOutput_Num) + ",";
    answer += String(PWMOutput_Num) + ",";
    answer += String(RelayOutput_Num) + ",";
    answer += String(OnOutput_Num) + ",";
    answer += String(USBOutput_Num) + ",";
    answer += String((short)Ren) + ";";
    Serial.println(answer);
    break;
  case 'r':
    Serial.println("#r" + String(i) + ":" + String(_switch.getreverse(i)) + ";");
    break;
  case 'R':
    _switch.setreverse(i, value);
    Serial.println("#r" + String(i) + ":" + String(_switch.getreverse(i)) + ";");
    break;
  case 'l':
    Serial.println("#l" + String(i) + ":" + String(_switch.getlimit(i)) + ";");
    break;
  case 'L':
    _switch.setlimit(i, value);
    Serial.println("#l" + String(i) + ":" + String(_switch.getlimit(i)) + ";");
    break;    
  case 'T':
    Serial.println("#T" + String(i) + ":" + String(_switch.getswitchtype(i)) + ";");
    break;
  case 'Y':
  _switch.Visible[i] = value;
    Serial.println("#y" + String(i) + ":" + String(_switch.Visible[i]) + ";");
    break;
    case 'y':
    Serial.println("#y" + String(i) + ":" + String(_switch.Visible[i]) + ";");
    break;
    case 'I':
    Serial.println("#i0:" + WiFi.localIP().toString() + ";");
    break;    
    case 'f':
    Serial.println("#f0:" + ssid1 + ";");
    break; 
    case 'F':
    ssid1=value2;
    StoreString(EEPROM_SSID_ADD, ssid1);
    EEPROM.commit();
    Serial.println("#f0:" + ssid1 + ";");
    break; 
    case 'h':
    Serial.println("#h0:***;");
    break; 
    case 'H':
    pwd1=value2;
    StoreString(EEPROM_PWD_ADD, pwd1);
    EEPROM.commit();   
    Serial.println("#h0:***;");
    break; 
    case 'p':
    ESP.restart();
    break;
    case 'e':
    Serial.println(SwitchErrorMessage);
    break; 
  default:
    Serial.println("Unknown command received");
    break;
  }
}

//===========================================================================
//============================== ERROR HANDLER ==============================
//===========================================================================

bool errortest(int num, int value)
{
  if (Ren == 0)
  {
    if (num > (DCOutput_Num + PWMOutput_Num))
    {
      ASCOM_err_flag[0]=true;
      SwitchErrorMessage = ASCOM_err[0];
      Serial.println(ASCOM_err[0]);
      return 0;
    }
    else ASCOM_err_flag[0]=false;
  }
  else
  {
    if ((num > (DCOutput_Num + PWMOutput_Num)) && (num != total - 1))
    {
      ASCOM_err_flag[0]=true;
      SwitchErrorMessage = ASCOM_err[0];
      Serial.println(ASCOM_err[0]);
      return 0;
    }
    else ASCOM_err_flag[0]=false;
  }

  if ((value > _switch.maxswitchvalue(num)) || (value < _switch.minswitchvalue(num)))
  {
    SwitchErrorMessage = ASCOM_err[1];
    Serial.println(ASCOM_err[1]);
    return 0;
  }
  else ASCOM_err_flag[1]=false;
  return 1;
}

//===========================================================================
//============================ CURRENT LILMITER =============================
//===========================================================================
/* This function is used to limit the current flowing through the system
It is called at regular intervals
Its function is to check the current consumption of the system and to turn off the switches if the current exceeds a certain limit.
Two mode of limiter are engaged at the same time:
  1. The first mode is to turn off a particular switch if the current exceeds its given limit
  2. The second mode is to turn off certain switches in a priority order if the total current exceeds a certain limit.
  The priority order is as follows:
  1. DC Rail bank : 5A whole bank
  2. PWM switches : 3A each
  3. DC switches : 6A each
  In addition, A limiter is set up for the whole DC bank and one for the whole PWM bank.
  The DC bank limiter is set to 10A and the PWM bank limiter is set to 6A.
  The general limiter is set to 25A.
  Those limits are set in the config.h file but it is higly recommended to not change them.
  We would like to remember you that this device is a DIY project and may or may not be safe.
  We decline responsibility for any damage caused by the malfunction of this firmware.*/

void currentLimiter()
{
  int i = 0;
  int j = 0;
  while (i < DCOutput_Num)
  {
    if (_switch.getswitchvalue(sensorDC0 + i * 2 + 1) > _switch.DClimit)
    {
      _switch.setswitchvalue(i, 0);
      _switch.DClimitflag[i] = true;
      Serial.println(ASCOM_err[3] + " DC Output " + String(i) + " Current :" + String(_switch.getswitchvalue(sensorDC0 + i * 2 + 1)) + " Limit :" + String(_switch.DClimit));
    }
    else
      _switch.DClimitflag[i] = false;
    i++;
  }

    while (i < DCOutput_Num + PWMOutput_Num)
  {
    if (_switch.getswitchvalue(sensorPWM0 + j * 2 + 1) > _switch.PWMlimit)
    {
      _switch.setswitchvalue(i, 0);
      _switch.PWMlimitflag[j] = true;
      Serial.println(ASCOM_err[5] + " PWM Output " + String(i));
    }
    else
      _switch.PWMlimitflag[j] = false;
    i++;
    j++;
  }

  i++;
  if (_switch.getswitchvalue(sensorOn0 + 1) > _switch.Onlimit)
  {
    _switch.setswitchvalue(i, 0);
    _switch.Onlimitflag = true;
    Serial.println(ASCOM_err[4] + " On Output " + String(i));
  }
  else
    _switch.Onlimitflag = false;
  i++;


  // Check the total current consumption of the system
  if (_switch.TotalCurrentDC > _switch.TotalDClimit)
  {
      ASCOM_err_flag[6]=true;
      SwitchErrorMessage = ASCOM_err[6];
    _switch.TotalDClimitflag = true;
    Serial.println(ASCOM_err[6] + " Total DC current limit exceeded: " + String(_switch.TotalCurrentDC) + "A");
    for (int i = 0; i < DCOutput_Num; i++)
    {
      _switch.setswitchvalue(i, 0);
    }
  }
  else
    _switch.TotalDClimitflag = false;

  if (_switch.TotalCurrentPWM > _switch.TotalPWMlimit)
  {
    _switch.TotalPWMlimitflag = true;
    Serial.println(ASCOM_err[7] + " Total PWM current limit exceeded: " + String(_switch.TotalCurrentPWM ) + "A");
    for (int i = 0; i < PWMOutput_Num; i++)
    {
      _switch.setswitchvalue(DCOutput_Num + i, 0);
    }
  }
  else
    _switch.TotalPWMlimitflag = false;

  if (_switch.TotalCurrent > _switch.Totallimit)
  {
    Serial.println(ASCOM_err[8] + " Total current limit exceeded: " + String(_switch.TotalCurrent) + "A");
    _switch.setswitchvalue(DCOutput_Num+PWMOutput_Num, 0); // Turn off On
      if (_switch.TotalCurrent > _switch.Totallimit)
      {
        Serial.println(ASCOM_err[8] + " Total current limit exceeded: " + String(_switch.TotalCurrent) + "A");
        for (int i = 0; i < PWMOutput_Num; i++)
        {
          _switch.setswitchvalue(DCOutput_Num+i, 0); // Turn off all PWM outputs
          _switch.TotalCurrentflag = true;
        }
      }
    }
  
  else
    _switch.TotalCurrentflag = false;


    //Setting flags for error messages to display in the client
    SwitchErrorMessage="#E0: ;";
    for(int i=0; i<DCOutput_Num;i++) 
    {
      if(_switch.DClimitflag[i]) 
      {
        SwitchErrorMessage= ASCOM_err[3];
        ASCOM_err_flag[3]=true; 
      }
      else ASCOM_err_flag[3]=false;
    }
    
    if(_switch.Onlimitflag) 
    {
     SwitchErrorMessage= ASCOM_err[4];
      ASCOM_err_flag[4]=true; 
    }
    else ASCOM_err_flag[4]=false;

    for(int i=0; i<PWMOutput_Num;i++) 
    {
      if(_switch.PWMlimitflag[i]) 
      {
        SwitchErrorMessage= ASCOM_err[5];
        ASCOM_err_flag[5]=true; 
      }
      else ASCOM_err_flag[5]=false;
    }

    if(_switch.TotalDClimitflag)
    {
      ASCOM_err_flag[6]=true; 
      SwitchErrorMessage= ASCOM_err[6];
    }
    if(_switch.TotalPWMlimitflag)
    {
      ASCOM_err_flag[7]=true; 
      SwitchErrorMessage= ASCOM_err[7];
    }
    if(_switch.TotalCurrentflag)
    {
      ASCOM_err_flag[8]=true; 
      SwitchErrorMessage= ASCOM_err[8];
    }
    if(_switch.InputVoltage<10.0)
  {
    ASCOM_err_flag[9]=true;
    SwitchErrorMessage = ASCOM_err[9];
  }
  else ASCOM_err_flag[9]=false;
}



//===========================================================================
//============================ WEBPAGE FUNCTIONS =============================
//===========================================================================

void UpdateWebpageReadings()
{

  unsigned long now = millis(); // read out the current "time" ("millis()" gives the time in ms since the Arduino started)
  if ((unsigned long)(now - previousMillis) > interval)
  {                                           // check if "interval" ms has passed since last time the clients were updated
    String jsonString = "";                   // create a JSON string for sending data to the client
    StaticJsonDocument<300> doc;              // create a JSON container
    JsonObject object = doc.to<JsonObject>(); // create a JSON Object
    object["answer"] = "SEND_READINGS";
    //if(UIChanged){
    for(int i = 0; i < DCOutput_Num; i++)
    {
      object["DC"+String(i+1)] = String(_switch.getswitch(i));
    }
    if(USBOutput_Num>0){
        for(int i = 0; i < USBOutput_Num; i++)
    {
      object["USB"+String(i+1)] = String(_switch.getswitch(DCOutput_Num+PWMOutput_Num+OnOutput_Num+1+i));
    }
      }
        for(int i = 0; i < PWMOutput_Num; i++)
    {
      object["PWM"+String(i+1)] = String(_switch.getswitch(DCOutput_Num+i));
      if(Ren)object["AUTO"+String(i+1)] = String(_switch.getswitchvalue(DCOutput_Num+PWMOutput_Num+i));
    }
    object["ON"] = String(_switch.getswitch(DCOutput_Num+PWMOutput_Num));
    object["RELAY"] = String(_switch.getswitch(DCOutput_Num+PWMOutput_Num+1));
    //UIChanged = false;
  //}
    object["INV"] = String(_switch.getswitchvalue(SensorPos)) + "V";
    object["INA"] = String(_switch.getswitchvalue(SensorPos+1)) + "A";
    object["INP"] = String(_switch.getswitchvalue(SensorPos) * _switch.getswitchvalue(SensorPos+1)) + "W";
    
    if(Ren)
    {
    object["TEMP"] = String(_switch.getswitchvalue(TotalOutputNum-3)) + "°C";
    object["HUM"] = String(_switch.getswitchvalue(TotalOutputNum-2)) + "%";
    object["DEW"] = String(_switch.getswitchvalue(TotalOutputNum-1)) + "°C"; 
    }
    
    for(int i = 0; i < DCOutput_Num; i++)
    {
      object["DC"+String(i+1)+"V"] = String(_switch.getswitchvalue(sensorDC0 + i * 2)) + "V";
      object["DC"+String(i+1)+"A"] = String(_switch.getswitchvalue(sensorDC0 + i * 2 + 1)) + "A";
    }
    for(int i = 0; i < PWMOutput_Num; i++)
    {
      object["PWM"+String(i+1)+"V"] = String(_switch.getswitchvalue(sensorPWM0 + i * 2)) + "V";
      object["PWM"+String(i+1)+"A"] = String(_switch.getswitchvalue(sensorPWM0 + i * 2 + 1)) + "A";
    }
    object["ONV"] = String(_switch.getswitchvalue(sensorOn0)) + "V";
    object["ONA"] = String(_switch.getswitchvalue(sensorOn0 + 1)) + "A";
    serializeJson(doc, jsonString);     // convert JSON object to string
    webSocket.broadcastTXT(jsonString); // send JSON string to clients
    previousMillis = now;               // reset previousMillis
  }
}

void UpdatewifiParam()
{
  // read out the current "time" ("millis()" gives the time in ms since the Arduino started)
  // check if "interval" ms has passed since last time the clients were updated
  String jsonString = "";                   // create a JSON string for sending data to the client
  StaticJsonDocument<500> doc;              // create a JSON container
  JsonObject object = doc.to<JsonObject>(); // create a JSON Object
  object["answer"] = "SEND_WIFI";
  object["ssid"] = ssid1;
  object["pwd"] = pwd1;
  serializeJson(doc, jsonString);     // convert JSON object to string
  Serial.println(jsonString);         // print JSON string to console for debug purposes (you can comment this out)
  webSocket.broadcastTXT(jsonString); // send JSON string to clients
}

void UpdateNameParam()
{
  String jsonString = "";                   // create a JSON string for sending data to the client
  StaticJsonDocument<500> doc;              // create a JSON container
  JsonObject object = doc.to<JsonObject>(); // create a JSON Object
  object["answer"] = "SEND_NAME";
  for (int p = 0; p < totalswitches; p++)
    object["n" + String(p)] = _switch.getswitchname(p); // We just send the actual switches. The name of the sensors are not needed for the webpage.
  serializeJson(doc, jsonString);                       // convert JSON object to string
  Serial.println(jsonString);                           // print JSON string to console for debug purposes (you can comment this out)
  webSocket.broadcastTXT(jsonString);                   // send JSON string to clients
}

void UpdateLimitParam()
{
  String jsonString = "";                   // create a JSON string for sending data to the client
  StaticJsonDocument<500> doc;              // create a JSON container
  JsonObject object = doc.to<JsonObject>(); // create a JSON Object
  object["answer"] = "SEND_LIMIT";
  object["dcl"] = String(_switch.DClimit);
  object["pwml"] = String(_switch.PWMlimit);
  object["onl"] = String(_switch.Onlimit);
  object["tdcl"] = String(_switch.TotalDClimit);
  object["tpwml"] = String(_switch.TotalPWMlimit);
  object["tl"] = String(_switch.Totallimit);
  serializeJson(doc, jsonString);     // convert JSON object to string
  Serial.println(jsonString);         // print JSON string to console for debug purposes (you can comment this out)
  webSocket.broadcastTXT(jsonString); // send JSON string to clients
}

void UpdatePolarityParam()
{
  String jsonString = "";                   // create a JSON string for sending data to the client
  StaticJsonDocument<500> doc;              // create a JSON container
  JsonObject object = doc.to<JsonObject>(); // create a JSON Object
  object["answer"] = "SEND_POL";
  object["dcR"] = String(_switch.ReverseDC);
  object["relR"] = String(_switch.ReverseRelay);
  object["onR"] = String(_switch.ReverseOn);
  object["pwmR"] = String(_switch.ReversePWM);
  serializeJson(doc, jsonString);     // convert JSON object to string
  Serial.println(jsonString);         // print JSON string to console for debug purposes (you can comment this out)
  webSocket.broadcastTXT(jsonString); // send JSON string to clients
}

// ----------------------------------------------------------------------
// webSocketEvent(args)
// This function will receive data from the webpage and respond accordingly
// ----------------------------------------------------------------------

void webSocketEvent(byte num, WStype_t type, uint8_t *payload, size_t length)
{ // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type)
  {                         // switch on the type of information sent
  case WStype_DISCONNECTED: // if a client is disconnected, then type == WStype_DISCONNECTED
    Serial.println("Client " + String(num) + " disconnected");
    break;
  case WStype_CONNECTED: // if a client is connected, then type == WStype_CONNECTED
    Serial.println("Client " + String(num) + " connected");
    // optionally you can add code here what to do when connected
    break;
  case WStype_TEXT: // if a client has sent data, then type == WStype_TEXT
    // try to decipher the JSON string received
    StaticJsonDocument<500> doc; // create a JSON container
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    else
    {
      // JSON string was received correctly, so information can be retrieved:
      String data, label;
      int buf;
      float f;
      char str[30];
      String g_request = doc["request"].as<String>();
      if (g_request == "SEND_BACK_WIFI")
      {
        data = doc["_ssid"].as<String>();
        if ((data != "") && (data != ssid1))
        {
          ssid1 = data;
          StoreString(EEPROM_SSID_ADD, data);
        }
        data = doc["_pwd"].as<String>();
        if ((data != "") && (data != pwd1))
        {
          pwd1 = data;
          StoreString(EEPROM_PWD_ADD, data);
        }
        EEPROM.commit();
        delay(1000);
        //ESP.restart();
        return;
      }

      else if (g_request == "SEND_BACK_LIMITS")
      {
        data = doc["_l1"].as<String>();
        if ((data != "") && (data != String(_switch.DClimit)))
        { 
          Serial.println(data);
          _switch.DClimit = data.toFloat();
          StoreString(EEPROM_LDC_ADD, data);
        }
        data = doc["_l3"].as<String>();
        if ((data != "") && (data != String(_switch.Onlimit)))
        {
          _switch.Onlimit = data.toFloat();
          StoreString(EEPROM_LON_ADD, data);
        }
        data = doc["_l2"].as<String>();
        if ((data != "") && (data != String(_switch.PWMlimit)))
        {
          _switch.PWMlimit = data.toFloat();
          StoreString(EEPROM_LPWM_ADD, data);
        }
        data = doc["_l4"].as<String>();
        if ((data != "") && (data != String(_switch.TotalDClimit)))
        {
          _switch.TotalDClimit = data.toFloat();
          StoreString(EEPROM_LTDC_ADD, data);
        }
        data = doc["_l5"].as<String>();
        if ((data != "") && (data != String(_switch.TotalPWMlimit)))
        {
          _switch.TotalPWMlimit = data.toFloat();
          StoreString(EEPROM_LTPWM_ADD, data);
        }
        data = doc["_l6"].as<String>();
        if ((data != "") && (data != String(_switch.Totallimit)))
        {
          _switch.Totallimit = data.toFloat();
          StoreString(EEPROM_LT_ADD, data);
        }
        EEPROM.commit();
        delay(1000);
        return;
      }

      else if (g_request == "SEND_BACK_NAMES")
      {
        for (int i = 0; i <totalswitches; i++)
        {
          label = "_n" + String(i + 1);
          data = doc[label].as<String>();
          Serial.println("Received name for Switch " + String(i) + ": " + data);
          if ((data != "") && (data != _switch.getswitchname(i)))
          {
            Serial.println("Storing name for Switch " + String(i) + ": " + data);
            _switch.setswitchname(i, data);
           // StoreString(EEPROM_NAME_ADD + i * 20, data);
          }
        }
        EEPROM.commit();
        delay(1000);
        //ESP.restart();
        return;
      }

      else if (g_request == "SEND_BACK_POL")
      {
        data = doc["_p1"].as<String>();
        if ((data != "") && (data != String(_switch.ReverseDC)))
        {
          Serial.println(data);
          _switch.ReverseDC = data.toInt();
          StoreByte(EEPROM_RDC_ADD, data.toInt());
        }
        data = doc["_p2"].as<String>();
        if ((data != "") && (data != String(_switch.ReverseRelay)))
        {
          Serial.println(data);
          _switch.ReverseRelay = data.toInt();
          StoreByte(EEPROM_RREL_ADD, data.toInt());
        }
        data = doc["_p3"].as<String>();
        if ((data != "") && (data != String(_switch.ReverseOn)))
        {
          Serial.println(data);
          _switch.ReverseOn = data.toInt();
          StoreByte(EEPROM_RON_ADD, data.toInt());
        }
        data = doc["_p4"].as<String>();
        if ((data != "") && (data != String(_switch.ReversePWM)))
        {
          Serial.println(data);
          _switch.ReversePWM = data.toInt();
          StoreByte(EEPROM_RPWM_ADD, data.toInt());
        }
        EEPROM.commit();
        delay(1000);
        //ESP.restart();
        return;
      }
      else if (String(g_request) == "GET_WIFI")
      { 
        UpdatewifiParam();
      }
      else if (String(g_request) == "GET_NAME")
      { 
        UpdateNameParam();
      }
      else if (String(g_request) == "GET_LIM")
      {
        UpdateLimitParam();
      }
      else if (String(g_request) == "GET_POL")
      {
        UpdatePolarityParam();
      }

      else if (g_request == "SEND_DCSTATE")
      {
        int ind, val;
        data = doc["nb"].as<String>();
        if (data != "")
          ind = data.toInt();
        data = doc["state"].as<String>();
        if (data != "")
          val = data.toInt();
        _switch.setswitch(ind, (bool)val);
        Serial.println("Received dcstate from webpage for Switch " + String(ind) + " value" + String(val));
        return;
      }

      else if (g_request == "SEND_AUTOSTATE")
      {
        if(Ren)
        {
        int ind, val;
        data = doc["nb"].as<String>();
        if (data != "")
          ind = data.toInt();
        data = doc["state"].as<String>();
        if (data != "")
          val = data.toInt();
        _switch.setswitchvalue((DCOutput_Num+PWMOutput_Num+ind), val);
        Serial.println("Received autostate from webpage for Switch " + String(ind) + " value" + String(val));
        return;
        }
      }
      else if (g_request == "SEND_USBSTATE")
      {
        if(USBOutput_Num>0){
        int ind, val;
        data = doc["nb"].as<String>();
        if (data != "")
          ind = data.toInt();
        data = doc["state"].as<String>();
        if (data != "")
          val = data.toInt();
        _switch.setswitch((DCOutput_Num+2*PWMOutput_Num+OnOutput_Num+1+ind), (bool)val);
        Serial.println("Received usbstate from webpage for Switch " + String(ind) + " value" + String(val));
        return;
        }
      }

      else if (g_request == "SEND_RELAYSTATE")
      {
        int val;
        data = doc["state"].as<String>();
        if (data != "")
          val = data.toInt();
        _switch.setswitch(DCOutput_Num+2*PWMOutput_Num+OnOutput_Num, (bool)val);
        return;
      }
      else if (g_request == "SEND_ONSTATE")
      {
        int val;
        data = doc["state"].as<String>();
        if (data != "")
          val = data.toInt();
        _switch.setswitch(DCOutput_Num + 2*PWMOutput_Num, (bool)val);
        return;
      }
      else if (g_request == "SEND_PWMSTATE")
      {
        int ind, val;
        data = doc["nb"].as<String>();
        if (data != "")
          ind = data.toInt();
        data = doc["state"].as<String>();
        if (data != "")
          val = data.toInt();
        _switch.setswitchvalue(DCOutput_Num + ind-1, val);
        return;
      }
    }
    break;
  }
}

//========================================================================================
//====================================ALPACA FUNCTIONS====================================
//========================================================================================

// ----------------------------------------------------------------------
// checkASCOMALPACADiscovery()
// This function will answer Alpaca discovery requests
// ----------------------------------------------------------------------

void checkASCOMALPACADiscovery()
{
  // (c) Daniel VanNoord
  // https://github.com/DanielVanNoord/AlpacaDiscoveryTests/blob/master/Alpaca8266/Alpaca8266.ino
  // if there's data available, read a packet

  int packetSize = _ASCOMDISCOVERYUdp.parsePacket();
  if (packetSize)
  {
    char ipaddr[16];
    IPAddress remoteIp = _ASCOMDISCOVERYUdp.remoteIP();
    snprintf(ipaddr, sizeof(ipaddr), "%i.%i.%i.%i", remoteIp[0], remoteIp[1], remoteIp[2], remoteIp[3]);
    // read the packet into packetBufffer
    int len = _ASCOMDISCOVERYUdp.read(_packetBuffer, 255);
    Serial.println(_packetBuffer);
    if (len > 0)
    {
      // Ensure that it is null terminated
      _packetBuffer[len] = 0;
    }
    // No undersized packets allowed
    if (len < 16)
    {
      return;
    }
    // 0-14 "alpacadiscovery", 15 ASCII Version number of 1
    if (strncmp("alpacadiscovery1", _packetBuffer, 16) != 0)
    {
      return;
    }

    String strresponse = "{\"alpacaport\":" + String(4040) + "}";
    uint8_t response[36] = {0};
    len = strresponse.length();

    // copy to response

    for (int i = 0; i < len; i++)
    {
      response[i] = (uint8_t)strresponse[i];
    }
    Serial.println(strresponse);
    _ASCOMDISCOVERYUdp.beginPacket(_ASCOMDISCOVERYUdp.remoteIP(), _ASCOMDISCOVERYUdp.remotePort());
    _ASCOMDISCOVERYUdp.write(response, len);
    _ASCOMDISCOVERYUdp.endPacket();
  }
}

// ----------------------------------------------------------------------
// getURLParameters()
// This function will read any data sent from the ASCOM client and parse.
// ----------------------------------------------------------------------

void getURLParameters()
{
  String str;

  for (int i = 0; i < _ascomserver->args(); i++)
  {

    if (i >= ASCOMMAXIMUMARGS)
    {
      break;
    }
    str = _ascomserver->argName(i);
    str.toLowerCase();
    if (str.equals("clientid"))
    {
      _ASCOMClientID = (unsigned int)_ascomserver->arg(i).toInt();
    }
    if (str.equals("clienttransactionid"))
    {
      _ASCOMClientTransactionID = (unsigned int)_ascomserver->arg(i).toInt();
    }
    if (str.equals("connected"))
    {
      String strtmp = _ascomserver->arg(i);
      strtmp.toLowerCase();
      if (strtmp.equals("true"))
      {
        _ASCOMConnectedState = 1;
      }
      else
      {
        _ASCOMConnectedState = 0;
      }
    }
  }
}

int getURLindex()
{
  String strtmp = "";
  int id = -1;
  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";

  // getting parameters
  String str;
  for (int i = 0; i < _ascomserver->args(); i++)
  {
    if (i >= ASCOMMAXIMUMARGS)
    {
      break;
    }
    str = _ascomserver->argName(i);
    str.toLowerCase();
    if (str.equals("clientid"))
    {
      _ASCOMClientID = (unsigned int)_ascomserver->arg(i).toInt();
    }
    if (str.equals("clienttransactionid"))
    {
      _ASCOMClientTransactionID = (unsigned int)_ascomserver->arg(i).toInt();
    }
    if (str.equals("id"))
    {
      strtmp = _ascomserver->arg(i);
      strtmp.toLowerCase();
      id = strtmp.toInt();
    }
  }
  return id;
}

// ----------------------------------------------------------------------
// get_man_setup()
// This function uploads the webpage to the browser
// ----------------------------------------------------------------------



void get_setup()
{

  _ASCOMServerTransactionID++;

  _ascomserver->client().println("HTTP/1.1 200 OK");
  _ascomserver->client().println("Content-type:text/html");
  _ascomserver->client().println();
  //_ascomserver->client().print(webpage);
  //_ascomserver->client().print(String(USBOutput_Num));
  _ascomserver->client().print(webpage1);
}

// ----------------------------------------------------------------------
// get_man_version()
// ----------------------------------------------------------------------
void get_man_version()
{

  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":[1]," + addclientinfo(jsonretstr);

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_man_description()
// ----------------------------------------------------------------------
void get_man_description()
{

  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(ASCOMMANAGEMENTINFO) + "," + addclientinfo(jsonretstr);

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_man_configureddevices()
// ----------------------------------------------------------------------
void get_man_configureddevices()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":[{\"DeviceName\":" + String(ASCOMNAME) + ",\"DeviceType\":\"switch\",\"DeviceNumber\":0,\"UniqueID\":\"" + String(ASCOMGUID) + "\"}]," + addclientinfo(jsonretstr);

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_interfaceversion()
// ----------------------------------------------------------------------
void get_interfaceversion()
{

  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(3) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_description()
// ----------------------------------------------------------------------
void get_description()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();

  jsonretstr = "{\"Value\":" + String(ASCOMDESCRIPTION) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_name()
// ----------------------------------------------------------------------
void get_name()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(ASCOMNAME) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_driverinfo()
// ----------------------------------------------------------------------
void get_driverinfo()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(ASCOMDRIVERINFO) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_driverversion()
// ----------------------------------------------------------------------
void get_driverversion()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":\"" + String(program_version) + "\",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// set_connect()
// ----------------------------------------------------------------------
void set_connect()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters(); // checks connected param and sets _ASCOMConnectedState
  _ASCOMConnectedState = 1;
  jsonretstr = "{ \"Errornumber\":0, \"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void set_disconnect()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters(); // checks connected param and sets _ASCOMConnectedState
  _ASCOMConnectedState = 0;
  jsonretstr = "{ \"Errornumber\":0, \"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// set_connected()
// ----------------------------------------------------------------------
void set_connected()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters(); // checks connected param and sets _ASCOMConnectedState
  jsonretstr = "{ \"Errornumber\":0, \"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_connected()
// ----------------------------------------------------------------------
void get_connected()
{
  String jsonretstr = "";
  String answer = "false";
  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters(); // checks connected param and sets _ASCOMConnectedState
  if (_ASCOMConnectedState) answer = "true";
    jsonretstr = "{\"Value\":" + answer + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}


// ----------------------------------------------------------------------
// get_connecting()
// ----------------------------------------------------------------------
void get_connecting()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  bool answer = false; // We do not support connecting state, so always return false
  getURLParameters(); // checks connected param and sets _ASCOMConnectedState
  jsonretstr = "{\"Value\":false,\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// addclientinfo
// Adds client info to reply
// ----------------------------------------------------------------------
String addclientinfo(String str)
{
  String str1 = str;
  // add clientid
  str1 = str1 + "\"ClientID\":" + String(_ASCOMClientID) + ",";
  // add clienttransactionid
  str1 = str1 + "\"ClientTransactionID\":" + String(_ASCOMClientTransactionID) + ",";
  // add ServerTransactionID
  str1 = str1 + "\"ServerTransactionID\":" + String(_ASCOMServerTransactionID) + ",";
  // add errornumber
  str1 = str1 + "\"ErrorNumber\":" + String(_ASCOMErrorNumber) + ",";
  // add errormessage
  str1 = str1 + "\"ErrorMessage\":\"" + _ASCOMErrorMessage + "\"}";
  return str1;
}

void get_supportedactions()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  // get clientID and clienttransactionID
  getURLParameters();
  jsonretstr = "{\"Value\": [\"set_ADJVoltage\",\"set_IndivLimit\",\"set_DCLimit\",\"set_PWMLimit\",\"set_GlobalLimit\"]," + addclientinfo(jsonretstr);

  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// sendreply
// send a reply to client
// ----------------------------------------------------------------------
void sendreply(int replycode, String contenttype, String jsonstr)
{
  // ascomserver.send builds the http header, jsonstr will be in the body
  _ascomserver->send(replycode, contenttype, jsonstr);
}

void get_switch()
{
  String jsonretstr = "";
  String answer = "false";
  int id = getURLindex();
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":" + String("1025") + ",\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
  if(_switch.getswitch(id)) answer = "true";
    jsonretstr = "{\"Value\":" + answer + ",\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_switchvalue()
{
  String jsonretstr = "";
  int id = getURLindex();
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":1025,\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
    jsonretstr = "{\"Value\":" + String(_switch.getswitchvalue(id)) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void set_switch()
{
  String jsonretstr = "";
  String strtmp = "";
  int id;
  bool state;
  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";

  // getting parameters
  String str;
  for (int i = 0; i < _ascomserver->args(); i++)
  {
    if (i >= ASCOMMAXIMUMARGS)
    {
      break;
    }
    str = _ascomserver->argName(i);
    str.toLowerCase();
    if (str.equals("clientid"))
    {
      _ASCOMClientID = (unsigned int)_ascomserver->arg(i).toInt();
    }
    if (str.equals("clienttransactionid"))
    {
      _ASCOMClientTransactionID = (unsigned int)_ascomserver->arg(i).toInt();
    }
    if (str.equals("id"))
    {
      strtmp = _ascomserver->arg(i);
      strtmp.toLowerCase();
      id = strtmp.toInt();
    }
    if (str.equals("state"))
    {
      strtmp = _ascomserver->arg(i);
      strtmp.toLowerCase();
      state = strtmp.toInt();
    }
  }
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":1025,\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
  {
    jsonretstr = "{ \"ErrorNumber\":0, \"ErrorMessage\":\"\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    UIChanged = true;
    _switch.setswitch(id, state);
  }
}

void set_switchvalue()
{
  String jsonretstr = "";
  String strtmp = "";
  int id;
  float value;
  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";

  // getting parameters
  String str;
  for (int i = 0; i < _ascomserver->args(); i++)
  {
    if (i >= ASCOMMAXIMUMARGS)
    {
      break;
    }
    str = _ascomserver->argName(i);
    str.toLowerCase();
    if (str.equals("clientid"))
    {
      _ASCOMClientID = (unsigned int)_ascomserver->arg(i).toInt();
    }
    if (str.equals("clienttransactionid"))
    {
      _ASCOMClientTransactionID = (unsigned int)_ascomserver->arg(i).toInt();
    }
    if (str.equals("id"))
    {
      strtmp = _ascomserver->arg(i);
      strtmp.toLowerCase();
      id = strtmp.toInt();
    }
    if (str.equals("value"))
    {
      strtmp = _ascomserver->arg(i);
      strtmp.toLowerCase();
      value = strtmp.toFloat();
    }
  }
  Serial.println("Set switchvalue id " + String(id) + " to " + String(int(value)));
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":1025,\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
  {
    UIChanged = true;
    _switch.setswitchvalue(id, int(value));
    jsonretstr = "{ \"ErrorNumber\":0, \"ErrorMessage\":\"\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
  }
}

void get_maxswitch()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
  getURLParameters();
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":" + String(_switch.maxswitch()) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_switchdescription()
{
  String jsonretstr = "";
  int id = getURLindex();
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":1025,\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
    jsonretstr = "{\"Value\": \""+ _switch.getswitchdescription(id) +"\",\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_switchname()
{
  String jsonretstr = "";
  int id = getURLindex();
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":1025,\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
    jsonretstr = "{\"Value\": \""+ _switch.getswitchname(id) +"\",\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_minswitchvalue()
{
  String jsonretstr = "";
  int id = getURLindex();
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":" + String("1025") + ",\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
    jsonretstr = "{\"Value\":" + String(_switch.minswitchvalue(id)) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_maxswitchvalue()
{
  String jsonretstr = "";
  int id = getURLindex();
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":" + String("1025") + ",\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
    jsonretstr = "{\"Value\":" + String(_switch.maxswitchvalue(id)) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_switchstep()
{
  String jsonretstr = "";
  int id = getURLindex();
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":1025,\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
    jsonretstr = "{\"Value\":" + String(_switch.switchstep(id)) + ",\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_canwrite()
{
  String jsonretstr = "";
  int id = getURLindex();
  String answer = "false";
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":1025,\"ErrorMessage\":\"" + String("Index Out of Range") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
    if(_switch.canwrite(id)) answer = "true";
    jsonretstr = "{\"Value\":" + answer + ",\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

// ----------------------------------------------------------------------
// get_notfound()
//
// ----------------------------------------------------------------------
void get_notfound()
{
  String message = "err not found ";
  String jsonretstr = "";

  message += "URI: ";
  message += _ascomserver->uri();
  message += "\nMethod: ";
  if (_ascomserver->method() == HTTP_GET)
  {
    message += "GET";
  }
  else if (_ascomserver->method() == HTTP_POST)
  {
    message += "POST";
  }
  else if (_ascomserver->method() == HTTP_PUT)
  {
    message += "PUT";
  }
  else if (_ascomserver->method() == HTTP_DELETE)
  {
    message += "DELETE";
  }
  else
  {
    message += "UNKNOWN_METHOD: " + _ascomserver->method();
  }
  message += "\nArguments: ";
  message += _ascomserver->args();
  message += "\n";
  for (uint8_t i = 0; i < _ascomserver->args(); i++)
  {
    message += " " + _ascomserver->argName(i) + ": " + _ascomserver->arg(i) + "\n";
  }

  _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
  _ASCOMErrorMessage = T_NOTIMPLEMENTED;
  _ASCOMServerTransactionID++;
  jsonretstr = "{" + addclientinfo(jsonretstr);

  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_async()
{
  String jsonretstr = "";
  int id = getURLindex();
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":1025,\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
    jsonretstr = "{\"Value\":false,\"Errornumber\":0,\"Errormessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void set_async()
{
  String jsonretstr = "";
  int id = getURLindex();
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":1025,\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
  jsonretstr = "{ \"ErrorNumber\":0, \"ErrorMessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}


void set_cancelasync()
{
  String jsonretstr = "";
    int id = getURLindex();
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":1025,\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
  jsonretstr = "{ \"ErrorNumber\":0, \"ErrorMessage\":\"\" }";
  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}

void get_statechangecomplete()
{
  String jsonretstr = "";

  _ASCOMServerTransactionID++;
  _ASCOMErrorNumber = 0;
  _ASCOMErrorMessage = "";
     int id = getURLindex();
  // Answer
  if ((id < 0)||(id >= _switch.maxswitch()))
  {
    _ASCOMErrorNumber = ASCOMNOTIMPLEMENTED;
    _ASCOMErrorMessage = T_NOTIMPLEMENTED;
    jsonretstr = "{ \"ErrorNumber\":1025,\"ErrorMessage\":\"" + String("Bad input : no negative number allowed") + "\" }";
    // sendreply builds http header, sets content type, and then sends jsonretstr
    sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
    return;
  }
  else
  // addclientinfo adds clientid, clienttransactionid, servertransactionid, errornumber, errormessage and terminating }
  jsonretstr = "{\"Value\":true,\"ErrorNumber\":0,\"ErrorMessage\":\"\" }";

  // sendreply builds http header, sets content type, and then sends jsonretstr
  sendreply(NORMALWEBPAGE, JSONPAGETYPE, jsonretstr);
}
