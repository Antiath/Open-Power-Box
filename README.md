# 🔭 Open Power Box: Heavy Duty Power Distribution System for Astrophotography Equipment
**Open Power Box** is a complete, open-source hardware and software project designed to build a robust power distribution box specifically for an **observatory-class amateur astronomy setup**. While many similar projects exist, they often target compact, portable systems; this project is built for greater capabilities. The hardware side of the project is designed following the specifications detailled below but the firmware, though preconfigured for this specific hardware, has been written to allow scalability to an arbitrary number of outputs so that one could use it as-is for his custom version of the project, without minamal very adjustments to the firmware. 

**LICENSING** : The entire project is **completely open source**. The hardware files and documentation fall under the **Licence CC BY-SA 4.0**, while the drivers and the firmware are licensed under **GNU Lesser General Public License version 3 (LGPLv3)** 

<img src="images/PowerBox2_2_KICAD.jpg" width="800">

## ✨ Key Features

### Power and Output Specifications
* **Input:** 12V (nominal, unregulated) DC with a **20A limit**. The polarity protection circuit can handel up to 20V without damaging it but we really recommand to stay around 12V as there is no voltage regulation on the output.
* **Switched DC Outputs:** 7 individual ON/OFF DC switches, rated at **12V/5A each**.
* **Dew Heater Outputs:** 3 outputs with low-frequency PWM control, rated at **12V/2A each**.
* **Automatic Dew Heater Control** Using a SHT31 sensor on the i2c bus, the device can regulate the power of a heater band.
* **12V Rail:** A single rail of 4 DC outputs for devices that don't need individual control, limited to **12V/5A for the whole rail**. Useful for things that stay on all the time for exemple.
* **Switchable Relay:** A relay useful for controlling a device with specific voltage requirements, max **30V-5A**.
* **Switchable USB Hub (Optional):** A 7-port switchable USB2 hub (on a second PCB). This allows for remote USB connection reset if a peripheral malfunctions, but a simple off-the-shelf hub can also be used if this feature is not needed.

### Protection and Customization
* **Configurable Software Fuses:** Most switches have a dedicated current sensor to monitor current draw and shut down the switch if a configurable limit is exceeded. These limits are stored in the EEPROM and can be modified in the clients.
* **Global Limits:** Three global current limits are also in place, one for the 7 DC switches together, one for the dew heaters together, and one for the total input current. And the total limit of 20A for the whole box (excluding the relay). These limits will shut down parts of the device in a sequence of priorities.
* **Reverse Polarity Protection:** The XT90 input connector is folowed by a reverse polarity protection circuit.
* **Reversible Logic:** Switch logic can be swapped to correct issues where the client-side toggle is reversed (ON/OFF issue).

### Connectivity and Communication
The device supports multiple communication methods, allowing a choice of OS and connection type (USB or WiFi).

| Connection Type | Operating System | Client Interface | Notes |
| :--- | :--- | :--- | :--- |
| **USB/Serial** | Windows | ASCOM Driver | The ASCOM driver provides its own minimised UI, similar to GSS or EQMOD. |
| **USB/Serial** | Linux | Indi Driver | Use the dedicated Indi driver (In development). |
| **WiFi** | Windows | Alpaca Server | Provides similar functionnality to the ASCOM USB driver but no driver installation needed if your client supports Alpaca; it finds the device on the network automatically.|
| **WiFi** | Any OS | HTTP Browser Interface | Accessible at the device's IP address on port 4040. Meant for quick device configuration only (like a router's configuration page).  |

---

## 🛠️ Project Components

The complete project includes everything you need to build and run your own power box:
* **Hardware:** Schematics, gerber files, and a Bill of Materials (BOM) list for PCB creation.
* **Enclosure:** STL files for 3D printing an enclosure.
* **Software:**
    * **Firmware:** Includes base code, serial communication, an HTTP server for a browser interface, and an ASCOM/Alpaca server.
    * **Drivers:** ASCOM Driver and Indi Driver.
 
## 🛠️ Completed build illustrations

### Standard version

<img src="images/IMG20251108003557.jpg" width="483"><img src="images/IMG20251108002914.jpg" width="500">

### USB HUB version

<img src="images/IMG20251207185915.jpg" width="500"><img src="images/IMG20251207185200.jpg" width="500">
---

## 🚀 Getting Started

### 🔌 Flashing the firmware

Right now the project is configured via Visual Studio Code and PlatformIO. Onece ready, clone this repositery inside Visual Studio code, connect the box via USB, select the right COM Port and flash the firmware. Normally you shouldn't have anything more to do but if for some reason  the flash fails when attempting to upload,  press the BOOT button on the pcb, keep pressing and reflash again. Wait for it to finish. 
A archive with the firmware configured for the Arduino IDE will be added soon.

To get your Open Power Box communicating with your host PC, you will need to install the appropriate drivers.

### 🔌 Windows Setup (USB/Serial)

If you are using **Windows** and connecting via **USB/Serial**, you must install the **ASCOM driver**:

1.  **Install the ASCOM Driver:** Download and install the dedicated ASCOM Driver for this project.
2.  **Connect the Device:** Connect the power box to your Windows PC via the USB/Serial interface.
3.  **Client Interface:** The driver provides a limited user interface in your preferred client (like **NINA**) with control for all the switches and display of the main measurements.
7.  **Driver UI:** When you open the COM port, in addition to the controls available in the client ,the driver will also automatically launch its own **User Interface**, minimized by default, functioning similarly to tools like GSS or EQMOD. This last UI provides the full functionnality of the device.
   
### 💻 Linux Setup (USB/Serial)

If you are using **Linux** and connecting via **USB/Serial**, you can use the provided INDI driver. It should be available through the last release of INDI but in case it isn't, you can compile from source.

1. Make sure you have a stable installation of **INDI** and **cmake**.
2. If this is not already done,  make also sure you have the right dependencies to build indi projects:
```
sudo apt install build-essential devscripts debhelper fakeroot cdbs software-properties-common cmake
sudo add-apt-repository ppa:mutlaqja/ppa
sudo apt install libindi-dev libnova-dev libz-dev libgsl-dev
```
3. Download/clone this repository, open a terminal into INDI Driver, make a new build folder and cd into it.
```
mkdir build
cd build
```
5. Inside the build folder, execute the following commands
```
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ../
```
You should now see the config.h and indi_opbdriver.xml files.
Stay in the Build folder and build the executable
```
make
```
and install the driver now with
```
sudo make install
```
6. In Ekos, the driver will be located in the Power category under the Generic Manufacturer and **named Open Power Box**
7. When loading the driver, you will need to paste the usb id of the device. You can find it in a terminal with
```
ls /dev/serial/by-id
```
or
```
ls -l /dev/serial/by-id
```

### 🌐 WiFi Setup

#### Setting up the WiFi credentials in the device

##### Via the USB Driver
After flashing the device with the firmware, there will be no SSID or password stored. To do that, you can use the ASCOM USB Driver. After installing it, lauch you client of choice and select the ascom driver, then your COM Port in the settings and connect. The auxiliary UI will appear. Click on the Parameter tab and below you can fill the fields for the SSID and the password. Click on the Restart button to reset the device. After a few seconds, if the device managed to connect,  the IP address displayed in the UI will be updated. The device is now ready to be used via the local network.   

##### Via Editing the Frimware
If you really don't want or can't install the usb driver, you can still store the WiFi credentials in the code before flashing it. 
Open main.cpp, find the following lines:
```
   //ssid1=""; 
   //pwd1="";
```
remove the // on both linesand add your SSID and password in the between the "", like for exemple : 
```
   ssid1="ssidwifi"; 
   pwd1="passwifi";
```
and finally Flash the device.
To get the IP address, open a Serial connection on the COM Port and push the reset button on the pcb. The IP address will be displayed during boot.
We recommand that you go into your router to reserve that IP address for the MAC address of the device.

####  Alpaca
Alpaca is a network version of the ASCOM API. It provides tools to connect to network capable devices and communicate with them in the same way you normally do with a normal USB ASCOM driver. The main difference for the user is that Alpaca only requires that you have an up to date version of ASCOM installed. It does not however requires you to any extra driver for your devices as long as the device can connect to your local network. In you preferred client, say Nina, you would go to your equpment setup page, in the switch section you would open the list of devices and OpenPowerBox should appear by itself if it is already connected to the local network. Sometimes it may take a few trys to get it to appear in the list. Select it, connect and voilà. You have basic access to the device. 

#### Browser
If you know the IP adress of the device and it is connected to the local network, you can open a page at http://ip_adress:4040/. This UI gives you access to almost all the features of the power box appart from automation. In particular, you can read every sensor available wich allows you to debug issues more easily in case of problems with a device. It also allows you to change configuration like the current limits of the software fuses, the logic polarity of the switches, tha names seen by ASCOM and wifi credentials. If the IP adress is unknown, you can get it through the USB interface.  See instructions above.

---

## 🏭 Manufacturing and Assembly Notes

### Choose your fighter : USB Hub or no USB Hub

The project proposes twho versions. The base project only has power switches ( dc and pwm) while the second version add an extra usb hub. This last one requires a second pcb dedicated to the HUB. This has been done to minimize manufacturing costs. The USB hub requires a 4 layer pcb while the power board needs only a 2 layer board. Putting everything on a single 4 layer pcb may force us to make a large and costly board. 

### Printed Circuit Board (PCB) Fabrication
* The main board is a **2-layer board** and should preferably be manufactured with a **2 oz copper thickness** to maximize the amount of copper and minimize voltage drop across the board.
* The USB hub board is a **4-layer board** and can be manufactured with a **normal 1 oz copper thickness on surface layers**. With JLC, specify the JLC04161H-3313 stackup.
  
| Layer | Material Type | Thickness |
| :---: | :---: | :---: |
| Top Layer | Copper | 0.035mm | 
| Prepreg | 3313*1 | 0.09940mm | 
| Inner Layer L2 | Copper | 0.0152mm | 
| Core | Core | 1.265mm | 
| Inner Layer L3 | Copper | 0.0152mm | 
| Prepreg | 3313*1 | 0.09940mm | 
| Bottom Layer  | Copper | 0.035mm | 

The choice of stackup affects the performance of the USB traces on the pcb or, more specificaly this stackup allows us to use really fine traces, wich simplifies the layout phase of the pcb. If you use our gerbers to make your own board, this is the stackup to use. If you choose a different stackup, you will need to re-engineer the differential pairs and redo the layout. This configuaration has been tested and achieves usb transfert speeds up to the theoritical maximum of HIGH SPEED USB2 ( 480 Mbit/s).

* Prototypes were manufactured using **JLC PCB**. If you choose the same manufacturer and encounter out-of-stock parts not easily replaced by an equivalent, you can request that JLC source those specific parts on the global market for use during manufacturing.
  

### Wiring
* If you are using the provided **3D printable case**, the terminal blocks on the PCB should be oriented **inward** to facilitate easier wiring to the external connectors.
* The connectors you will choose to use need to be rated for the currents you want. Typycal 12v barrel jacks and RCA jacks are not generally made to support large currents so you should be careful when selecting them. The ones used for the building of the test units were found on Amazon and may not be available anymore when you read this. iF you choose other connectors and want to use our 3dprinted case, make sure to correct the hole diamater if necessary.
* The XT90 innput connector we use accepts a gauge of 10 AWG wich should be suitable for 20-30A over 1 or 2m. Flexible multistranded wire should be used here in order to avoid resistance when moving the telescope around as the powerbox would be typically put on top of the setup and its power cable would laying down to you power supply.
* Always size the other cables appropriatly. If you expect to use the full 5A of an output, not only the gauge should be checked but also every intermediary connector, if possible, as they are very likely to not make good enough contact for this kind of stress.
* Terminate your internal wires with ferrules when connecting to a terminal block.
  
### Extra component sourcing (if still available at the time of reading):
   * 5.5mm x 2.1mm female barrel jacks from manufacturer RUNCCI-YUN: https://www.amazon.fr/dp/B09TVDDWN2?ref=ppx_yo2ov_dt_b_fed_asin_title
   * RCA female jacks from RUNCCI-YUN: https://www.amazon.fr/dp/B09TPL4TV5?ref=ppx_yo2ov_dt_b_fed_asin_title
   * XT90 connector https://www.amazon.fr/dp/B0CF85B2LY?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1
   * Suitable power supply : Mean well LRS-350-12 wich is a 350W PSU ( or the LRS-450-12 if you can afford it) https://www.meanwell.com/webapp/product/search.aspx?prod=LRS-350 

---

## ⚙️ ASCOM Switch Class Overview

The power box is designed around the **ASCOM Switch class**, which is tailored for peripherals like power boxes and handles On/Off switches, Dew Heaters, and sensors.

In the ASCOM world, these entities are all referred to as a **"switch"**:

* **ON/OFF Switches:** Self-explanatory.
* **Dew Heaters:** Referred to as "**analog**" switches with an infinite range of values instead of just two.
* **Sensors:** Considered "**read-only**" switches.

Clients determine the switch type by querying the `CanWrite`, `Min`, `Max` and `Step` properties. The state of all switches is stored in a large array, with an index defining the order they are read by the client.

### Example Switch Array Structure

| Index | Switch Name | Can Write | Min-Max | Step |
| :---: | :---: | :---: | :---: | :---: |
| 0 | dc1 | Yes | 0-1 | 1 |
| 1 | dc2 | Yes | 0-1 | 1 |
| 2 | pwm | Yes | 0-100 | 10 |
| 3 | sensor | No | - | - |

### Full Switch array

Base Version:

| Switch | DC1 | DC2 | DC3 | DC4 | DC5 | DC6 | DC7 | PWM1 | PWM2 | PWM3 | Ren1 | Ren2 | Ren3 | DC Rail | Relay | Input V | Total A | Total A DC | Total A PWM | DC1_V | DC1_A | DC2_V | DC2_A | DC3_V | DC3_A | DC4_V | DC4_A | DC5_V | DC5_A | DC6_V | DC6_A | DC7_V | DC7_A | PWM1_V | PWM1_A | PWM2_V | PWM2_A | PWM3_V | PWM3_A | DC Rail_V | DC Rail_A | Temp | Hum | Dew_point |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| index | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 | 32 | 33 | 34 | 35 | 36 | 37 | 38 | 39 | 40 | 41 | 42 | 43 |

USB HUB Version:

| Switch | DC1 | DC2 | DC3 | DC4 | DC5 | DC6 | DC7 | PWM1 | PWM2 | PWM3 | Ren1 | Ren2 | Ren3 | DC Rail | Relay | USB1 | USB2 | USB3 | USB4 | USB5 | USB6 | USB7 | Input V | Total A | Total A DC | Total A PWM | DC1_V | DC1_A | DC2_V | DC2_A | DC3_V | DC3_A | DC4_V | DC4_A | DC5_V | DC5_A | DC6_V | DC6_A | DC7_V | DC7_A | PWM1_V | PWM1_A | PWM2_V | PWM2_A | PWM3_V | PWM3_A | DC Rail_V | DC Rail_A | Temp | Hum | Dew_point |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| index | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 | 32 | 33 | 34 | 35 | 36 | 37 | 38 | 39 | 40 | 41 | 42 | 43 | 44 | 45 | 46 | 47 | 48 | 49 | 50 |

---

## 💬 Serial Communication Protocol
If you choose to use USB to interact with the device, the firmware communicates via serial commands over the COM port. While the webserver is regularly updated, there is no data sent automatically through the serial port, the driver has to poll the device. This means that, for now, the general behavior of the COM port is synchronous.
Each command sent to the device will trigger a response even if no data is necesseraily expected back. For example, if you send the command to change the sate of a switch, the device will execute the command and then send the changed state back as an answer to acknowlege the action.  Also, the formatting of the commands sent to the device is different than the formatting of the reponse. This is intentionnal and allows for easier debugging when sniffing the com port. 

### Command Format (from Host to Device)

Commands use a simple letter and one or two parameters:

* **To Get a Value:** `# <command> <index>`
    * *Example (Get value of the first DC switch - index 0):* `# G 0`
* **To Set a Value:** `# <command> <index> <value>`
    * *Example (Set PWM index 2 to 55):* `# S 2 55`

* The response starts with `#`.
* A space separates the command letter from the index and the index from the value ( if one is required).
* If no index is needed, `0` is included by default.
* No character to terminate other than a newline and carriage return.


### Response Format

Every command  from the client triggers a response:

* **Responding to a Get command:** `#<command><index>:<value>;`
    * *Example (Get value of the first DC switch - index 0, the value is 1):* `#G0:1;`

* The response starts with `#` and is terminated with `;` (plus newline and carriage return).
* A `:` character separates the index from the value.
* No spaces are included.

### ASCOM Compliant Commands

| Method | Command | Response Format | Type [Range] | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Get switch value** | `# G <index>` | `#G<index>:<value>;` | Dc: boo[0-1], PWM: int [0-100], Sensor: float | Get the current state/value of a switch |
| **Set switch value** | `# S <index> <value>` | `#G<index>:<value>;` | Dc: boo[0-1], PWM: int [0-100] | Set the state/value of a switch |
| **Can Write** | `# W <index>` | `#W<index>:<value>;` | boo[0-1] | Tells if the switch is writable or not |
| **Maximum** | `# M <index>` | `#M<index>:<value>;` | Dc: int[1], PWM: int [100] | Gives the maximum writable value |
| **Minimum** | `# m <index>` | `#m<index>:<value>;` | Dc: int[0], PWM: int [0] | Gives the minimum writable value |
| **Get Switch Name** | `# n <index>` | `#n<index>:<value>;` | String | Get the name of a switch |
| **Set Switch Name** | `# N <index> <value>` | `#n<index>:<value>;` | String | Set the name of a switch |
| **Max switch (Total Count)** | `# X 0` | `#x0:<value>;` | Int | Total number of switches (length of the switch array) |
| **Get Switch Description** | `# D <index>` | `#D<index>:<value>;` | String | Get the description of a switch |

### Custom Commands

This project includes extra custom commands for device-specific features:

| Method | Command | Response Format | Type [Range] | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Set Visible** | `# Y <index> <value>` | `#y<index>:<value>;` | boo[0-1] | Set the flag for driver display (show/hide switch) |
| **Get Visible** | `# y <index>` | `#y<index>:<value>;` | boo[0-1] | Get the display flag |
| **Set Reverse** | `# R <index> <value>` | `#r<index>:<value>;` | boo[0-1] | Set the polarity of a switch |
| **Get Reverse** | `# r <index> <value>` | `#r<index>:<value>;` | boo[0-1] | Get the polarity of a switch |
| **Get Limit** | `# l <index>` | `#l<index>:<value>;` | Int | Get the current limit of a switch |
| **Set Limit** | `# L <index>` | `#l<index>:<value>;` | Int | Set the current limit of a switch |
| **Get local IP** | `# I 0` | `#I0:<value>;` | String | Get the IP Address of the device |
| **Set WiFi SSID** | `# F 0 <ssid>` | `#f0:<value>;` | String | Set the SSID of the WiFi AP |
| **Get WiFi SSID** | `# f 0` | `#f0:<value>;` | String | Get the SSID of the WiFi AP |
| **Set WiFi PASSWORD** | `# H 0 <password>` | `--------` | String | Set the password of the WiFi AP |
| **Restart ESP32** | `# p 0` | `--------` | - | Restart the device |
| **Get Last Error** | `# e 0` | `#e0:<value>;` | String | Get the last error message raised by the device |

---

## 💡 Extra
Space is included on the PCB for future expansion, although the code for these features is not yet implemented:
* **Ethernet Module:** Provisions for an Ethernet module via SPI bus (not implemented in code).


## DISCLAIMER
1) This project in its entirity is provided as is without any guarranty of functionnality or safety. In case of issues with the hardware or the software we may or may not provide technical support.
2) This project has been conceived from the start with the objective to limit electrical hazards as much as reasonnably possible. It includes multiple safety measures such as hardware fuses, software fuses and polarity protection of the main input. The pcb we made has also been stress tested to make sure it doesn't catch fire when pushing it to the higher currents we rated the project for.  Nonetheless there is always a risk of something going wrong with high current devices. **We decline any responsability in case of any accident occuring during the build AND the use of the device.**
When building this project, we HIGHLY recommend to :
   * make sure you have the required skills and knowledge to build/modify/manipulate electronic devices such as this one. 
   * oversize your cables whenever possible.
   * Make sure your connectors are able to support the current you need. This is easier said than done but at least keep the idea in mind when shopping. 
3) Even with all precautions taken, there will be a significant voltage drop when drawing multiple amps. If your peripheral is sensitive to voltage drops (like QHY cameras) and raising the voltage of the supply to compensate is not a safe option,  than we recommand to power the the device separatly. We measured a typical internal resistance of 10 mOhms per output (through all the device) wich will cause a typical 0.1V to 0.15V drop when pushing the powerbox to its limits. **If you still see significantly more voltage drop than this, the culprit is more than likely your cabling or a connector.**
