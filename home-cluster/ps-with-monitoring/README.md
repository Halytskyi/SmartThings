# Power Supply with Monitoring

This is version 2 with power supplies on 18V and 24V and included input for connecting Solar Batteries on 24V.

## Description

Power Supply Monitoring module was developed for measure voltage, current and power consumption on AC line, PS outputs and Solar Batteries input with ability send data to server via [PJON protocol](https://github.com/gioblu/PJON).

## Main functions

- measure voltage, curent and power consumption on PS outputs, Solar Batteries input and dc-dc converter;
- measure AC line parameters: Voltage, Current, Power, Energy, Frequency and Power factor;
- measuring temperature on PS outputs and dc-dc converters;
- send data to server via [PJON protocol](https://github.com/gioblu/PJON)

## PJON Specification

- PJON TxRx Bus Server ID: _1_
- PJON Tx Bus Server ID: _6_
- PJON Bus Device ID: _15_
- PJON Strategy: _SoftwareBitBang_

## Requirements and components for Power Supply

- 1 x Power supply 18V 20A (model: JC-360-18)
- 1 x Power supply 24V 10A (model: S-240-24)
- 2 x 300W 20A DC-DC Buck Converter Step Down Modules
- 2 x MBR6045PT diodes (for mixing lines from 3 inputs)
- 2 x MBR60100CT diodes (for chargers)
- 1 x MBR4045CT diode (for DC convertors)
- 2 x 34x12x38mm heatsink for MBR6045PT diodes (19.3V -> 18.8V, 8A (~150W) - 70℃; 22V -> 21.47V, 7A (~150W) - 60℃)
- 2 x 34x12x30mm heatsink for MBR60100CT diodes (19.1V -> 18.4V, 3A (~55W) - 48℃; 22.6V -> 21.8V, 3A (~65W) - 50℃)
- 1 x 34x12x30mm heatsink for MBR4045CT diode (12.6V -> 12.05V, 5A (~60W) - 60℃)

### Modification power supply 24V 10A (model: S-240-24)

This power supply came with ability adjusting output voltage within 22.7 - 28.77V limits. For building my UPS the lowwer limit is little bit high, therefore, was decided to descrease it to 22.0V. This is was done by simple changing RSS1 resistor (located near ajustable resistor) from 820 Om to 1.5 kOm. As I didn't have one 1.5 kOm resistor I took 2 resistors on 1 kOm and 510 Om and connected them in series. Check the photo below:

[<img src="images/ps_24v10a_modification.jpeg" width="500"/>](images/ps_24v10a_modification.jpeg)

After this modification adjusting output voltage limits became: 21.3V - 28.77V which totally acceptable for this UPS.

## Requirements and components for monitoring module

- 1 x Arduino Pro Mini 328 - 5V/16MHz
- 1 x HW-613 Mini DC-DC 3A Step Down Power Supply Module
- 4 x ACS712-20A modules
- 4 x 10k resistors
- 4 x 100k resistors
- 4 x DS18B20
- 1 x PZEM004T v3.0
- 2 x 1 MOm resistors
- 1 x 1N4001 diode

| Arduino PIN | Component | Notes |
| --- | --- | --- |
| D2 (Ext. Int.) | Rx (Connects to the Tx pin on the PZEM) | [PZEM004T v3.0](https://innovatorsguru.com/pzem-004t-v3) |
| D3 (PWM) | Tx (Connects to the Rx pin on the PZEM) | [PZEM004T v3.0](https://innovatorsguru.com/pzem-004t-v3) |
| D4 | - ||
| D5 (PWM) | - ||
| D6 (PWM) | - ||
| D7 | [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0/src/strategies/SoftwareBitBang) | Communication with Server (TxRx) |
| D8 | - ||
| D9 (PWM) | - ||
| D10 (PWM) | 1-Wire | Temperature sensors |
| D11 (PWM) | - ||
| D12 | [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0/src/strategies/SoftwareBitBang) | Communication with Server (Tx only) |
| D13 | - ||
| A0 | Voltmeter: r1=100k, r2=10k | 24V 10A PS output (V-1) |
| A1 | Voltmeter: r1=100k, r2=10k | 24V Solar Battaries output (V-2) |
| A2 | Voltmeter: r1=100k, r2=10k | 12V DC-DC (from UPS 2+3) output (V-3) |
| A3 | Voltmeter: r1=100k, r2=10k | 18V 20A PS output (V-4) |
| A4 | ACS712-20A | 24V 10A PS output (I-1) |
| A5 | ACS712-20A | 24V Solar Battaries output (I-2) |
| A6 | ACS712-20A | 12V DC-DC (from UPS 2+3) output (I-3) |
| A7 | ACS712-20A | 18V 20A PS output (I-4) |

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

| Command | Description | EEPROM | Auto-push | Notes |
| --- | --- | --- | --- | --- |
| V-[1-4] | Read value of voltage for 1-4 outputs | - | + (auto-push every 1 minute) | Volt |
| V-[1-4]-a | Read value of auto-push voltage for 1-4 outputs | - | - | 0 - disabled<br>1 - enabled |
| V-[1-4]-a=[0,1] | Disable/enable auto-push for read values of voltage for 1-4 outputs | + | - | 0 - disable<br>1 - enable<br>default: 0 |
| I-[1-4] | Read value of current for 1-4 outputs | - | + (auto-push every 1 minute) | Amper |
| I-[1-4]-a | Read value of auto-push current for 1-4 outputs | - | - | 0 - disabled<br>1 - enabled |
| I-[1-4]-a=[0,1] | Disable/enable auto-push for read values of current for 1-4 outputs | + | - | 0 - disable<br>1 - enable<br>default: 0 |
| P-[1-4] | Read value of power consumption for 1-4 outputs | - | + (auto-push every 1 minute) | Watt (Volt * Amper) |
| P-[1-4]-a | Read value of auto-push power consumption for 1-4 outputs | - | - | 0 - disabled<br>1 - enabled |
| P-[1-4]-a=[0,1] | Disable/enable auto-push for read values of power consumption for 1-4 outputs | + | - | 0 - disable<br>1 - enable<br>default: 0 |
| T-[1-4] | Read value of temperature on PS outputs, dc-dc converter and near diodes | - | + (auto-push every 1 minute) | °C |
| T-[1-4]-a | Read value of auto-push for temperature on PS outputs, dc-dc converter and near diodes | - | - | 0 - disabled<br>1 - enabled |
| T-[1-4]-a=[0,1] | Disable/Enable auto-push for read values of temperature on PS outputs, dc-dc converter and near diodes | + | - | 0 - disable<br>1 - enable<br>default: 0 |
| L-[v,c,p,e,f,pf] | Read value of AC line parameters | - | + (auto-push every 1 minute) | Voltage (V)<br>Current (A)<br>Power (W)<br>Energy (kWh)<br>Frequency (Hz)<br>Power factor |
| L-[v,c,p,e,f,pf]-a | Read value of auto-push AC line parameters | - | - | 0 - disabled<br>1 - enabled |
| L-[v,c,p,e,f,pf]-a=[0,1] | Disable/enable auto-push for read values of AC line parameters | + | - | 0 - disable<br>1 - enable<br>default: 0 |

where,<br>
[V,I,P]-1 - 24V 10A PS output<br>
[V,I,P]-2 - 24V Solar Battaries output<br>
[V,I,P]-3 - 12V DC-DC (from UPS 2+3) output<br>
[V,I,P]-4 - 18V 20A PS output<br>
T-1 - 24V 10A PS<br>
T-2 - 12V DC-DC2 (from UPS 2+3), near 24V 10A PS<br>
T-3 - 12V DC-DC1 (from UPS 2+3), near 18V 20A PS<br>
T-4 - 18V 20A PS<br>
***EEPROM*** - memory values are kept when the board is turned off<br>
***Auto-push*** - periodically send data to server

## Device Photos

### UPS schema and common photo

[<img src="images/ps-monitoring_schema.jpeg" width="300"/>](images/ps-monitoring_schema.jpeg)
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
