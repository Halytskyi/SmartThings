# Cameras and lamps control

## Description

Cameras and lamps control module was developed for remote and automatic on/off cameras and lamps with ability send data to server via [PJON protocol](https://github.com/gioblu/PJON).

Entire control box consists from 3 modules:

- Cameras and lamps control module;
- Module of fuses;
- Ethernet switch

### Main functions

- 2 outputs for cameras which can be controlled remotely;
- 2 PWM outputs for lamps which can be controlled remotely and automatically (motion + light sensors);
- for each motion sensor can be defined any lamp for control in automode;
- for each lamp can be defined any light sensor for control in automode;
- can be enabled "blinking mode" for any sensor in lamp automode (see description in "commands" table, 'S-m-[1-3]-b');
- lamps blinking via remote control (see description in "commands" table, 'L-[1-2]-b');
- change lamps brightness to 50% and "motion time" to 5 seconds when power supply from battery (see in "commands" table, 'B');
- enable "alarm output" on defined time (up to 120 seconds) by command from server (see in "commands" table, 'A');
- send messages to server if motion detected (can be configured for each sensor separately);
- control and send data from/to server via [PJON protocol](https://github.com/gioblu/PJON)

### PJON Specification

- PJON TxRx Bus Server ID: _1_
- PJON Tx Bus Server ID: _6_
- PJON Bus Device ID: _25-29_
- PJON Strategy: _SoftwareBitBang_

## Requirements and components

- 1 x Arduino Pro Mini 328 - 5V/16MHz
- 2 x HW-613 Mini DC-DC 3A Step Down Power Supply Module
- 5 x IRLZ44N
- 5 x 270 Om resistors
- 3 x 10k resistor
- 3 x 100k resistors
- 2 x 1 MOm resistors
- 1 x 1N4001 diode
- 2 x light sensors (GL5528)
- 4 x motion sensors (HC-SR501)
- 1 x 0.2A fuse (control module)
- 2 x 0.5A fuse (ethernet switch and alarm output)
- 4 x 1A (lamps and cameras)
- 1 x 5A (common fuse)
- 1 x thermal fuse 10A 65°C
- 1 x ABS Plastic IP65 Waterproof (150x110x70mm)

| Arduino PIN | Component | Notes |
| --- | --- | --- |
| D2 (Ext. Int.) | - | - |
| D3 (PWM) | IRLZ44N switch | Lamp 1 |
| D4 | - | - |
| D5 (PWM) | IRLZ44N switch | Camera 1 |
| D6 (PWM) | IRLZ44N switch | Lamp 2 |
| D7 | [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0/src/strategies/SoftwareBitBang) | Communication with Server (TxRx) |
| D8 | - | - |
| D9 (PWM) | IRLZ44N switch | Camera 2 |
| D10 (PWM) | - ||
| D11 (PWM) | - ||
| D12 | [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0/src/strategies/SoftwareBitBang) | Communication with Server (TX only) |
| D13 | IRLZ44N switch | Alarm output (can be used for enable siren) |
| A0 | Motion sensor 1 ||
| A1 | Motion sensor 2 ||
| A2 | Motion sensor 3 ||
| A3 | Light sensor 1 (GL5528) ||
| A4 | Light sensor 2 (GL5528) ||
| A5 | Light sensor 3 (GL5528) ||

### Components photos and schematics

| Name | Schema / Photo |
| --- | --- |
| IRLZ44N switch | [<img src="images/IRLZ44N_switch.jpg" alt="IRLZ44N switch" width="200"/>](images/IRLZ44N_switch.jpg) [<img src="images/IRLZ44N_outline.jpg" alt="IRLZ44N outline " width="231"/>](images/IRLZ44N_outline.jpg) |
| Motion sensor | [<img src="images/motion_sensor.jpg" alt="Motion sensor" width="200"/>](images/motion_sensor.jpg) |
| Multiple motion sensors | [<img src="images/multiple_motion_sensors.jpg" alt="Multiple motion sensors" width="300"/>](images/multiple_motion_sensors.jpg) |
| Light sensor | [<img src="images/light_sensor.jpg" alt="Light sensor" width="200"/>](images/light_sensor.jpg) [<img src="images/GL5528.jpg" alt="Light sensor" width="154"/>](images/GL5528.jpg) |
| HW-613 | [<img src="images/HW-613_1.jpg" alt="HW-613" width="130"/>](images/HW-613_1.jpg) [<img src="images/HW-613_2.jpg" alt="HW-613" width="152"/>](images/HW-613_2.jpg) |
| Ethernet switch | [<img src="images/ethernet_switch.jpg" alt="Ethernet switch" width="200"/>](images/ethernet_switch.jpg) |

## Commands

| Command | Description | EEPROM | Auto-push | Notes |
| --- | --- | --- | --- | --- |
| L-[1-2] | Read value of lamps | - | - | see below |
| L-[1-2]=[0-100] | Lamps brightness | - | - | %, value from 0 to 100 <br>default: 0 |
| L-[1-2]-a | Read value of automode | - | - | see below |
| L-[1-2]-a=[0-2] | Disable/enable automode: Motion + Light sensor | + | + | 0 - disable<br>1 - enable<br>2 - enable with auto-push value when status changed<br>default: 0 |
| L-[1-2]-a-b | Read value of "max brightness" for lamps automode | - | - | see below |
| L-[1-2]-a-b=[0-100] | Define "max brightness" for lamps automode | + | - | %, value from 0 to 100<br>default: 100 |
| L-[1-2]-a-f | Read value of "fade speed" in "automode" | - | - | see below |
| L-[1-2]-a-f=[0-9] | Define "fade speed" for "automode", 0 - disabled, 9 - very slow | + | - | value from 0 to 9<br>default: 0 |
| L-[1-2]-a-l | Read value of "light sensor" for lamp in automode | - | - | see below |
| L-[1-2]-a-l=[1-3] | Define "light sensor" for lamp | + | - | 1-3 - num of light sensor<br>default for L-1: 1<br>default for L-2: 2 |
| L-[1-2]-b | Read value of "lamp blinking" | - | - | see below |
| L-[1-2]-b=[0-2] | Disable/enable "lamp blinking" | - | - | 0 - disable<br>1 - strobe<br>2 - once per second<br>default: 0 |
| S-m-[1-3] | Read value of motion sensors | - | + | 0 - motion not detected<br>1 - motion detected |
| S-m-[1-3]-a | Read value of "auto push when status changed" for motion sensors | - | - | see below |
| S-m-[1-3]-a=[0,1] | Configure "auto push when status changed" for motion sensors | + | - | 0 - disable<br>1 - enable<br>default: 0 |
| S-m-[1-3]-b | Read value of "lamp blinking" for lamps automode | - | - | see below |
| S-m-[1-3]-b=[0-2] | Define "lamp blinking" mode for lamps automode | - | - | 0 - disable<br>1 - strobe<br>2 - one blink per second<br>default: 0 |
| S-m-[1-3]-l | Read value of "lamp" for motion sensors in automode | - | - | see below |
| S-m-[1-3]-l=[1-2] | Define lamp for motion sensors | + | - | 1-2 - num of lamp<br>default for S-m-1: 1<br>default for S-m-2: 2<br>default for S-m-3: 2 |
| S-m-[1-3]-t | Read value of "time" for motion sensors. This time will added to sensor configured time after switch it to LOW state | - | - | see below |
| S-m-[1-3]-t=[0-180] | Define "time" for motion sensors | + | - | Sec., value from 0 to 180<br>default: 0 (disable)) |
| S-l-[1-3] | Read value of light sensors | - | + | %, value from 0 to 100 |
| S-l-[1-3]-a | Read value of "auto push" for light sensors | - | - | see below |
| S-l-[1-3]-a=[0-6] | Configure "auto push" for light sensors | + | - | 0 - disable<br>1-5 - minutes<br>6 - when status changed (not often 1 minute)<br>default: 0 |
| S-l-[1-3]-b | Read value of "brightness limit" for light sensors | - | - | see below |
| S-l-[1-3]-b=[0-100] | Define "brightness limit" for light sensors | + | - | %, value from 0 to 100<br>default: 0) |
| C-[1-2] | Read value of cameras | - | - | see below |
| C-[1-2]=[0,1] | Cameras control | + | - | 0 - turn OFF<br>1 - turn ON<br>default: 0 |
| A | Read value of alarm | - | - | see below |
| A=[0-120] | Disable/enable alarm | - | - | Sec., value from 0 to 120<br>default: 0 (disable)) |
| B | Read value of "power supply from battery" | - | - | see below |
| B=[0,1] | Disable/enable "power supply from battery" | - | - | 0 - disable<br>1 - enable<br>default: 0 |

If enabled "S-m-[1-3]-a" and "S-m-[1-3]-b", answer "L-1<101" - strobe, "L-1<102" - one blink per second

***EEPROM*** - memory values are kept when the board is turned off<br>
***Auto-push*** - periodically send data to server

**Note:** Motion sensors (HC-SR501) configuration: timer and sensitivity set to the minimum, the trigger jumper set to H.

## Device Photos

### Cameras and lamps control module

[<img src="images/cameras-lamps-control_1_1.jpg" alt="Cameras and lamps control module" width="250"/>](images/cameras-lamps-control_1_1.jpg)
[<img src="images/cameras-lamps-control_1_2.jpg" alt="Cameras and lamps control module" width="242"/>](images/cameras-lamps-control_1_2.jpg)
[<img src="images/cameras-lamps-control_1_3.jpg" alt="Cameras and lamps control module" width="245"/>](images/cameras-lamps-control_1_3.jpg)
[<img src="images/cameras-lamps-control_1_4.jpg" alt="Cameras and lamps control module" width="233"/>](images/cameras-lamps-control_1_4.jpg)
[<img src="images/cameras-lamps-control_1_5.jpg" alt="Cameras and lamps control module" width="222"/>](images/cameras-lamps-control_1_5.jpg)
[<img src="images/cameras-lamps-control_1_6.jpg" alt="Cameras and lamps control module" width="250"/>](images/cameras-lamps-control_1_6.jpg)
[<img src="images/cameras-lamps-control_1_7.jpg" alt="Cameras and lamps control module" width="243"/>](images/cameras-lamps-control_1_7.jpg)
[<img src="images/cameras-lamps-control_1_8.jpg" alt="Cameras and lamps control module" width="250"/>](images/cameras-lamps-control_1_8.jpg)

### Module of fuses

[<img src="images/cameras-lamps-control_2_1.jpg" alt="Module of fuses" width="274"/>](images/cameras-lamps-control_2_1.jpg)
[<img src="images/cameras-lamps-control_2_2.jpg" alt="Module of fuses" width="250"/>](images/cameras-lamps-control_2_2.jpg)

### Assembled

[<img src="images/cameras-lamps-control_3_1.jpg" alt="Assembled" width="250"/>](images/cameras-lamps-control_3_1.jpg)
[<img src="images/cameras-lamps-control_3_2.jpg" alt="Assembled" width="262"/>](images/cameras-lamps-control_3_2.jpg)
[<img src="images/cameras-lamps-control_3_3.jpg" alt="Assembled" width="250"/>](images/cameras-lamps-control_3_3.jpg)
[<img src="images/cameras-lamps-control_3_4.jpg" alt="Assembled" width="258"/>](images/cameras-lamps-control_3_4.jpg)
[<img src="images/cameras-lamps-control_3_5.jpg" alt="Assembled" width="250"/>](images/cameras-lamps-control_3_5.jpg)
[<img src="images/cameras-lamps-control_3_6.jpg" alt="Assembled" width="250"/>](images/cameras-lamps-control_3_6.jpg)
[<img src="images/cameras-lamps-control_3_7.jpg" alt="Assembled" width="250"/>](images/cameras-lamps-control_3_7.jpg)
[<img src="images/cameras-lamps-control_3_8.jpg" alt="Assembled" width="288"/>](images/cameras-lamps-control_3_8.jpg)
