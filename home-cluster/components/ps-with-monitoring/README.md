# Power Supply with Monitoring

This is version 3 with power supplies on 18V and 24V and included input for connecting Solar Batteries on 24V.

## Description

Power Supply Monitoring module was developed for measure voltage, current and power consumption on AC line, PS outputs and Solar Batteries input with ability send data to server via **i2c protocol**.

## Main functions

- measure voltage, curent and power consumption on PS outputs, Solar Batteries input and dc-dc converters;
- measure AC line parameters: Voltage, Current, Power, Energy, Frequency and Power factor;
- measuring temperature on PS outputs and dc-dc converters;
- send data to server via i2c protocol.

## Specification

- i2c address: 0x14

## Requirements and components for Power Supply

- 1 x Power supply 18V 20A (model: JC-360-18)
- 1 x Power supply 24V 15A (model: Alito, ALT-1220T)
- 2 x 300W 20A DC-DC Buck Converter Step Down Modules
- 2 x MBR6045PT diodes (for mixing lines from 3 inputs)
- 2 x MBR60100CT diodes (for backup)
- 1 x MBR4045CT diode (for DC convertors)
- 4 x 34x12x38mm heatsink for MBR6045PT diodes (19.3V -> 18.8V, 8A (~150W) - 70℃; 22V -> 21.47V, 7A (~150W) - 60℃)
- 1 x 34x12x30mm heatsink for MBR4045CT diode (12.6V -> 12.05V, 5A (~60W) - 60℃)

## Requirements and components for monitoring module

- 1 x Arduino Pro Mini 328 - **5V/16MHz**
- 1 x HW-613 Mini DC-DC 3A Step Down Power Supply Module (**5V output**)
- 4 x ACS712-20A modules
- 4 x 10k resistors
- 4 x 100k resistors
- 4 x DS18B20
- 1 x PZEM004T v3.0
- 2 x 1 MOm resistors
- 1 x 1N4001 diode
- 1 x Bidirectional Logic Level Converter

| Arduino PIN | Component | Notes |
| --- | --- | --- |
| D2 (Ext. Int.) | Rx (Connects to the Tx pin on the PZEM) | [PZEM004T v3.0](https://innovatorsguru.com/pzem-004t-v3) |
| D3 (PWM) | Tx (Connects to the Rx pin on the PZEM) | [PZEM004T v3.0](https://innovatorsguru.com/pzem-004t-v3) |
| D4 | - ||
| D5 (PWM) | - ||
| D6 (PWM) | - ||
| D7 | - ||
| D8 | - ||
| D9 (PWM) | - ||
| D10 (PWM) | 1-Wire | Temperature sensors |
| D11 (PWM) | - ||
| D12 | - ||
| D13 | - ||
| A0 | Voltmeter: r1=100k, r2=10k | 24V 15A PS output (v1) |
| A1 | ACS712-20A | 24V 15A PS output (i1) |
| A2 | ACS712-20A | 18V 20A PS output (i2) |
| A3 | Voltmeter: r1=100k, r2=10k | 18V 20A PS output (v2) |
| A4 | i2c SDA (through Bidirectional LLC) | Communication with i2c master |
| A5 | i2c SCL (through Bidirectional LLC) | Communication with i2c master |
| A6 | Voltmeter: r1=100k, r2=10k | 24V Solar Battaries output (v3) or 12V DC-DC output (v4) |
| A7 | ACS712-20A | 24V Solar Battaries output (i3) or 12V DC-DC output (i4) |

### Components photos and schematics

| Name | Schema / Photo |
| --- | --- |
| Voltmeter | [<img src="images/voltmeter.jpg" alt="Voltmeter" width="170"/>](images/voltmeter.jpg) |
| ACS712 | [<img src="images/ACS712_1.jpg" alt="ACS712_1" width="170"/>](images/ACS712_1.jpg) [<img src="images/ACS712_2.jpg" alt="ACS712_2" width="294"/>](images/ACS712_2.jpg) |
| HW-613 | [<img src="images/HW-613_1.jpg" alt="HW-613" width="130"/>](images/HW-613_1.jpg) [<img src="images/HW-613_2.jpg" alt="HW-613" width="152"/>](images/HW-613_2.jpg) |
| DS18B20 | [<img src="images/DS18B20.jpg" alt="DS18B20" width="330"/>](images/DS18B20.jpg) |
| PZEM-004T v3 | [<img src="images/PZEM-004T-v3.jpg" alt="PZEM-004T" width="330"/>](images/PZEM-004T-v3.jpg) |
| 300W 20A DC-DC Buck Converter Step Down Module | [<img src="images/DC-DC_20A_300W_converter_1.jpeg" width="180"/>](images/DC-DC_20A_300W_converter_1.jpeg) [<img src="images/DC-DC_20A_300W_converter_2.jpeg" width="200"/>](images/DC-DC_20A_300W_converter_2.jpeg) |

### Commands

| Command | Description | EEPROM | Notes |
| --- | --- | --- | --- |
| sd | Read value of source: Solar Battaries or DC-DC converter | - | 0 - Solar Battaries<br>1 - DC-DC converter |
| sd=[0-1] | Set value of source: Solar Battaries or DC-DC converter | + | 0 - Solar Battaries<br>1 - DC-DC converter |
| v | Read value of voltage for 1-4 outputs | - | Volt |
| v[1-4] | Read value of voltage for 1-4 outputs | - | Volt |
| i | Read value of current for 1-4 outputs | - | Amper |
| i[1-4] | Read value of current for 1-4 outputs | - | Amper |
| p | Read value of power consumption for 1-4 outputs | - | Watt (Volt * Amper) |
| p[1-4] | Read value of power consumption for 1-4 outputs | - | Watt (Volt * Amper) |
| t | Read value of all temperature sensors | - | °C |
| t[1-4] | Read value of temperature on PS outputs, dc-dc converter and near diodes | - | °C |
| ta | Read value of auto-mode for get temperatures | - | 0 - disabled<br>10-120 - seconds |
| ta=[10-120] | Set value of auto-mode for get temperatures | + | 0 - disabled<br>10-120 - seconds |
| l | Read value of all AC line parameters | - | Voltage (V)<br>Current (A)<br>Power (W)<br>Energy (kWh)<br>Frequency (Hz)<br>Power factor |
| l[v,c,p,e,f,pf] | Read value of AC line parameters | - | Voltage (V)<br>Current (A)<br>Power (W)<br>Energy (kWh)<br>Frequency (Hz)<br>Power factor |
| la | Read value of auto-mode for get AC line parameters | + | 0 - disabled<br>10-120 - seconds |
| la=[10-120] | Set value of auto-mode for get AC line parameters | + | 0 - disabled<br>10-120 - seconds |

where,<br>
[v,i,p]-1 - 24V 15A PS output<br>
[v,i,p]-2 - 18V 20A PS output<br>
[v,i,p]-3 - 24V Solar Battaries output<br>
[v,i,p]-4 - 12V DC-DC output<br>
t1 - 24V 15A PS<br>
t2 - 18V 20A PS<br>
t3 - 12V DC-DC2, from 24V 15A PS<br>
t4 - 12V DC-DC1, from 18V 20A PS<br>
***EEPROM*** - memory values are kept when the board is turned off

**Note:** read values of AC line parameters are available only when the auto-mode is enabled (la=[10-60]).

## Device Photos

### UPS schema and common photo

[<img src="images/ps-monitoring_schema.jpeg" width="300"/>](images/ps-monitoring_schema.jpeg)

Version 3:

[<img src="images/ps-monitoring_1_v3.jpeg" width="277"/>](images/ps-monitoring_1_v3.jpeg)
[<img src="images/ps-monitoring_2_v3.jpeg" width="350"/>](images/ps-monitoring_2_v3.jpeg)
[<img src="images/ps-monitoring_3_v3.jpeg" width="350"/>](images/ps-monitoring_3_v3.jpeg)
[<img src="images/ps-monitoring_4_v3.jpeg" width="250"/>](images/ps-monitoring_4_v3.jpeg)
[<img src="images/ps-monitoring_5_v3.jpeg" width="293"/>](images/ps-monitoring_5_v3.jpeg)

Upgraded 24V 15A PS version (2):

[<img src="images/ps-monitoring_common_24v15a_1.jpeg" width="350"/>](images/ps-monitoring_common_24v15a_1.jpeg)
[<img src="images/ps-monitoring_common_24v15a_2.jpeg" width="350"/>](images/ps-monitoring_common_24v15a_2.jpeg)
[<img src="images/ps-monitoring_common_24v15a_3.jpeg" width="350"/>](images/ps-monitoring_common_24v15a_3.jpeg)
[<img src="images/ps-monitoring_common_24v15a_4.jpeg" width="350"/>](images/ps-monitoring_common_24v15a_4.jpeg)

Old 24V 10A PS version (1):

[<img src="images/ps-monitoring_common_1.jpeg" width="350"/>](images/ps-monitoring_common_1.jpeg)
[<img src="images/ps-monitoring_common_2.jpeg" width="408"/>](images/ps-monitoring_common_2.jpeg)
[<img src="images/ps-monitoring_common_3.jpeg" width="179"/>](images/ps-monitoring_common_3.jpeg)
[<img src="images/ps-monitoring_common_4.jpeg" width="350"/>](images/ps-monitoring_common_4.jpeg)
[<img src="images/ps-monitoring_common_5.jpeg" width="152"/>](images/ps-monitoring_common_5.jpeg)

[<img src="images/ps-monitoring_common_6.jpeg" width="700"/>](images/ps-monitoring_common_6.jpeg)

### PS diodes

[<img src="images/ps-diodes_1.jpeg" width="300"/>](images/ps-diodes_1.jpeg)
[<img src="images/ps-diodes_2.jpeg" width="282"/>](images/ps-diodes_2.jpeg)
[<img src="images/ps-diodes_3.jpeg" width="300"/>](images/ps-diodes_3.jpeg)
[<img src="images/ps-diodes_4.jpeg" width="131"/>](images/ps-diodes_4.jpeg)
[<img src="images/ps-diodes_5.jpeg" width="282"/>](images/ps-diodes_5.jpeg)
[<img src="images/ps-diodes_6.jpeg" width="150"/>](images/ps-diodes_6.jpeg)
[<img src="images/ps-diodes_7.jpeg" width="266"/>](images/ps-diodes_7.jpeg)

### DC diode

[<img src="images/dc-diode_1.jpeg" width="300"/>](images/dc-diode_1.jpeg)
[<img src="images/dc-diode_2.jpeg" width="300"/>](images/dc-diode_2.jpeg)
[<img src="images/dc-diode_3.jpeg" width="200"/>](images/dc-diode_3.jpeg)

### Monitoring board version 2

**Note:** added [PZEM004T v3.0](https://innovatorsguru.com/pzem-004t-v3) and [PJON filters](https://github.com/gioblu/PJON/wiki/Mitigate-interference) (1MOm resistors and diode)

[<img src="images/ps-monitoring_1_v2.jpg" alt="PS monitoring" width="300"/>](images/ps-monitoring_1_v2.jpg)
[<img src="images/ps-monitoring_2_v2.jpg" alt="PS monitoring" width="296"/>](images/ps-monitoring_2_v2.jpg)
[<img src="images/ps-monitoring_3_v2.jpg" alt="PS monitoring" width="305"/>](images/ps-monitoring_3_v2.jpg)

### Monitoring board version 1

[<img src="images/ps-monitoring_1_v1.jpg" alt="PS monitoring" width="300"/>](images/ps-monitoring_1_v1.jpg)
[<img src="images/ps-monitoring_2_v1.jpeg" alt="PS monitoring" width="310"/>](images/ps-monitoring_2_v1.jpeg)
[<img src="images/ps-monitoring_3_v1.jpg" alt="PS monitoring" width="346"/>](images/ps-monitoring_3_v1.jpg)
[<img src="images/ps-monitoring_4_v1.jpg" alt="PS monitoring" width="326"/>](images/ps-monitoring_4_v1.jpg)
