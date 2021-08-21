# Power Supply Monitoring

## Description

Power Supply Monitoring module was developed for measure voltage, current and power consumption on AC line and PS outputs with ability send data to server via [PJON protocol](https://github.com/gioblu/PJON).

## Main functions

- measure voltage, curent and power consumption on PS outputs and dc-dc converters;
- measure AC line parameters: Voltage, Current, Power, Energy, Frequency and Power factor;
- measuring temperature on PS outputs and dc-dc converters;
- send data to server via [PJON protocol](https://github.com/gioblu/PJON)

## PJON Specification

- PJON TxRx Bus Server ID: _1_
- PJON Tx Bus Server ID: _6_
- PJON Bus Device ID: _15_
- PJON Strategy: _SoftwareBitBang_

## Requirements and components

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
| A0 | Voltmeter: r1=100k, r2=10k | 12V 20A PS output (V-1) |
| A1 | Voltmeter: r1=100k, r2=10k | 5V DC-DC (from UPS 4) output (V-2) |
| A2 | Voltmeter: r1=100k, r2=10k | 5V DC-DC (from UPS 2+3) output (V-3) |
| A3 | Voltmeter: r1=100k, r2=10k | 12V 25A PS output (V-4) |
| A4 | ACS712-20A | 12V 20A PS output (I-1) |
| A5 | ACS712-20A | 5V DC-DC (from UPS 4) output (I-2) |
| A6 | ACS712-20A | 5V DC-DC (from UPS 2+3) output (I-3) |
| A7 | ACS712-20A | 12V 25A PS output (I-4) |

### Components photos and schematics

| Name | Schema / Photo |
| --- | --- |
| Voltmeter | [<img src="images/voltmeter.jpg" alt="Voltmeter" width="170"/>](images/voltmeter.jpg) |
| ACS712 | [<img src="images/ACS712_1.jpg" alt="ACS712_1" width="170"/>](images/ACS712_1.jpg) [<img src="images/ACS712_2.jpg" alt="ACS712_2" width="294"/>](images/ACS712_2.jpg) |
| HW-613 | [<img src="images/HW-613_1.jpg" alt="HW-613" width="130"/>](images/HW-613_1.jpg) [<img src="images/HW-613_2.jpg" alt="HW-613" width="152"/>](images/HW-613_2.jpg) |
| DS18B20 | [<img src="images/DS18B20.jpg" alt="DS18B20" width="330"/>](images/DS18B20.jpg) |
| PZEM-004T v3 | [<img src="images/PZEM-004T-v3.jpg" alt="PZEM-004T" width="330"/>](images/PZEM-004T-v3.jpg) |

## Commands

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
| T-[1-4] | Read value of temperature on PS outputs and dc-dc converters | - | + (auto-push every 1 minute) | °C |
| T-[1-4]-a | Read value of auto-push for temperature on PS outputs and dc-dc converters | - | - | 0 - disabled<br>1 - enabled |
| T-[1-4]-a=[0,1] | Disable/Enable auto-push for read values of temperature on PS outputs and dc-dc converters | + | - | 0 - disable<br>1 - enable<br>default: 0 |
| L-[v,c,p,e,f,pf] | Read value of AC line parameters | - | + (auto-push every 1 minute) | Voltage (V)<br>Current (A)<br>Power (W)<br>Energy (kWh)<br>Frequency (Hz)<br>Power factor |
| L-[v,c,p,e,f,pf]-a | Read value of auto-push AC line parameters | - | - | 0 - disabled<br>1 - enabled |
| L-[v,c,p,e,f,pf]-a=[0,1] | Disable/enable auto-push for read values of AC line parameters | + | - | 0 - disable<br>1 - enable<br>default: 0 |

where,<br>
[V,I,P]-1 - 12V 20A PS output<br>
[V,I,P]-2 - 5V DC-DC (from UPS 4) output<br>
[V,I,P]-3 - 5V DC-DC (from UPS 2+3) output<br>
[V,I,P]-4 - 12V 25A PS output<br>
T-1 - 12V 20A PS<br>
T-2 - 5V DC-DC (from UPS 4)<br>
T-3 - 5V DC-DC (from UPS 2+3)<br>
T-4 - 12V 25A PS<br>
***EEPROM*** - memory values are kept when the board is turned off<br>
***Auto-push*** - periodically send data to server

## Device Photos

### UPS schema and common photo

[<img src="images/ps-monitoring_schema.jpg" alt="PS monitoring" width="220"/>](images/ps-monitoring_schema.jpg)
[<img src="images/ps-monitoring_common.jpg" alt="PS monitoring" width="350"/>](images/ps-monitoring_common.jpg)

### Board version 2

**Note:** added [PZEM004T v3.0](https://innovatorsguru.com/pzem-004t-v3) and [PJON filters](https://github.com/gioblu/PJON/wiki/Mitigate-interference) (1MOm resistors and diode)

[<img src="images/ps-monitoring_1_v2.jpg" alt="PS monitoring" width="300"/>](images/ps-monitoring_1_v2.jpg)
[<img src="images/ps-monitoring_2_v2.jpg" alt="PS monitoring" width="296"/>](images/ps-monitoring_2_v2.jpg)
[<img src="images/ps-monitoring_3_v2.jpg" alt="PS monitoring" width="305"/>](images/ps-monitoring_3_v2.jpg)

### Board version 1

[<img src="images/ps-monitoring_1_v1.jpg" alt="PS monitoring" width="300"/>](images/ps-monitoring_1_v1.jpg)
[<img src="images/ps-monitoring_2_v1.jpg" alt="PS monitoring" width="310"/>](images/ps-monitoring_2_v1.jpg)
[<img src="images/ps-monitoring_3_v1.jpg" alt="PS monitoring" width="346"/>](images/ps-monitoring_3_v1.jpg)
[<img src="images/ps-monitoring_4_v1.jpg" alt="PS monitoring" width="326"/>](images/ps-monitoring_4_v1.jpg)