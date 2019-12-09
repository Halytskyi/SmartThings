# Power supply boards and USB HUBs

## Description

### Main functions

- Control of:
  - 5 single board computers (5V)
  - 2 mini PCs (12V)
  - 2 USB HUBs (with external power supply)
  - 2 IP KVM devices (can be extended up to 5)

- Remote devices control: *On*, *Off* and *Reboot*
- Button devices control: Turn On devices by buttons (on case if remote control for some reasons not available)
- Remote reading voltage and current for single board computers and mini PCs

### Specification

- **i2c address**: *0x03*
- **Button #1**: Turn ON devices connected to D2-D10 Arduino Pins
- **Button #2**: Turn ON devices connected to D11-A1 Arduino Pins

***Note:*** Devices can't be turned off by buttons

### Tools

[python-client](python-client) - command-line tool for devices remote control

## Requirements and components

### Board #1

- 5 x IRF4905 transistors
- 5 x BC547 transistors
- 5 x 1k resistors
- 5 x INA219 current/voltage sensors
- 10 x 10k resistors
- 1 x HW-613 Mini DC-DC 3A Step Down Power Supply Module (for INA219/Arduino devices, 3.3V output)
- 1 x MT3608 DC-DC Step Up Converter Booster Power Supply Module (for switch, 9V output)

### Board #2

- 1 x Arduino Pro Mini 328 - **3.3V/8MHz**
- 4 x IRF4905 transistors
- 4 x BC547 transistors
- 4 x 1k resistors
- 2 x INA219 current/voltage sensors
- 8 x 10k resistors
- 2 x USB HUB modules (added external power supply)

| Cmd | Arduino PIN | Component | Notes |
| --- | --- | --- | --- |
| c\|2 | D2 (Ext. Int.) | IRF4905 + BC547 (Switch) | USB module #1 |
| c\|3 | D3 (PWM) | IRF4905 + BC547 (Switch) | USB module #2, for serial console's (IP KVM) |
| c\|4 | D4 | IRF4905 + BC547 (Switch) | Master01 (mini PC) |
| c\|7 | D5 (PWM) | IRF4905 + BC547 (Switch) | Worker01 (mini PC) |
| c\|1 | D6 (PWM) | IRF4905 + BC547 (Switch) | Raspberry Pi |
| c\|9 | D7 | IRF4905 + BC547 (Switch) | Worker03 (Rock64) |
| c\|8 | D8 | IRF4905 + BC547 (Switch) | Worker02 (Rock64) |
| c\|6 | D9 (PWM) | IRF4905 + BC547 (Switch) | Master03 (Rock64) |
| c\|5 | D10 (PWM) | IRF4905 + BC547 (Switch) | Master02 (Rock64) |
| i\|1 | D11 (PWM) | Output 1 | IP KVM #1 |
| i\|2 | D12 | Output 2 | IP KVM #2 |
| i\|3 | D13 | Output 3 | - |
| i\|4 | A0 | Output 4 | - |
| i\|5 | A1 | Output 5 | - |
| - | A2 | Button #1 | Cluster control (D2 - D10) |
| - | A3 | Button #2 | IP KVM control (D11 - A1) |
| - | A4 | i2c SDA | Communication with i2c master |
| - | A5 | i2c SCL | Communication with i2c master |

### Components

| Name | Schema / Photo |
| --- | --- |
| Switch | [<img src="images/Switch.png" alt="Switch" width="250"/>](images/Switch.png) [<img src="images/IRF4905.png" alt="IRF4905" width="202"/>](images/IRF4905.png) [<img src="images/BC547.png" alt="BC547" width="225"/>](images/BC547.png) |
| INA219 | [<img src="images/INA219_1.png" alt="INA219" width="150"/>](images/INA219_1.png) [<img src="images/INA219_2.png" alt="INA219" width="168"/>](images/INA219_2.png) [<img src="images/INA219_addrs.png" alt="INA219" width="162"/>](images/INA219_addrs.png) |
| HW-613 | [<img src="images/HW-613_1.png" alt="HW-613" width="130"/>](images/HW-613_1.png) [<img src="images/HW-613_2.png" alt="HW-613" width="152"/>](images/HW-613_2.png) |
| USB HUB | [<img src="images/USB_HUB1.png" alt="USB_HUB" width="150"/>](images/USB_HUB1.png) [<img src="images/USB_HUB2.png" alt="USB_HUB" width="155"/>](images/USB_HUB2.png) |

## Commands

| Command | Description | EEPROM | Notes |
| --- | --- | --- | --- |
| cv0 | get status of all cluster devices | - ||
| c[e\|d\|r\|v][1-9] | enable/disable/reboot/status for cluster device | + (for e\|d) | see mapping in "Cmd" column |
| iv0 | get status of all IP KVM devices | - ||
| i[e\|d\|v][1-5] | enable/disable/status for IP KVM device | + (for e\|d) | see mapping in "Cmd" column |
| s0 | read voltage/current/power for cluster devices | - ||
| s1 | read voltage/current/power | - | Raspberry Pi |
| s2 | read voltage/current/power | - | Master01 (mini PC) |
| s3 | read voltage/current/power | - | Master02 (Rock64) |
| s4 | read voltage/current/power | - | Master03 (Rock64) |
| s5 | read voltage/current/power | - | Worker01 (mini PC) |
| s6 | read voltage/current/power | - | Worker02 (Rock64) |
| s7 | read voltage/current/power | - | Worker03 (Rock64) |

***EEPROM*** - memory whose values are kept when the board is turned off

## Device Photos

### Board #1

[<img src="images/ps-board1_1.jpg" alt="Board-1" width="200"/>](images/ps-board1_1.jpg)
[<img src="images/ps-board1_2.jpg" alt="Board-1" width="213"/>](images/ps-board1_2.jpg)
[<img src="images/ps-board1_3.jpg" alt="Board-1" width="210"/>](images/ps-board1_3.jpg)
[<img src="images/ps-board1_4.jpg" alt="Board-1" width="240"/>](images/ps-board1_4.jpg)
[<img src="images/ps-board1_5.jpg" alt="Board-1" width="236"/>](images/ps-board1_5.jpg)
[<img src="images/ps-board1_6.jpg" alt="Board-1" width="212"/>](images/ps-board1_6.jpg)
[<img src="images/ps-board1_7.jpg" alt="Board-1" width="414"/>](images/ps-board1_7.jpg)

### Board #2
[<img src="images/ps-board2_1.jpg" alt="Board-2" width="206"/>](images/ps-board2_1.jpg)
[<img src="images/ps-board2_2.jpg" alt="Board-2" width="209"/>](images/ps-board2_2.jpg)
[<img src="images/ps-board2_3.jpg" alt="Board-2" width="196"/>](images/ps-board2_3.jpg)
[<img src="images/ps-board2_4.jpg" alt="Board-2" width="241"/>](images/ps-board2_4.jpg)
[<img src="images/ps-board2_5.jpg" alt="Board-2" width="262"/>](images/ps-board2_5.jpg)
[<img src="images/ps-board2_6.jpg" alt="Board-2" width="210"/>](images/ps-board2_6.jpg)
[<img src="images/ps-board2_7.jpg" alt="Board-2" width="214"/>](images/ps-board2_7.jpg)
[<img src="images/ps-board2_8.jpg" alt="Board-2" width="194"/>](images/ps-board2_8.jpg)
[<img src="images/ps-board2_9.jpg" alt="Board-2" width="187"/>](images/ps-board2_9.jpg)
[<img src="images/ps-board2_10.jpg" alt="Board-2" width="206"/>](images/ps-board2_10.jpg)
[<img src="images/ps-board2_11.jpg" alt="Board-2" width="237"/>](images/ps-board2_11.jpg)
[<img src="images/ps-board2_12.jpg" alt="Board-2" width="204"/>](images/ps-board2_12.jpg)
[<img src="images/ps-board2_13.jpg" alt="Board-2" width="191"/>](images/ps-board2_13.jpg)

## URLs

[Raspberry Pi INA219 Tutorial](https://www.rototron.info/raspberry-pi-ina219-tutorial)