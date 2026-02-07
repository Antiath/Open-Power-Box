/*******************************************************************************
 * INDI Driver for the project Open Power Box
 *
 * Copyright (C) 2026 Mispelaer Florent 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * DESCRIPTION 
 * 
 * The project (software and hardware)  is available at www.github.com/Antiath/Open-Power-XXL
 * 
 * The Open Power Box is a hardware and software project aiming to provide a versatile and customizable power distribution solution for amateur astronomers. 
 * The device offers multiple 12V outputs, dew hearter outputs all controllable.
 * The hardware side of the project is fixed to 7 DC outputs, 3 dew heater outputs, 1 DC bank output (multiple gagned connectors for devices that dont require individual control), 1 relay output.
 * The hadware comes in two versions, The one descibres just above and a similar one with 7 USB2 switchable ports.
 * The project is primarily designed for large setups wwith multiple instruments ,so typically for observatory setups,
 * but the firmare is configurable to allow developpers to change those numbers at will. The driver will automatically scale itself according to those parameters.
 *  
 * FEATURES
 * - Control of all outputs (DC, Dew, USB) with individual ON/OFF commands, as well as duty cycle control for dew heaters.
 * - Real-time monitoring of voltage, current and power consumption for each DC ouputs and Dew outpouts, as well as total consumption.
 * - Hardware fuses are provided for the dew heaters but not for the DC Outputs ( to avoid unwanted voltage drops that can mess with some devices like QHY cameras).
 * - Software fuses (current limits) on all outputs, configrurable in the driver.
 * - Reboot command to reset the device.
 * - WiFi connectivity settings of the web browser interface (at http://IP_ADDRESS/4040). Driver only uses USB though.
 * - Configurable output names that can be set from the driver and are stored in the device's memory.
 * - Polarity (switch polarity , not voltage polarity) inversion for all outputs.
 * - Web browser interface handled by the device.
 * 
 *  IN DEVELOPMENT
 * - add support for automatic control of dew heaters based on temperature readings from a connected sensor (e.g. ZWO AMB temperature sensor connected to one of the DC outputs, with a custom cable that provides the necessary connections for the sensor to work and allows the driver to read the temperature data through the same USB connection used for controlling the outputs).
 ******************************************************************************/

#include "indi_opbdriver.h"

#include <memory>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sstream>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>

std::unique_ptr<OPB> opb(new OPB());

OPB::OPB() : INDI::DefaultDevice(), INDI::PowerInterface(this)
{
    setVersion(1, 0);
    initialized=false;
}


const char *OPB::getDefaultName()
{
    return "Open Power Box";
}


///////////////////////////////////////////////////////////////////////
/// Initialization of properties that don't need pior connection to the device.
/// Remark : The driver has no knowledge of the number of ports. 
/// It wil fetch this information from the device after connection, and then initialize the properties accordingly. 
//////////////////////////////////////////////////////////////////////

bool OPB::initProperties() 
{
    INDI::DefaultDevice::initProperties();
    
        // Reboot
    RebootSP[0].fill("REBOOT", "Reboot Device", ISS_OFF);
    RebootSP.fill(getDefaultName(), "REBOOT_DEVICE", "Device", MAIN_CONTROL_TAB, IP_RW, ISR_ATMOST1,60, IPS_IDLE);
    
        // Connection port
    PortTP[0].fill("PORT", "Port", "/dev/serial/by-id/usb-Silicon_Labs_CP2104_USB_to_UART_Bridge_Controller_02D7B25D-if00-port0");
    PortTP.fill(getDeviceName(), "DEVICE_PORT", "Connection", CONNECTION_TAB, IP_RW, 60, IPS_IDLE);


        // WiFi settings    
    WifiTP[0].fill("IP_ADRESS", "IP Adress", "-");
    WifiTP[1].fill("SSID", "WiFi SSID", "-");
    WifiTP[2].fill("PWD", "WiFi Password", "-");
    WifiTP.fill(getDeviceName(), "WIFI", "WiFi", CONNECTION_TAB, IP_RW, 60, IPS_IDLE);
        // Get WiFi info button   
    GetWiFiInfoSP[0].fill("GETWIFIINFO", "Get SSID + IP", ISS_OFF);
    GetWiFiInfoSP.fill(getDefaultName(), "WIFI_INFO", "Wifi info", CONNECTION_TAB, IP_RW, ISR_ATMOST1,60, IPS_IDLE);
    
        // Overall Power Consumption (Custom properties, not part of INDI::Power Interface)
    TotalConsumptionNP[INPUT_VOLTAGE].fill("INPUT_VOLTAGE", "Input Voltage (V)", "%4.2f", 0, 999, 100, 0);
    TotalConsumptionNP[TOTAL_CURRENT].fill("TOTAL_CURRENT", "Total Current (A)", "%4.2f", 0, 999, 100, 0);
    TotalConsumptionNP[TOTAL_POWER].fill("TOTAL_POWER", "Total Power (W)", "%4.2f", 0, 999, 100, 0);
    TotalConsumptionNP.fill(getDeviceName(), "POWER_CONSUMPTION", "Consumption",
                            MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);
    
        // Buttons to toggle all DC outputs and all Dew Heaters in one click
    AllDCSP[0].fill("ALL_DC_ON", "ON", ISS_ON);
    AllDCSP[1].fill("ALL_DC_OFF", "OFF", ISS_OFF);
    AllDCSP.fill(getDeviceName(), "MAIN_DC", "Toggle All DC switches", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
    defineProperty(AllDCSP);
    
    AllPWMSP[0].fill("ALL_PWM_ON", "ON", ISS_ON);
    AllPWMSP[1].fill("ALL_PWM_OFF", "OFF", ISS_OFF);
    AllPWMSP.fill(getDeviceName(), "MAIN_PWM", "Toggle All Dew Heaters", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
    defineProperty(AllPWMSP);
    
    Alldc=true;
    Allpwm=true;
    
    setDriverInterface(POWER_INTERFACE);
    
    SetCapability(POWER_HAS_DC_OUT | POWER_HAS_DEW_OUT | POWER_HAS_USB_TOGGLE | POWER_HAS_VOLTAGE_SENSOR | POWER_HAS_OVERALL_CURRENT | POWER_HAS_PER_PORT_CURRENT);
    
        // Define port property (needed before connection)
    defineProperty(PortTP);
    defineProperty(WifiTP);

    return true;
}

///////////////////////////////////////////////////////////////////////
/// Update properties after connection to the device or disconnection.
//////////////////////////////////////////////////////////////////////

bool OPB::updateProperties()
{
    INDI::DefaultDevice::updateProperties();
    if (connectedstate)
    {
        defineProperty(RebootSP);
        defineProperty(GetWiFiInfoSP);
        defineProperty(TotalConsumptionNP);
        defineProperty(AllDCSP);
        defineProperty(AllPWMSP);
        PI::updateProperties();
        SetTimer(getCurrentPollingPeriod());
    }
    else
    {
        deleteProperty(GetWiFiInfoSP);
        deleteProperty(RebootSP);
        deleteProperty(AllDCSP);
        deleteProperty(AllPWMSP);
        deleteProperty(TotalConsumptionNP);
        PI::updateProperties();
    }

    return true;
}

bool OPB::Handshake()
{
return true;
}


//////////////////////////////////////////////////////////////////////
/// Handle incoming switch changes from the client and send the corresponding commands to the device.
//////////////////////////////////////////////////////////////////////

bool OPB::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev && !strcmp(dev, getDefaultName()))
    {
    
            if (GetWiFiInfoSP.isNameMatch(name))
        {
            GetWiFiInfoSP.setState(GetWifiIP_SSID() ? IPS_OK : IPS_ALERT);
            GetWiFiInfoSP.apply();
            return true;
        }
        
            if (RebootSP.isNameMatch(name))
        {
            RebootSP.setState(reboot() ? IPS_OK : IPS_ALERT);
            RebootSP.apply();
            LOG_INFO("Rebooting device...");
            return true;
        }
    
              if (AllDCSP.isNameMatch(name))
        {   
            AllDCSP.update(states, names, n);
            bool enabled = (AllDCSP[0].getState() == ISS_ON);
            Alldc=enabled;
            if(!enabled){
            for(int i=0;i<numDC;i++){ 
            SetSwitchUSB(i, (int)enabled);
            PI::PowerChannelsSP[i].setState(((bool)(std::stoi(state[i])))? ISS_ON : ISS_OFF);
            }
            PI::PowerChannelsSP.apply();
            }
            AllDCSP.apply();
            return true;
          } 
          
            if (AllPWMSP.isNameMatch(name))
        {
            AllPWMSP.update(states, names, n);
            bool enabled = (AllPWMSP[0].getState() == ISS_ON);
            Allpwm=enabled;
            if(!enabled){
            for(int i=0;i<numPWM;i++){ 
            SetSwitchValueUSB(numDC+i, enabled ? 1.00 : 0.00);
            PI::DewChannelDutyCycleNP[i].setValue((double)(std::stoi(state[numDC+i])));
            }
            
            PI::DewChannelDutyCycleNP.apply();
            }
            AllPWMSP.apply();
            
            return true;
          } 
          
                if (OnSP.isNameMatch(name))
        {
            OnSP.update(states, names, n);
            bool enabled = (OnSP[0].getState() == ISS_ON);
            SetSwitchUSB(numDC+numPWM, (int)enabled);
            if ((bool)std::stoi(state[numDC+numPWM])==enabled)
            {
                OnSP.setState(IPS_OK);
            }
            else
            {
                OnSP.setState(IPS_ALERT);
            }
            OnSP.apply();
            return true;
          } 
          
            if (RelaySP.isNameMatch(name))
        {
            RelaySP.update(states, names, n);
            bool enabled = (RelaySP[0].getState() == ISS_ON);
            SetSwitchUSB(numDC+numPWM+numOn, (int)enabled);
            if ((bool)std::stoi(state[numDC+numPWM+numOn])==enabled)
            {
                RelaySP.setState(IPS_OK);
            }
            else
            {
                RelaySP.setState(IPS_ALERT);
            }
            RelaySP.apply();
            return true;
        }
        
           if (ReverseDCSP.isNameMatch(name))
        {
            ReverseDCSP.update(states, names, n);
            bool enabled = (ReverseDCSP[0].getState() == ISS_ON);
            SetReverseUSB(0,enabled);
            if (Reverse[0]==enabled){ ReverseDCSP.setState(IPS_OK); }
            else { ReverseDCSP.setState(IPS_ALERT); }
            ReverseDCSP.apply();
            return true;
        }
          if (ReversePWMSP.isNameMatch(name))
        {
            ReversePWMSP.update(states, names, n);
            bool enabled = (ReversePWMSP[0].getState() == ISS_ON);
            SetReverseUSB(1,enabled);
            if (Reverse[1]==enabled){ ReversePWMSP.setState(IPS_OK); }
            else { ReversePWMSP.setState(IPS_ALERT); }
            ReversePWMSP.apply();
            return true;
        }
          if (ReverseOnSP.isNameMatch(name))
        {
            ReverseOnSP.update(states, names, n);
            bool enabled = (ReverseOnSP[0].getState() == ISS_ON);
            SetReverseUSB(2,enabled);
            if (Reverse[2]==enabled){ ReverseOnSP.setState(IPS_OK); }
            else { ReverseOnSP.setState(IPS_ALERT); }
            ReverseOnSP.apply();
            return true;
        }   
          if (ReverseRelaySP.isNameMatch(name))
        {
            ReverseRelaySP.update(states, names, n);
            bool enabled = (ReverseRelaySP[0].getState() == ISS_ON);
            SetReverseUSB(3,enabled);
            if (Reverse[3]==enabled){ ReverseRelaySP.setState(IPS_OK); }
            else { ReverseRelaySP.setState(IPS_ALERT); }
            ReverseRelaySP.apply();
            return true;
        }  
          if (ReverseUSBSP.isNameMatch(name))
        {
            ReverseUSBSP.update(states, names, n);
            bool enabled = (ReverseUSBSP[0].getState() == ISS_ON);
            SetReverseUSB(4,enabled);
            if (Reverse[4]==enabled){ ReverseUSBSP.setState(IPS_OK); }
            else { ReverseUSBSP.setState(IPS_ALERT); }
            ReverseUSBSP.apply();
            return true;
        }        
        if (PI::processSwitch(dev, name, states, names, n)){
            return true;
            }
    }
    return DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

//////////////////////////////////////////////////////////////////////
/// Handle incoming text changes from the client and send the corresponding commands to the device.
//////////////////////////////////////////////////////////////////////

bool OPB::ISNewText(const char * dev, const char * name, char * texts[], char * names[], int n)
{
    if (dev && !strcmp(dev, getDeviceName()))
    {
        if (WifiTP.isNameMatch(name))
        {
            
            SetSSID(std::string(texts[1]));
            SetPWD(std::string(texts[2]));
            Transmit('p',0);
            usleep(5000000);
            GetIP();
            WifiTP[0].setText(IP.c_str());
            WifiTP[1].setText(SSID.c_str());
            WifiTP.apply();
            return true;
        }
        
        if (PowerChannelLabelsTP.isNameMatch(name))
        {
        bool changed=false;
            for(int i=0;i<numDC;i++)
            {
            if(std::string(PowerChannelLabelsTP[i].getText())!=std::string(texts[i]))
              {
              changed=true;
              SetNameUSB(i,std::string(texts[i]));
              PowerChannelLabelsTP[i].setText(name_switch[i].c_str());
              }
            }
            if(changed)PowerChannelLabelsTP.apply();
            return true;
        }
        
        if (DewChannelLabelsTP.isNameMatch(name))
        {
        bool changed2=false;
            for(int i=0;i<numPWM;i++)
            {
              if(std::string(DewChannelLabelsTP[i].getText())!=std::string(texts[i]))
              {
              changed2=true;
              SetNameUSB(numDC+i,std::string(texts[i]));
              DewChannelLabelsTP[i].setText(name_switch[numDC+i].c_str());
              }
            }
            if(changed2)DewChannelLabelsTP.apply();
            return true;
        }
        
        if (LimitsTP.isNameMatch(name))
        {
        bool changed3=false;
            for(int i=0;i<(int)(sizeof(Limit)/sizeof(Limit[0]));i++) 
            {
            if(std::string(LimitsTP[i].getText())!=std::string(texts[i]))
              {
              changed3=true;
              SetLimitsUSB(i,std::stof(std::string(texts[i])));
              LimitsTP[i].setText((std::to_string(Limit[i])).c_str());
              }
            }
            if(changed3)LimitsTP.apply();
            return true;
        }
        
        if (PI::processText(dev, name, texts, names, n))
            return true;
    }

    return DefaultDevice::ISNewText(dev, name, texts, names, n);
}

//////////////////////////////////////////////////////////////////////
/// Handle incoming number changes from the client and send the corresponding commands to the device.
//////////////////////////////////////////////////////////////////////

bool OPB::ISNewNumber(const char * dev, const char * name, double values[], char * names[], int n)
{
    if (dev && !strcmp(dev, getDeviceName()))
    {
        if (PI::processNumber(dev, name, values, names, n))
            return true;
    }
    return DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

// INDI::PowerInterface overrides
bool OPB::SetPowerPort(size_t port, bool enabled)
{
if(Alldc){
std::string s1="Changing state of ";
std::string s2=" from ";
std::string s3=" to ";
s1= s1+ std::to_string((int)port);
s1= s1+s2;
s1= s1+ state[port];
s1 = s1+s3;
s1= s1+ std::to_string((int)enabled);
LOG_INFO(s1.c_str());

    SetSwitchUSB(port, (int)enabled);
  }  
    return true;
}

bool OPB::SetDewPort(size_t port, bool enabled, double dutyCycle) 
{
if(Allpwm){
std::string s1="Setting Dew Port ";
std::string s2=" to ";
std::string s3=" with duty cycle ";
s1= s1+ std::to_string((int)port);
s1= s1+s2;
s1= s1+ std::to_string(enabled);
s1 = s1+s3;
s1= s1+ std::to_string((int)dutyCycle);
LOG_INFO(s1.c_str());
    if (enabled) SetSwitchValueUSB(numDC+port, (int)dutyCycle);
    else SetSwitchValueUSB(numDC+port, 0); //Following the indexing system set up in the firmware. Refer to the github.
}
    return true; // Assume success since we will get the real state of the outputs from the device in the next update cycle, and update the switch states accordingly.
}

bool OPB::SetUSBPort(size_t port, bool enabled)
{
    std::cout << "Setting USB Port " << port << " to " << (enabled ? "ON" : "OFF") << std::endl;
    SetSwitchUSB(numDC+numPWM+numRelay+numOn+port, (int)enabled); //Following the indexing system set up in the firmware. Refer to the github.

    return true; // Assume success since we will get the real state of the outputs from the device in the next update cycle, and update the switch states accordingly.
}


//////////////////////////////////////////////////////////////////////
/// Connect to the device, fetch device information with GetNum() and initialize properties accordingly.
//////////////////////////////////////////////////////////////////////

bool OPB::Connect()
{
    if (!openSerialPort())
    {
    connectedstate=false;
        LOG_ERROR("Failed to open serial port");
        return false;
    }
      connectedstate=true;
    LOG_INFO("Successfully connected to OPPBXXL");
    
    GetNum();
            std::string msg="Number of switches returned by the device: ";
            msg=msg.append(std::to_string(numDC));
            msg=msg.append(" DC switches + ");
            msg=msg.append(std::to_string(numPWM));
            msg=msg.append(" Dew heaters + ");
            msg=msg.append(std::to_string(numRelay));
            msg=msg.append(" Relays + ");
            msg=msg.append(std::to_string(numOn));
            msg=msg.append(" DC bank + ");
            msg=msg.append(std::to_string(numUSB));
            msg=msg.append(" USB ports");
            LOG_INFO(msg.c_str());  
    GetIP();
    GetSSID();
    
    for(int i=0;i<numDC+numPWM;i++)GetNameUSB(i);
    
    if(initialized==false)
    {
    PI::initProperties(POWER_TAB, numDC, numPWM, 0 , 0, numUSB);
    for(int i=0;i<numDC;i++)
    {
      PI::PowerChannelLabelsTP[i].setText(name_switch[i].c_str());
    }
      PI::PowerChannelLabelsTP.apply();
      
      for(int i=0;i<numPWM;i++)
    {
      PI::DewChannelLabelsTP[i].setText(name_switch[numDC+i].c_str());
    }
      PI::DewChannelLabelsTP.apply();
     
     
    for(int i=0; i < numDC;i++)
    {
    DCVEnum ena= static_cast<DCVEnum>(i);
    DCVoltageNP[ena].fill("DC_VOLTAGE_"+std::to_string(i), "Output Voltage (V) "+std::to_string(i), "%4.2f", 0, 999, 100, 0);
    DCAEnum enb= static_cast<DCAEnum>(i);
    DCCurrentNP[enb].fill("DC_CURRENT_"+std::to_string(i), "Output Current (A) "+std::to_string(i), "%4.2f", 0, 999, 100, 0);
    } 
    DCVoltageNP.fill(getDeviceName(), "DC_VOLTAGE", "DC Voltage (V)","Sensors", IP_RO, 60, IPS_IDLE);
    defineProperty(DCVoltageNP);
    DCCurrentNP.fill(getDeviceName(), "DC_CURRENT", "DC Current (A)","Sensors", IP_RO, 60, IPS_IDLE);
    defineProperty(DCCurrentNP);
    
    OnSensorNP[ON_V].fill("DC_BANK_VOLTAGE", "DC Bank Voltage (V) ", "%4.2f", 0, 999, 100, 0);
    OnSensorNP[ON_A].fill("DC_BANK_CURRENT", "DC Bank Current (A) ", "%4.2f", 0, 999, 100, 0);
    OnSensorNP.fill(getDeviceName(), "DC_BANK", "DC Bank Consumption","Sensors", IP_RO, 60, IPS_IDLE);
    defineProperty(OnSensorNP);
    
    
    for(int i=0; i < numPWM;i++)
    {
    PWMAEnum enc= static_cast<PWMAEnum>(i);
    PWMCurrentNP[enc].fill("PWM_CURRENT_"+std::to_string(i), "Output Current (A) "+std::to_string(i), "%4.2f", 0, 999, 100, 0);
    } 
    PWMCurrentNP.fill(getDeviceName(), "PWM_CURRENT", "Dew heater Current (A)","Sensors", IP_RO, 60, IPS_IDLE);
    defineProperty(PWMCurrentNP);
    
    LimitsTP[0].fill("DC_LIMIT_INDIV", "DC Limit (A)", "-");
    LimitsTP[1].fill("PWM_LIMIT_INDIV", "PWM Limit (A)", "-");
    LimitsTP[2].fill("DC_BANK_LIMIT", "DC Bank Limit (A)", "-");
    LimitsTP[3].fill("DC_LIMIT_TOTAL", "Total DC Limit (A)", "-");
    LimitsTP[4].fill("PWM_LIMIT_TOTAL", "Total PWM Limit (A)", "-");
    LimitsTP[5].fill("GLOBAL_LIMIT_TOTAL", "Global Limit (A)", "-");
    LimitsTP.fill(getDeviceName(), "LIMITS", "Limits", "Configuration", IP_RW, 60, IPS_IDLE); 
    defineProperty(LimitsTP);
    for(int i=0;i<(int)(sizeof(Limit)/sizeof(Limit[0]));i++) 
    {
      GetLimitsUSB(i);
      LimitsTP[i].setText((std::to_string(Limit[i])).c_str());
    }
    LimitsTP.apply();
    
    for(int i=0;i<(int)(sizeof(Reverse)/sizeof(Reverse[0]));i++)  
    {
    GetReverseUSB(i);
    }
    
    if(numRelay==1){
      RelaySP[0].fill("RELAY_ON", "Enabled", ISS_ON);
      RelaySP[1].fill("RELAY_OFF", "Disabled", ISS_OFF);
      RelaySP.fill(getDeviceName(), "RELAY_EN", "Relay", "Power", IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
      defineProperty(RelaySP);
    }
    if(numOn==1){
      OnSP[0].fill("ON_ON", "Enabled", ISS_ON);
      OnSP[1].fill("ON_OFF", "Disabled", ISS_OFF);
      OnSP.fill(getDeviceName(), "ON_EN", "DC Bank", "Power", IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
      defineProperty(OnSP);
    }
    
    if(numDC>0){
    ReverseDCSP[0].fill("DC_POLARITY_ON", "Normal", ISS_ON);
    ReverseDCSP[1].fill("DC_POLARITY_OFF", "Inverted", ISS_OFF);
    ReverseDCSP.fill(getDeviceName(), "P0LARITIES_DC", "DC Polarity", "Configuration", IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
    defineProperty(ReverseDCSP);
    ReverseDCSP[0].setState(Reverse[0]? ISS_ON : ISS_OFF);
    ReverseDCSP[1].setState(Reverse[0]? ISS_OFF : ISS_ON);
    ReverseDCSP.apply();
    }
    if(numPWM>0){
    ReversePWMSP[0].fill("PWM_POLARITY_ON", "Normal", ISS_ON);
    ReversePWMSP[1].fill("PWM_POLARITY_OFF", "Inverted", ISS_OFF);
    ReversePWMSP.fill(getDeviceName(), "P0LARITIES_PWM", "PWM Polarity", "Configuration", IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
    defineProperty(ReversePWMSP);
    ReversePWMSP[0].setState(Reverse[1]? ISS_ON : ISS_OFF);
    ReversePWMSP[1].setState(Reverse[1]? ISS_OFF : ISS_ON);
    ReversePWMSP.apply();
    }
    if(numOn>0){
    ReverseOnSP[0].fill("BANK_POLARITY_ON", "Normal", ISS_ON);
    ReverseOnSP[1].fill("BANK_POLARITY_OFF", "Inverted", ISS_OFF);
    ReverseOnSP.fill(getDeviceName(), "P0LARITIES_BANK", "Bank Polarity", "Configuration", IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
    defineProperty(ReverseOnSP);
    ReverseOnSP[0].setState(Reverse[2]? ISS_ON : ISS_OFF);
    ReverseOnSP[1].setState(Reverse[2]? ISS_OFF : ISS_ON);
    ReverseOnSP.apply();
    }
    if(numRelay>0){
    ReverseRelaySP[0].fill("RELAY_POLARITY_ON", "Normal", ISS_ON);
    ReverseRelaySP[1].fill("RELAY_POLARITY_OFF", "Inverted", ISS_OFF);
    ReverseRelaySP.fill(getDeviceName(), "P0LARITIES_RELAY", "Relay Polarity", "Configuration", IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
    defineProperty(ReverseRelaySP);
    ReverseRelaySP[0].setState(Reverse[3]? ISS_ON : ISS_OFF);
    ReverseRelaySP[1].setState(Reverse[3]? ISS_OFF : ISS_ON);
    ReverseRelaySP.apply();
    }
    if(numUSB>0){
    ReverseUSBSP[0].fill("USB_POLARITY_ON", "Normal", ISS_ON);
    ReverseUSBSP[1].fill("USB_POLARITY_OFF", "Inverted", ISS_OFF);
    ReverseUSBSP.fill(getDeviceName(), "P0LARITIES_USB", " Polarity", "Configuration", IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
    defineProperty(ReverseUSBSP);
    ReverseUSBSP[0].setState(Reverse[4]? ISS_ON : ISS_OFF);
    ReverseUSBSP[1].setState(Reverse[4]? ISS_OFF : ISS_ON);
    ReverseUSBSP.apply();
    }
    initialized=true;
    }
    else
    {
    defineProperty(LimitsTP);
    defineProperty(ReverseDCSP);
    defineProperty(ReversePWMSP);
    defineProperty(ReverseOnSP);
    defineProperty(ReverseRelaySP);
    defineProperty(ReverseUSBSP);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////
/// Disconnect from the device and clean up properties.
//////////////////////////////////////////////////////////////////////

bool OPB::Disconnect()
{
    closeSerialPort();
    if(numOn>0)deleteProperty(OnSP);
    if(numRelay>0)deleteProperty(RelaySP);
    
    deleteProperty(LimitsTP);
    if(numRelay>0) deleteProperty(ReverseRelaySP);
    if(numUSB>0) deleteProperty(ReverseUSBSP);
    if(numOn>0) deleteProperty(ReverseOnSP);
    if(numDC>0) deleteProperty(ReverseDCSP);
    if(numPWM>0) deleteProperty(ReversePWMSP);
    
    if(numDC>0)deleteProperty(DCVoltageNP);
    if(numDC>0)deleteProperty(DCCurrentNP);
    if(numOn>0)deleteProperty(OnSensorNP);
    if(numPWM>0)deleteProperty(PWMCurrentNP);
    
    
    connectedstate=false;
    LOG_INFO("Disconnected from OPPBXXL");
    return true;
}

//////////////////////////////////////////////////////////////////////
/// Low-level serial port handling. Taken from the SV241 driver from Pascal Watteel and adapted to our needs.
//////////////////////////////////////////////////////////////////////

bool OPB::openSerialPort()
{
    const char *portName = PortTP[0].getText();

    // Open port with O_NOCTTY to prevent it from becoming controlling terminal
    PortFD = open(portName, O_RDWR | O_NOCTTY);
    if (PortFD < 0)
    {
        LOGF_ERROR("Error opening serial port %s: %s", portName, strerror(errno));
        return false;
    }

    // CRITICAL: Set DTR and RTS LOW immediately to prevent ESP32 reset
    // ESP32 boards with CH340/CP2102 use DTR+RTS for auto-reset during programming.
    // Opening the serial port can cause DTR to pulse HIGH, resetting the device.
    int modemBits = 0;
    ioctl(PortFD, TIOCMGET, &modemBits);
    modemBits &= ~TIOCM_DTR;  // Clear DTR (set LOW)
    modemBits &= ~TIOCM_RTS;  // Clear RTS (set LOW)
    ioctl(PortFD, TIOCMSET, &modemBits);

    // Clear non-blocking mode (ensure blocking reads with timeout)
    fcntl(PortFD, F_SETFL, 0);

    // Configure serial port: 115200 8N1
    struct termios options;
    tcgetattr(PortFD, &options);

    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cflag &= ~PARENB;  // No parity
    options.c_cflag &= ~CSTOPB;  // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;       // 8 data bits
    options.c_cflag &= ~HUPCL;    // IMPORTANT: Disable HUPCL to prevent DTR drop on close
    options.c_cflag |= CLOCAL;    // Ignore modem control lines
    options.c_cflag |= CREAD;     // Enable receiver
    options.c_cflag &= ~CRTSCTS;  // Disable hardware flow control

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  // Raw input
    options.c_iflag &= ~(IXON | IXOFF | IXANY);          // No software flow control
    options.c_oflag &= ~OPOST;                            // Raw output

    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10;  // 1 second timeout

    tcsetattr(PortFD, TCSANOW, &options);
    tcflush(PortFD, TCIOFLUSH);

    // Ensure DTR/RTS stay LOW after termios configuration
    ioctl(PortFD, TIOCMGET, &modemBits);
    modemBits &= ~TIOCM_DTR;
    modemBits &= ~TIOCM_RTS;
    ioctl(PortFD, TIOCMSET, &modemBits);

    // Allow USB serial device to stabilize
    usleep(500000); 
    
    GetNum();

    LOGF_INFO("Opened serial port %s at 115200 baud (DTR/RTS held LOW)", portName);
    return true;
}

void OPB::closeSerialPort()
{
    if (PortFD >= 0)
    {
        close(PortFD);
        PortFD = -1;
    }
} 

//////////////////////////////////////////////////////////////////////
/// Handling reception of messages from the device that are terminated with a specific character (e.g. ';')
//////////////////////////////////////////////////////////////////////

std::string OPB::ReceiveTerminated(char terminator)
{
        int nbytes_read = 0, tty_rc = 0;
        char res[100] = {0};
      

        if ((tty_rc = tty_read_section(PortFD, res, terminator, 2, &nbytes_read)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial read error: %s", errorMessage);
            return "";
        }
        char ans[nbytes_read];
        for(int i=0;i<nbytes_read;i++)
        {
            ans[i]=res[i];
        }
        std::string answer= ans;
        int i=answer.find('#');
        answer=answer.substr(i); 
        return answer;
}

//////////////////////////////////////////////////////////////////////
/// Multiple overloads of the Transmit function to send commands to the device in a consistent format.
//////////////////////////////////////////////////////////////////////

bool  OPB::Transmit(char command, int SwitchNum, int value)
{
    int nbytes_written = 0, tty_rc = 0;

    std::string packet;
    packet='#';
    packet= packet.append(1,' ');
    packet= packet.append(1,command);
    packet= packet.append(1,' ');
    packet= packet.append(std::to_string(SwitchNum));
    packet= packet.append(1,' ');
    packet= packet.append(std::to_string(value));
    packet= packet.append(1,'\n');
    const char* cmd=packet.c_str();
    LOG_INFO(cmd);
    LOGF_DEBUG("CMD <%s>", cmd);
    
        if ((tty_rc = tty_write_string(PortFD, cmd, &nbytes_written)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial write error: %s", errorMessage);
            return false;
        }

    usleep(CMD_DELAY);

    return true;
}

bool  OPB::Transmit(char command, int SwitchNum, float value)
{
    int nbytes_written = 0, tty_rc = 0;

    std::string packet;
    packet='#';
    packet= packet.append(1,' ');
    packet= packet.append(1,command);
    packet= packet.append(1,' ');
    packet= packet.append(std::to_string(SwitchNum));
    packet= packet.append(1,' ');
    packet= packet.append(std::to_string(value));
    packet= packet.append(1,'\n');
    const char* cmd=packet.c_str();
    LOG_INFO(cmd);
    LOGF_DEBUG("CMD <%s>", cmd);

            tcflush(PortFD, TCIOFLUSH);
        if ((tty_rc = tty_write_string(PortFD, cmd, &nbytes_written)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial write error: %s", errorMessage);
            return false;
        }

    usleep(CMD_DELAY);

    return true;
}

bool  OPB::Transmit(char command, int SwitchNum)
{
    int nbytes_written = 0, tty_rc = 0;

    std::string packet;
    packet='#';
    packet= packet.append(1,' ');
    packet= packet.append(1,command);
    packet= packet.append(1,' ');
    packet= packet.append(std::to_string(SwitchNum));
    packet= packet.append(1,'\n');
    const char* cmd=packet.c_str();
    LOGF_DEBUG("CMD <%s>", cmd);

            tcflush(PortFD, TCIOFLUSH);
        if ((tty_rc = tty_write_string(PortFD, cmd, &nbytes_written)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial write error: %s", errorMessage);
            return false;
        }

     usleep(CMD_DELAY);

    return true;
}

bool  OPB::Transmit(char command, int SwitchNum, std::string value)
{
    int nbytes_written = 0, tty_rc = 0;

    std::string packet;
    packet='#';
    packet= packet.append(1,' ');
    packet= packet.append(1,command);
    packet= packet.append(1,' ');
    packet= packet.append(std::to_string(SwitchNum));
    packet= packet.append(1,' ');
    packet= packet.append(value);
    packet= packet.append(1,'\n');
    const char* cmd=packet.c_str();
    LOGF_DEBUG("CMD <%s>", cmd);

            tcflush(PortFD, TCIOFLUSH);
        if ((tty_rc = tty_write_string(PortFD, cmd, &nbytes_written)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial write error: %s", errorMessage);
            return false;
        }

     usleep(CMD_DELAY);

    return true;
}

//////////////////////////////////////////////////////////////////////
/// Sends a command to change the state of a switch with a non-boolean parameter ( typically for dew heaters), 
/// then waits for the device to acknowledge the new state and updates the internal state accordingly. 
/// If the device returns an error or doesn't acknowledge the new state, it reverts the internal state and logs an error message.
//////////////////////////////////////////////////////////////////////
        
                   void OPB::SetSwitchValueUSB(short id, double value)
        {
            int switchN;
            std::string prev;
            std::string answer = "";
            prev=state[id];
            Transmit('S',id,(int)value);
            state[id]=std::to_string(value);
            std::string buf = ReceiveTerminated(';');
            buf=buf.erase(0,1);
            buf=buf.erase(buf.length()-1);
            int i=buf.find(':');
            switchN = short(std::stoi(buf.substr(1,i-1)));
            if (buf[0] == 'E')
            {
            std::string msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'G') && (switchN == id))
            {
            buf=buf.substr(i+1);
            if(std::stoi(state[id])==std::stoi(buf))return;
            else 
              {
              state[id]=prev;
              LOG_INFO("ERROR : The power box didn't acknowledge the new state.");
              }
            } 
            return;
        }
        
 //////////////////////////////////////////////////////////////////////
/// Sends a command to change the state of a switch with a boolean parameter (typically for the normal switches), 
/// then waits for the device to acknowledge the new state and updates the internal state accordingly. 
/// If the device returns an error or doesn't acknowledge the new state, it reverts the internal state and logs an error message.
//////////////////////////////////////////////////////////////////////
 
        void OPB::SetSwitchUSB(short id, int value)
        {
            int switchN;
            std::string prev;
            std::string answer = "";
            prev=state[id];
            Transmit('S',id,value);
            state[id]=std::to_string(value);
            std::string buf = ReceiveTerminated(';');
            buf=buf.erase(0,1);
            buf=buf.erase(buf.length()-1);
            int i=buf.find(':');
            switchN = short(std::stoi(buf.substr(1,i-1)));
            if (buf[0] == 'E')
            {
            std::string msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'G') && (switchN == id))
            {
            buf=buf.substr(i+1);
            if(state[id][0]==buf[0])return;
            else 
              {
              state[id]=prev;
              LOG_INFO("ERROR : The power box didn't acknowledge the new state.");
              }
            } 
            return;
        }

//////////////////////////////////////////////////////////////////////
/// Sends a command to request the state of a switch, a dew heater, or any sensor from the device, 
/// waits for the response, and updates the internal state accordingly.
/// Note on the naming: On/off switches, dew heaters AND sensors are all "switches" in the firmware, following the ASCOM convention on those devices. 
/// thus, GetSwitchUSB is used to get the state of any of those.
//////////////////////////////////////////////////////////////////////       
        
             void OPB::GetSwitchUSB(short id)
        {
            int switchN;
            std::string answer = "";

            Transmit('G',id);
            std::string buf = ReceiveTerminated(';');
            buf=buf.erase(0,1);
            buf=buf.erase(buf.length()-1);
            int i=buf.find(':');
            switchN = short(std::stoi(buf.substr(1,i-1)));
            
            if (buf[0] == 'E')
            {
            std::string msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'G') && (switchN == id))
            {
            int j=buf.find(';');
            if(j!=(int)std::string::npos){
            buf=buf.erase(j);
            state[id]=buf.substr(i+1);   
            }
            }
            return;
        }

//////////////////////////////////////////////////////////////////////
/// Sends a command to request the name of a switch from the device.
//////////////////////////////////////////////////////////////////////   

              void OPB::GetNameUSB(short id)
        {
            int switchN;
            std::string answer = "";

            Transmit('n',id);
            std::string buf = ReceiveTerminated(';');
            buf=buf.erase(0,1);
            buf=buf.erase(buf.length()-1);
            int i=buf.find(':');
            switchN = short(std::stoi(buf.substr(1,i-1)));
            
            if (buf[0] == 'E')
            {
            std::string msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'n') && (switchN == id))
            {
            int j=buf.find(';');
            if(j!=(int)std::string::npos){
            buf=buf.erase(j);
            name_switch[id]=buf.substr(i+1);   
            }
            }
            return;
        }

//////////////////////////////////////////////////////////////////////
/// Sends a command to change the name of a switch.
//////////////////////////////////////////////////////////////////////          
        
                      void OPB::SetNameUSB(short id, std::string name)
        {
            int switchN;
            std::string answer = "";

            Transmit('N',id, name);
            
            std::string buf = ReceiveTerminated(';');
            LOG_INFO(buf.c_str());
            buf=buf.erase(0,1);
            int i=buf.find(':');
            switchN = short(std::stoi(buf.substr(1,i-1)));
            
            if (buf[0] == 'E')
            {
            std::string msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'n') && (switchN == id))
            {
            LOG_INFO(buf.c_str());
            int j=buf.find(';');
            if(j!=(int)std::string::npos){
            buf=buf.erase(j);
            LOG_INFO(buf.c_str());
            name_switch[id]=buf.substr(i+1); 
            LOG_INFO(name_switch[id].c_str());
            }
            }
            return;
        }


//////////////////////////////////////////////////////////////////////
/// Sends a command to request the number of switches, dew heaters, relays, DC banks, and USB ports from the device.
////////////////////////////////////////////////////////////////////// 

             void OPB::GetNum()
        {
            std::string answer = "";
            std::string msg;
            Transmit('Z',0);
            std::string buf = ReceiveTerminated(';');
            buf=buf.erase(0,1);
            int i=buf.find(':');

            if (buf[0] == 'E')
            {
            msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'Z'))
            {
            int j=buf.find(';');
            if(j!=(int)std::string::npos){
            buf=buf.erase(j);
            buf=buf.substr(i+1);

            int k=buf.find_last_of(',');
            numUSB=std::stoi(buf.substr(k+1));
 
            buf=buf.erase(k);
            k=buf.find_last_of(',');
            numOn=std::stoi(buf.substr(k+1));

            buf=buf.erase(k);
            k=buf.find_last_of(',');
            numRelay=std::stoi(buf.substr(k+1));

            buf=buf.erase(k);
            k=buf.find_last_of(',');
            numPWM=std::stoi(buf.substr(k+1));

            buf=buf.erase(k);
            numDC=std::stoi(buf);
        
            }
            }
            return;
        }

//////////////////////////////////////////////////////////////////////
/// Sends commands to request the WiFi IP address and SSID from the device.
//////////////////////////////////////////////////////////////////////        
        
           bool OPB::GetWifiIP_SSID()
        {
        GetIP();
        GetSSID();
            return true;
        }

 //////////////////////////////////////////////////////////////////////
/// Sends a command to request the WiFi IP address from the device.
//////////////////////////////////////////////////////////////////////         
        
          void OPB::GetIP()
        {
            std::string answer = "";
            std::string msg;
            Transmit('I',0);
            std::string buf = ReceiveTerminated(';');
            buf=buf.erase(0,1);
            int i=buf.find(':');

            if (buf[0] == 'E')
            {
            msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'i'))
            {
            int j=buf.find(';');
            if(j!=(int)std::string::npos){
            buf=buf.erase(j);
            buf=buf.substr(i+1);

            IP=buf;
            WifiTP[0].setText(IP.c_str());
            WifiTP.apply();
            msg="IP Address returned by the device: ";
            msg=msg.append(IP);
   
            LOG_INFO(msg.c_str());  
            }
            }
            return;
        }

//////////////////////////////////////////////////////////////////////
/// Sends a command to request the WiFi SSID from the device.
//////////////////////////////////////////////////////////////////////         
        
            void OPB::GetSSID()
        {
            std::string answer = "";
            std::string msg;
            Transmit('f',0);
            std::string buf = ReceiveTerminated(';');
            buf=buf.erase(0,1);
            int i=buf.find(':');

            if (buf[0] == 'E')
            {
            msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'f'))
            {
            int j=buf.find(';');
            if(j!=(int)std::string::npos){
            buf=buf.erase(j);
            buf=buf.substr(i+1);
            
            SSID=buf;
            WifiTP[1].setText(SSID.c_str());
            WifiTP.apply();
            msg="SSID returned by the device: ";
            msg=msg.append(SSID);
   
            LOG_INFO(msg.c_str());  
            }
            }
            return;
        }

//////////////////////////////////////////////////////////////////////
/// Sends a command to set the WiFi SSID from the device.
//////////////////////////////////////////////////////////////////////          
        
             void OPB::SetSSID(std::string ssid)
        {
            std::string answer = "";
            std::string msg;
            Transmit('F',0,ssid);
            std::string buf = ReceiveTerminated(';');
            buf=buf.erase(0,1);
            int i=buf.find(':');

            if (buf[0] == 'E')
            {
            msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'f'))
            {
            int j=buf.find(';');
            if(j!=(int)std::string::npos){
            buf=buf.erase(j);
            buf=buf.substr(i+1);
            
            SSID=buf;
            WifiTP[1].setText(SSID.c_str());
            WifiTP.apply();
            msg="SSID returned by the device: ";
            msg=msg.append(SSID);
   
            LOG_INFO(msg.c_str());  
            }
            }
            return;
        }

 //////////////////////////////////////////////////////////////////////
/// Sends a command to set the WiFi password of the device. No response expected.
//////////////////////////////////////////////////////////////////////         
        
               void OPB::SetPWD(std::string pwd)
        {
            Transmit('H',0,pwd);
            return;
        }
        

//////////////////////////////////////////////////////////////////////
/// Sends a command to request the state of the reverse polarity setting 
//////////////////////////////////////////////////////////////////////

        void OPB::GetReverseUSB(short id)  
        {
                    int switchN;
            std::string answer = "";

            Transmit('r',id);
            std::string buf = ReceiveTerminated(';');
            buf=buf.erase(0,1);
            int i=buf.find(':');
            switchN = short(std::stoi(buf.substr(1,i-1)));
            if (buf[0] == 'E')
            {
            std::string msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'r') && (switchN == id))
            {
            int j=buf.find(';');
            if(j!=(int)std::string::npos){
            buf=buf.erase(j);
            bool b=(bool)std::stoi(buf.substr(i+1));
            Reverse[id]=b;  
            }
            }
            return;
            } 

//////////////////////////////////////////////////////////////////////
/// Sends a command to set the state of the reverse polarity setting of a switch.
//////////////////////////////////////////////////////////////////////           
            
                  void OPB::SetReverseUSB(short id, int value )
        {
                    int switchN;
            std::string answer = "";

            Transmit('R',id,value);
            std::string buf = ReceiveTerminated(';');
            buf=buf.erase(0,1);
            //buf=buf.erase(buf.length()-1);
            int i=buf.find(':');
            switchN = short(std::stoi(buf.substr(1,i-1)));
            if (buf[0] == 'E')
            {
            std::string msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'r') && (switchN == id))
            {
            int j=buf.find(';');
            if(j!=(int)std::string::npos){
            buf=buf.erase(j);
            bool b=(bool)std::stoi(buf.substr(i+1));
            Reverse[id]=b;  
            }
            }
            return;
            } 

//////////////////////////////////////////////////////////////////////
/// Sends a command to request the current limit of a switch from the device.
//////////////////////////////////////////////////////////////////////              

        void OPB::GetLimitsUSB(short id)
                {
                    int switchN;
            std::string answer = "";

            Transmit('l',id);
            std::string buf = ReceiveTerminated(';');
            //LOG_INFO(buf.c_str());
            buf=buf.erase(0,1);
            buf=buf.erase(buf.length()-1);
            int i=buf.find(':');
            switchN = short(std::stoi(buf.substr(1,i-1)));
            
            if (buf[0] == 'E')
            {
            std::string msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'l') && (switchN == id))
            {
            int j=buf.find(';');
            if(j!=(int)std::string::npos){
            buf=buf.erase(j);
            float b=std::stof(buf.substr(i+1));
            Limit[id]=b;
            }
            }
            return;
            }
            
 //////////////////////////////////////////////////////////////////////
/// Sends a command to set the current limit of a switch from the device.
//////////////////////////////////////////////////////////////////////  

        void OPB::SetLimitsUSB(short id, float value)        
                {
                    int switchN;
            std::string answer = "";

            Transmit('L',id,value);
            std::string buf = ReceiveTerminated(';');
            //LOG_INFO(buf.c_str());
            buf=buf.erase(0,1);
            buf=buf.erase(buf.length()-1);
            int i=buf.find(':');
            switchN = short(std::stoi(buf.substr(1,i-1)));
            
            if (buf[0] == 'E')
            {
            std::string msg="The power box returned error: ";
            msg=msg.append(buf);
            LOG_INFO(msg.c_str());
            return;
            }
            else if ((buf[0] == 'l') && (switchN == id))
            {
            int j=buf.find(';');
            if(j!=(int)std::string::npos){
            buf=buf.erase(j);
            float b=std::stof(buf.substr(i+1));
            Limit[id]=b;
            }
            }
            return;
            }
 
//////////////////////////////////////////////////////////////////////
/// Handles the timerhit event, which is triggered periodically to poll the device for the state of switches, dew heaters and sensors.
//////////////////////////////////////////////////////////////////////            
            
void OPB::TimerHit()
{
    if (!isConnected() )
    {
        SetTimer(getCurrentPollingPeriod());
        return;
    }

    int total = numDC + numPWM + numOn + numRelay + numUSB;
    int sensorNum= total +4;
    int numSwitch = (numDC + numPWM + numOn)*2+ total +4;
    
    int numInputV = numDC + numPWM + numOn + numRelay;
    int numTotalA = numDC + numPWM + numOn + numRelay+1;

    
    for(int k=0;k<numSwitch;k++)  GetSwitchUSB(k);

    float v=std::stof(state[numInputV]);
    float a=std::stof(state[numTotalA]);
    
    PI::PowerSensorsNP[PI::SENSOR_VOLTAGE].setValue(v); 
    PI::PowerSensorsNP[PI::SENSOR_CURRENT].setValue(a); 
    PI::PowerSensorsNP[PI::SENSOR_POWER].setValue(v*a);     
    PI::PowerSensorsNP.setState(IPS_OK);
    PI::PowerSensorsNP.apply();
    
    
    for(int i=0;i<numDC;i++)PI::PowerChannelsSP[i].setState(((bool)(std::stoi(state[i])))? ISS_ON : ISS_OFF);
    PI::PowerChannelsSP.apply();
    for(int j=0;j<numPWM;j++)PI::DewChannelDutyCycleNP[j].setValue((double)(std::stoi(state[numDC+j])));
    PI::DewChannelDutyCycleNP.apply();
    
    for(int i=0; i < numDC;i++)
    {
    DCVEnum ena= static_cast<DCVEnum>(i);
    DCAEnum enb= static_cast<DCAEnum>(i);
    DCVoltageNP[ena].setValue(std::stof(state[sensorNum+2*i]));
    DCCurrentNP[enb].setValue(std::stof(state[sensorNum+2*i+1]));
    PowerChannelCurrentNP[i].setValue(std::stof(state[sensorNum+2*i+1]));
    } 
    DCVoltageNP.apply();
    DCCurrentNP.apply();
    PowerChannelCurrentNP.apply();
    
    for(int i=0; i < numPWM;i++)
    {
    PWMAEnum enc= static_cast<PWMAEnum>(i);
    PWMCurrentNP[enc].setValue(std::stof(state[sensorNum+2*numDC+2*i+1]));
    } 
    PWMCurrentNP.apply();
    DewChannelCurrentNP.apply();
    
    OnSensorNP[ON_V].setValue(std::stof(state[sensorNum+2*numDC+2*numPWM]));
    OnSensorNP[ON_A].setValue(std::stof(state[sensorNum+2*numDC+2*numPWM+1]));
    OnSensorNP.apply();
    
    TotalConsumptionNP[INPUT_VOLTAGE].setValue(v);
    TotalConsumptionNP[TOTAL_CURRENT].setValue(a);
    TotalConsumptionNP[TOTAL_POWER].setValue(v*a);
    TotalConsumptionNP.setState(IPS_OK);
    TotalConsumptionNP.apply();
    
    SetTimer(getCurrentPollingPeriod());
}

//////////////////////////////////////////////////////////////////////
/// Sends a command to reboot the device. No response expected. It will cause the driver to "crash" on the next update cycle and restart.
////////////////////////////////////////////////////////////////////// 

bool OPB::reboot()
{
    Transmit('p',0);
    return true;
}
