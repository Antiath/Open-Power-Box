/*******************************************************************************
 * INDI Driver for the project Open Power Box
 *
 * Copyright (C) 2026 Mispelaer Florent 
 *
 * The project (software and hardware)  is available at www.github.com/Antiath/Open-Power-XXL
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
 ******************************************************************************/

#pragma once

#include "defaultdevice.h"
#include "indipowerinterface.h"

#include <cstdint>
#include <cstring>
#include <poll.h>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

class OPB : public INDI::DefaultDevice, public INDI::PowerInterface
{
    public:
        OPB();
        virtual bool initProperties() override;
        virtual bool updateProperties() override;
        virtual void TimerHit() override;
        virtual bool Handshake();
         //virtual bool Handshake();
        
        // INDI::PowerInterface overrides
        virtual bool SetPowerPort(size_t port, bool enabled) override;
        virtual bool SetDewPort(size_t port, bool enabled, double dutyCycle)  override;
        virtual bool SetUSBPort(size_t port, bool enabled) override;
        
    protected:
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool ISNewText(const char * dev, const char * name, char * texts[], char * names[], int n) override;
    virtual bool ISNewNumber(const char * dev, const char * name, double values[], char * names[], int n) override;
    
        bool Connect() override;
        bool Disconnect() override;
        const char *getDefaultName() override;
    private:
        int PortFD{-1};
        bool openSerialPort();
        void closeSerialPort();
        INDI::PropertyText PortTP{1};
        std::string ReceiveTerminated(char terminator);
        bool Transmit(char command, int SwitchNum, int value);
        bool Transmit(char command, int SwitchNum, float value);
        bool Transmit(char command, int SwitchNum);
        bool Transmit(char command, int SwitchNum, std::string value);
        void GetSwitchUSB(short id);
        void SetSwitchValueUSB(short id, double value);
        void SetSwitchUSB(short id, int value);
        void GetNameUSB(short id);     
        void SetNameUSB(short id, std::string name);
        void GetReverseUSB(short id);  
        void SetReverseUSB(short id, int value);
        void GetLimitsUSB(short id);  
        void SetLimitsUSB(short id, float value);        
        void GetNum();
        bool GetWifiIP_SSID();
        void GetIP();
        void GetSSID();
        void SetSSID(std::string ssid);
        void SetPWD(std::string ppwd);
        bool reboot();
        
        bool connectedstate,initialized;

        std::string state[100];
        std::string name_switch[100];
        bool Reverse[5];//{DC,PWM,On,Relay,USB}
        float Limit[6];//{DC,PWM,On,TotalDC,TotalPWM,Total}
        static constexpr int CMD_DELAY = 100000;
        static constexpr int READ_TIMEOUT = 3000;
        int numDC,numPWM,numRelay,numOn,numUSB;
        std::string IP,SSID,PWD;
        
        
        INDI::PropertyText WifiTP{3};
        INDI::PropertySwitch GetWiFiInfoSP{1};
        INDI::PropertySwitch RebootSP{1};
        
        INDI::PropertySwitch RelaySP{2};
        INDI::PropertySwitch OnSP{2};
        
        INDI::PropertyText LimitsTP{6};
        INDI::PropertySwitch ReverseDCSP{2};
        INDI::PropertySwitch ReversePWMSP{2};
        INDI::PropertySwitch ReverseOnSP{2};
        INDI::PropertySwitch ReverseRelaySP{2};
        INDI::PropertySwitch ReverseUSBSP{2};
        
        INDI::PropertySwitch AllDCSP {2};
        INDI::PropertySwitch AllPWMSP {2};
        bool Alldc,Allpwm;


        INDI::PropertyNumber TotalConsumptionNP {3};
        enum
        {
            INPUT_VOLTAGE,
            TOTAL_CURRENT,
            TOTAL_POWER
        };
        
        INDI::PropertyNumber BankConsumptionNP {2};
        enum
        {
            TOTAL_DC_CURRENT,
            TOTAL_PWM_CURRENT
        };
        
        INDI::PropertyNumber DCVoltageNP {10};
        enum DCVEnum{DC_V_1, DC_V_2, DC_V_3, DC_V_4, DC_V_5, DC_V_6, DC_V_7, DC_V_8, DC_V_9, DC_V_10};
        INDI::PropertyNumber DCCurrentNP {10};
        enum DCAEnum{DC_A_1, DC_A_2, DC_A_3, DC_A_4, DC_A_5, DC_A_6, DC_A_7, DC_A_8, DC_A_9, DC_A_10};
        INDI::PropertyNumber PWMCurrentNP {10};
        enum PWMAEnum{PWM_A_1, PWM_A_2, PWM_A_3, PWM_A_4, PWM_A_5, PWM_A_6, PWM_A_7, PWM_A_8, PWM_A_9, PWM_A_10};
        INDI::PropertyNumber OnSensorNP {2};
        enum{ON_V,ON_A};

        
};

