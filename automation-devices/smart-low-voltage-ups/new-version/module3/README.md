# Low voltage UPS for smart home - Module #3

## Main functions

- sensors for measuring voltage, curent and power consumption on inputs and outputs of module #2 with ability sending data to server via [PJON protocol](https://github.com/gioblu/PJON)

## PJON Specification

- PJON TxRx Bus Server ID: _1_
- PJON Tx Bus Server ID: _6_
- PJON Bus Device ID: _18_
- PJON Strategy: _SoftwareBitBang_

## Requirements and components

- 1 x Arduino Pro Mini 328 - 5V/16MHz
- 4 x ACS712-20A modules
- 4 x 10k resistors
- 4 x 100k resistors
- 2 x 8A fuses
- 2 x 12A fuses

| Arduino PIN | Component | Notes |
| --- | --- | --- |
| D2 (Ext. Int.) | - ||
| D3 (PWM) | - ||
| D4 | - ||
| D5 (PWM) | - ||
| D6 (PWM) | - ||
| D7 | [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0/src/strategies/SoftwareBitBang) | Communication with Server (TxRx) |
| D8 | - ||
| D9 (PWM) | - ||
| D10 (PWM) | - ||
| D11 (PWM) | - ||
| D12 | [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0/src/strategies/SoftwareBitBang) | Communication with Server (TX only) |
| D13 | - ||
| A0 | Voltmeter: r1=100k, r2=10k | UPS output #1 |
| A1 | Voltmeter: r1=100k, r2=10k | UPS output #2 |
| A2 | Voltmeter: r1=100k, r2=10k | UPS output #3 |
| A3 | Voltmeter: r1=100k, r2=10k | UPS output #4 |
| A4 | ACS712-20A | UPS output #3 |
| A5 | ACS712-20A | UPS output #4 |
| A6 | ACS712-20A | UPS output #1 |
| A7 | ACS712-20A | UPS output #2 |

### Components photos and schematics

| Name | Schema / Photo |
| --- | --- |
| Voltmeter | [<img src="../images_common/voltmeter.jpg" alt="Voltmeter" width="170"/>](../images_common/voltmeter.jpg) |
| ACS712 | [<img src="../images_common/ACS712_1.jpg" alt="ACS712_1" width="170"/>](../images_common/ACS712_1.jpg) [<img src="../images_common/ACS712_2.jpg" alt="ACS712_2" width="294"/>](../images_common/ACS712_2.jpg) |

## Commands

| Command | Description | EEPROM | Auto-push | Notes |
| --- | --- | --- | --- | --- |
| V-[1-4] | Read value of voltage for chargers and outputs | - | + (auto-push every 1 minute) | Volt |
| V-[1-4]-a | Read value of auto-push voltage for chargers and outputs | - | - | 0 - disabled<br>1 - enabled |
| V-[1-4]-a=[0,1] | Disable/enable auto-push for read values of voltage for chargers and outputs | + | - | 0 - disable<br>1 - enable<br>default: 0 |
| I-[1-4] | Read value of current for chargers and outputs | - | + (auto-push every 1 minute) | Amper |
| I-[1-4]-a | Read value of auto-push current for chargers and outputs | - | - | 0 - disabled<br>1 - enabled |
| I-[1-4]-a=[0,1] | Disable/enable auto-push for read values of current for chargers and outputs | + | - | 0 - disable<br>1 - enable<br>default: 0 |
| P-[1-4] | Read value of power consumption for chargers and outputs | - | + (auto-push every 1 minute) | Watt (Volt * Amper) |
| P-[1-4]-a | Read value of auto-push power consumption for chargers and outputs | - | - | 0 - disabled<br>1 - enabled |
| P-[1-4]-a=[0,1] | Disable/enable auto-push for read values of power consumption for chargers and outputs | + | - | 0 - disable<br>1 - enable<br>default: 0 |

where,<br>
[V,I,P]-1 - UPS output #1<br>
[V,I,P]-2 - UPS output #2<br>
[V,I,P]-3 - UPS output #3<br>
[V,I,P]-4 - UPS output #4<br>
***EEPROM*** - memory values are kept when the board is turned off<br>
***Auto-push*** - periodically send data to server

## Device Photos

[<img src="images/slvu_module3_1.jpg" alt="Module 3" width="300"/>](images/slvu_module3_1.jpg)
[<img src="images/slvu_module3_2.jpg" alt="Module 3" width="328"/>](images/slvu_module3_2.jpg)
[<img src="images/slvu_module3_3.jpg" alt="Module 3" width="347"/>](images/slvu_module3_3.jpg)
