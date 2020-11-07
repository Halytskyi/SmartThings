# Low voltage UPS for smart home

## Description

This UPS was designed for smart home with separate two power supply lines and batteries for each line which include two inputs for chargers. For almost on each inputs and outputs measuring voltage, current and power consumption with ability to send data to server via [PJON protocol](https://github.com/gioblu/PJON).

## Main functions

UPS consists from 3 modules

[Module #1](module1) - control outputs and charge batteries<br>
[Module #2](module2) - UPS function (mixing voltage from batteries and power supplies)<br>
[Module #3](module3) - measuring voltage, curent and power consumption module

## Components photos and schematics

| Name | Schema / Photo |
| --- | --- |
| Automatic step UP/DOWN Regulator LTC3780 10A | [<img src="images/LTC3780_10A_1.jpg" alt="LTC3780_10A_1" width="300"/>](images/LTC3780_10A_1.jpg) [<img src="images/LTC3780_10A_2.jpg" alt="LTC3780_10A_2" width="213"/>](images/LTC3780_10A_2.jpg) |
| Automatic step UP/DOWN Regulator LTC3780 14A | [<img src="images/LTC3780_14A_1.jpg" alt="LTC3780_14A_1" width="270"/>](images/LTC3780_14A_1.jpg) [<img src="images/LTC3780_14A_2.jpg" alt="LTC3780_14A_2" width="220"/>](images/LTC3780_14A_2.jpg) |

## Device circuit

[<img src="images/slvu_circuit.jpg" alt="Device circuit" width="300"/>](images/slvu_circuit.jpg)

## Devices Photos

[<img src="images/slvu.jpg" alt="Device" width="300"/>](images/slvu.jpg)
