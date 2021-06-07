# Cluster Cooling

## Description

Cluster Cooling was developed for fans control depends from temperature with ability control and send data from/to server via [PJON protocol](https://github.com/gioblu/PJON).

## Main functions

- 4 PWM outputs for fans;
- fans control by commands from server;
- fans control automatically in depends from temperature. Can be chosen 1 of 2 modes: fan on/off without speed control and with speed control (fan speed will be gradually changed by PWM and depends from temperature sensors (check every 10 sec.) and defined low/high limits);
- measure temperature in 2x4 (8) zones;
- turn off all fans by external 5V HIGH signal (for example from fire alarm) or by command (A). Normal functionality after that can be resumed only by command via PJON;
- control and send data from/to server via [PJON protocol](https://github.com/gioblu/PJON)

## PJON Specification

- PJON TxRx Bus Server ID: _1_
- PJON Tx Bus Server ID: _2_
- PJON Bus Device ID: _21_
- PJON Strategy: _SoftwareBitBang_

## Requirements and components

- 1 x Arduino Pro Mini 328 - 5V/16MHz
- 1 x 1N4001 diode
- 4 x IRLZ44N
- 4 x 270 Om resistors
- 5 x 10 kOm resistors
- 1 x 4.7k resistor
- 2 x 1 MOm resistors (pull-down resistors for PJON)
- 8 x DS18B20
- 1 x HW-613 Mini DC-DC 3A Step Down Power Supply Module
- 1 x 1.5A fuse

| Arduino PIN | Component | Notes |
| --- | --- | --- |
| D2 (Ext. Int.) | - | - |
| D3 (PWM) | IRLZ44N fan switch | Fan 1: 3 x 80x80 |
| D4 | - | - |
| D5 (PWM) | IRLZ44N fan switch | Fan 2: 3 x 80x80 |
| D6 (PWM) | IRLZ44N fan switch | Fan 3: 3 x 80x80 |
| D7 | [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0/src/strategies/SoftwareBitBang) | Communication with Server (TxRx) |
| D8 | - | - |
| D9 (PWM) | IRLZ44N fan switch | Fan 4: 3 x 30x30 |
| D10 (PWM) | Alarm input | For external 5V HIGH signal |
| D11 (PWM) | 1-Wire | Temperature sensors |
| D12 | [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0/src/strategies/SoftwareBitBang) | Communication with Server (TX only) |
| D13 | - | - |
| A0 | - | - |
| A1 | - | - |
| A2 | - | - |
| A3 | - | - |

### Components photos and schematics

| Name | Schema / Photo |
| --- | --- |
| Fan switch | [<img src="images/MosfetN_Switch.png" alt="Switch" width="200"/>](images/MosfetN_Switch.png) [<img src="images/IRLZ44N.png" alt="irlr2905" width="259"/>](images/IRLZ44N.png) |
| DS18B20 | [<img src="images/DS18B20.jpg" alt="DS18B20" width="330"/>](images/DS18B20.jpg) |
| HW-613 | [<img src="images/HW-613_1.jpg" alt="HW-613" width="130"/>](images/HW-613_1.jpg) [<img src="images/HW-613_2.jpg" alt="HW-613" width="152"/>](images/HW-613_2.jpg) |

## Commands

| Command | Description | EEPROM | Auto-push | Notes |
| --- | --- | --- | --- | --- |
| F-[1-4] | Read value of fan speed | - | + (auto push every 1 minute) | 0 - fan disabled<br>1-100 - fan speed (%) |
| F-[1-4]=[0-100] | Define fan speed | + | - | 0 - disable fan (default)<br>1-100 - fan speed (%) |
| F-[1-4]-a | Read value of "auto push" for fan speed | - | - | 0 - disabled<br>1 - enabled |
| F-[1-4]-a=[0,1] | Disable/Enable "auto push" for fan speed (useful for automatic mode) | + | - | 0 - disable (default)<br>1 - enable |
| F-[1-4]-ac | Read value of automatic fan control mode | - | - | 0 - disabled<br>1 - enabled without speed control<br>2 - enabled with speed control |
| F-[1-4]-ac=[0-2] | Disable/Enable automatic fan control mode | + | - | 0 - disable (default)<br>1 - enable without speed control<br>2 - enable with speed control |
| F-[1-4]-tl | Read value for "temperature low limit" of temperature sensors | - | - | °C, if temperature is less than defined value - correspond fan is stopped |
| F-[1-4]-tl=[20-25] | Define "temperature low limit" for temperature sensors | + | - | °C, value from 20 to 25 (default: 22) |
| F-[1-4]-th | Read value for "temperature high limit" of temperature sensors | - | - | °C, if temperature is greater than defined value - correspond fan speed is 100% |
| F-[1-4]-th=[26-39] | Define "temperature high limit" for temperature sensors | + | - | °C, value from 26 to 39 (default: 30) |
| T-[1-8] | Read temperature of sensors 1-8 | - | + (auto push every 1 minute) | °C, see notes below |
| T-[1-8]-a | Read value of "auto push" for 1-8 sensors | - | - | 0 - disabled<br>1 - enabled |
| T-[1-8]-a=[0,1] | Disable/Enable "auto push" for 1-8 sensors | + | - | 0 - disable (default)<br>1 - enable |
| A | External alarm status | - | + (auto push every 1 minute if status "1") | 0 - no alarm<br>1 - alarm |
| A=[0,1] | Disable/enable external alarm | + | - | 0 - disable<br>1 - enable |

***EEPROM*** - memory values are kept when the board is turned off<br/>
***Auto-push*** - periodically send data to server

**Notes:** `1-2` temperature sensors for "Fan #1", `3-4` - "Fan #2", etc. In automatic fan control mode 2 fan speed calculation by temperature from sensor in group with highest temperature.

## Device Photos

[<img src="images/cluster-cooling_1.jpeg" width="300"/>](images/cluster-cooling_1.jpeg)
[<img src="images/cluster-cooling_2.jpeg" width="288"/>](images/cluster-cooling_2.jpeg)
[<img src="images/cluster-cooling_3.jpeg" width="300"/>](images/cluster-cooling_3.jpeg)
[<img src="images/cluster-cooling_4.jpeg" width="310"/>](images/cluster-cooling_4.jpeg)
[<img src="images/cluster-cooling_5.jpeg" width="326"/>](images/cluster-cooling_5.jpeg)
[<img src="images/cluster-cooling_6.jpeg" width="279"/>](images/cluster-cooling_6.jpeg)
[<img src="images/cluster-cooling_7.jpeg" width="278"/>](images/cluster-cooling_7.jpeg)
