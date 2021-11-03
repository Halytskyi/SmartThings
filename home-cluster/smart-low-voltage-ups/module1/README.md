# Low voltage UPS for smart home - Module #1

## Main functions

- control charge process for 2 batteries from DC or Solar inputs;
- two batteries can be charged simultaneously;
- start batteries charge if voltage >5V and <12.5V;
- charging will be enabled if input voltage to charger >=15.0V;
- stop charge batteries if current consumption from them >0.2A;
- stop charge batteries if input voltage <15V or charging current <0.15A or was received signal from charger status LED;
- measuring batteries temperature, don't start batteries charge if temperature >42°C and stop charge if >45°C;
- enable batteries outputs if voltage >12.0V, disable if batteries discharged (less 11.1V)
- measuring batteries temperature, don't enable outputs if batteries temperature >42°C and disable outputs if >45°C;
- measuring voltage, current and power consumption on batteries and chargers with ability sending data to server via [PJON protocol](https://github.com/gioblu/PJON);
- module and outputs can be configured and controlled from server via [PJON protocol](https://github.com/gioblu/PJON)

## PJON Specification

- PJON TxRx Bus Server ID: _1_
- PJON Tx Bus Server ID: _6_
- PJON Bus Device ID: _16_
- PJON Strategy: _SoftwareBitBang_

## Requirements and components for module #1.1

- 1 x Arduino Pro Mini 328 - 5V/16MHz
- 2 x IRF4905 transistors
- 2 x BC547 transistors
- 2 x 12v Zener diode (1N4742A)
- 2 x SR240 diodes
- 2 x SR540 diodes (before chargers input)
- 2 x 1k resistors
- 2 x 2k 2W resistors
- 8 x 10k resistors
- 2 x 100k resistors
- 2 x 420 Ohm resistors
- 2 x [MOSFET relays OMRON G3VM-61A1](https://www.aliexpress.com/item/1005001989679623.html?spm=a2g0s.9042311.0.0.27424c4dnmIQ2j)
- 2 x DS18B20 (for batteries)
- 2 x 1 MOm resistors
- 1 x 1N4001 diode
- 1 x HW-613 Mini DC-DC 3A Step Down Power Supply Module (for ACS712 and Arduino devices, 5V output)
- 2 x 15x10x25mm aluminum heatsinks (19.1V -> 18.4V, 3A (~55W) - 40℃; 22.6V -> 21.8V, 3A (~65W) - 42℃)
- 1 x 0.2A fuse (arduino, ammeter, etc)
- 1 x 70x90 PCB

| Arduino PIN | Component | Notes |
| --- | --- | --- |
| D2 (Ext. Int.) | Transistor switch 3 | Output 1 |
| D3 (PWM) | Transistor switch 4 | Output 2 |
| D4 | Transistor switch 5 | Output 3 |
| D5 (PWM) | Transistor switch 6 | Output 4 |
| D6 (PWM) | 1-Wire | Temperature sensors |
| D7 | [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0/src/strategies/SoftwareBitBang) | Communication with Server (TxRx) |
| D8 | Transistor switch 2 | Solar/DC charger #2 |
| D9 (PWM) | OMRON G3VM-61A1 | DC charger status LED #2 |
| D10 (PWM) | OMRON G3VM-61A1 | DC charger status LED #1 |
| D11 (PWM) | Transistor switch 1 | Solar/DC charger #1 |
| D12 | [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0/src/strategies/SoftwareBitBang) | Communication with Server (TX only) |
| D13 | - | - |
| A0 | ACS712-20A | Solar/DC charger #1 |
| A1 | ACS712-20A | Battery #1 (Transistor switch 3 and 4) |
| A2 | ACS712-20A | Battery #2 (Transistor switch 5 and 6) |
| A3 | ACS712-20A | Solar/DC charger #2 |
| A4 | Voltmeter: r1=100k, r2=10k | Battery #1; 10k=9.76k, 100k=98.7k |
| A5 | Voltmeter: r1=100k, r2=10k | Battery #2; 10k=9.9k, 100k=99.5k |
| A6 | Voltmeter: r1=100k, r2=10k | Input to DC charger #1; 10k=9.89k, 100k=100k |
| A7 | Voltmeter: r1=100k, r2=10k | Input to DC charger #2; 10k=10.9k, 100k=98.7k |

## Requirements and components for module #1.2

To avoid increase temperature above 100℃ on transistors and work with load up to 150W is required 2 MOSFET transistors per channel with related heatsinks.

- 8 x IRF4905 transistors
- 4 x BC547 transistors
- 4 x 1k resistors
- 10 x 10k resistors
- 2 x 100k resistors
- 4 x ACS712-20A modules
- 4 x 34x12x30mm heatsink for IRF4905, output 2 and 3 (12V -> 11.3V, 13.5A (~153W) - 89℃ on transistor and 101℃ on heatsink)
- 4 x 34x12x38mm heatsink for IRF4905, output 1 and 4 (12V -> 11.3V, 13.5A (~153W) - 83℃ on transistor and 93℃ on heatsink)

## Components photos and schematics

| Name | Schema / Photo |
| --- | --- |
| Switch (for module #1.1) | [<img src="images/Switch_24V.jpeg" alt="Switch" width="250"/>](images/Switch_24V.jpeg) [<img src="images/IRF4905.jpg" alt="IRF4905" width="202"/>](images/IRF4905.jpg) [<img src="images/BC547.jpg" alt="BC547" width="225"/>](images/BC547.jpg) |
| Switch (for module #1.2) | [<img src="images/Switch_two_IRF4905.jpeg" alt="Switch" width="300"/>](images/Switch_two_IRF4905.jpeg) |
| Voltmeter | [<img src="../images_common/voltmeter.jpg" alt="Voltmeter" width="170"/>](../images_common/voltmeter.jpg) |
| ACS712 | [<img src="../images_common/ACS712_1.jpg" alt="ACS712_1" width="170"/>](../images_common/ACS712_1.jpg) [<img src="../images_common/ACS712_2.jpg" alt="ACS712_2" width="294"/>](../images_common/ACS712_2.jpg) |
| HW-613 | [<img src="../images_common/HW-613_1.jpg" alt="HW-613" width="130"/>](../images_common/HW-613_1.jpg) [<img src="../images_common/HW-613_2.jpg" alt="HW-613" width="152"/>](../images_common/HW-613_2.jpg) |
| DS18B20 | [<img src="images/DS18B20.jpg" alt="DS18B20" width="330"/>](images/DS18B20.jpg) |
| OMRON G3VM-61A1 | [<img src="images/OMRON_G3VM-61A1_input.jpeg" width="330"/>](images/OMRON_G3VM-61A1_input.jpeg) |

### Chargers

Choose correct DC charger - it's very important step. I made mistake here when I chose charger "YH11059B on XL4005 chip" which doesn't work when battery "minus" connected to the same "minus" as charger (i.e. input and output "minus" of charger connected together), because this charger has shunt resistor between "minus" input and output, as result, the current does not pass through this shunt.

Photo of charger and schematic which very similar to that one:

[<img src="images/YH11059B-XL4005.jpeg" width="268"/>](images/YH11059B-XL4005.jpeg)
[<img src="images/XL4015-CC-CV-Buck-Module-schematic.jpeg" width="330"/>](images/XL4015-CC-CV-Buck-Module-schematic.jpeg)

After long searching the charger which should cover all my requirements I found awesome "DD30CRTA" which has very good characteristics and works well with common "minus" (GND):

[<img src="images/DD30CRTA_1.jpeg" width="220"/>](images/DD30CRTA_1.jpeg)
[<img src="images/DD30CRTA_2.jpeg" width="387"/>](images/DD30CRTA_2.jpeg)

It came with configured 3A charge current which is too much for my 9Ah AGM batteries. A rule of thumb for gel and AGM batteries states that the charging current should be 15 to 25 % of the battery capacity, which mean in my case 1.35 - 2.25A.  
I fixed that by solder out one of the two resistors R080 (0.08 Ohm) and in result I got 1.5A on output which is excellent for my batteries:

[<img src="images/DD30CRTA_3.jpeg" width="300"/>](images/DD30CRTA_3.jpeg)

Example of resistor values (120mV / RCS)

| RCS 1210 Resistor | Charging current |
| --- | --- |
| 0.24 Ohm (R240) | 0.5A |
| 0.12 Ohm (R120) | 1A |
| 0.08 Ohm (R080) | 1.5A |
| 0.06 Ohm (R060) | 2A |
| 0.04 Ohm (R040) | 3A |

Photos of mounts

[<img src="images/DD30CRTA_4.jpeg" width="344"/>](images/DD30CRTA_4.jpeg)
[<img src="images/DD30CRTA_5.jpeg" width="200"/>](images/DD30CRTA_5.jpeg)

Soldering wires to status LED for use it with controlled module

[<img src="images/DD30CRTA_6.jpeg" width="200"/>](images/DD30CRTA_6.jpeg)
[<img src="images/DD30CRTA_7.jpeg" width="342"/>](images/DD30CRTA_7.jpeg)

Heatmap during charging battery by 1.5A current (73.8℃)

[<img src="images/DD30CRTA_8.jpeg" width="250"/>](images/DD30CRTA_8.jpeg)

## Commands

| Command | Description | EEPROM | Auto-push | Notes |
| --- | --- | --- | --- | --- |
| C-[1-2] | Read value of DC charger state | - | - | 0 - disabled<br>1 - enabled |
| C-[1-2]=[0,1] | Disable/enable DC charger | - | - | 0 - disable<br>1 - enable<br>default: 0 |
| C-[1-2]-a | Read value of charger automode | - | - | 0 - disabled<br>1 - enabled (by current)<br>2 - enabled (by current/LED) |
| C-[1-2]-a=[0-2] | Disable/enable charger automode | + | - | 0 - disable<br>1 - enable (by current)<br>2 - enable (by current/LED)<br>default: 1 |
| C-a-m | Read value of charger automode messages | - | - | 0 - disabled<br>1 - "status" messages only<br>2 - "state" messages only<br>3 - "status" and "state" messages<br>Status template:<br>"C-a-m<C-[1-2]:\<status>"<br>Possible "status":<br>1 - everything is ok<br>2 - input voltage to DC charger less 15V<br>"I-[1,2]=value" - battery current consumption >0.2A<br>"T-[1-2]=value" - temperature on batteries >42℃<br>State template:<br>"C-a-m<C-[1-2]=[0,1]"|
| C-a-m=[0-3] | Disable/enable charger automode messages | + | - | 0 - disable<br>1 - "status" messages only<br>2 - "state" messages only<br>3 - "status" and "state" messages<br>default: 0 |
| O-[1-4] | Read value of outputs status | - | - | 0 - disabled<br>1 - enabled |
| O-[1-4]=[0,1] | Disable/enable outputs | - | - | 0 - disable<br>1 - enable<br>default: 0 |
| O-a | Read value of outputs control automode | - | - | 0 - disabled<br>1 - enabled |
| O-a=[0,1] | Disable/enable outputs control automode | + | - | 0 - disable<br>1 - enable<br>default: 1 |
| O-a-m | Read value of outputs control automode messages | - | - | 0 - disabled<br>1 - "status" messages only<br>2 - "state" messages only<br>3 - "status" and "state" messages<br>Status template:<br>"O-a-m<B-[1-2]:\<status>"<br>Possible "status:"<br>1 - everything is ok<br>"V-[1,2]=value" - batteries voltage < 11.1V<br>"T-[1-2]=value" - temperature on batteries >45℃<br>State template:<br>"O-a-m<O-[1-4]=[0,1]" |
| O-a-m=[0-3] | Disable/enable outputs control automode messages | + | - | 0 - disable<br>1 - "status" messages only<br>2 - "state" messages only<br>3 - "status" and "state" messages<br>default: 0 |
| T-c-[1-2] | Read value of temperature control | - | - | 0 - disabled<br>1 - enabled |
| T-c-[1-2]=[0,1] | Disable/enable temperature control | + | - | 0 - disable<br>1 - enable<br>default: 1<br>where:<br>T-c-1 - Battery 1<br>T-c-2 - Battery 2<br> |
| T-[1-2] | Read value of temperature on batteries | - | + (auto-push every 1 minute) | °C<br>T-1 - Battery 1<br>T-2 - Battery 2<br> |
| T-[1-2]-a | Read value of auto-push for temperature on batteries | - | - | 0 - disabled<br>1 - enabled |
| T-[1-2]-a=[0,1] | Disable/enable auto-push for read values of temperature on batteries | + | - | 0 - disable<br>1 - enable<br>default: 0 |
| V-[1-4] | Read value of input voltage for chargers and batteries outputs | - | + (auto-push every 1 minute) | Volt |
| V-[1-4]-a | Read value of auto-push input voltage for chargers and batteries outputs | - | - | 0 - disabled<br>1 - enabled |
| V-[1-4]-a=[0,1] | Disable/enable auto-push for read values of input voltage for chargers and batteries outputs | + | - | 0 - disable<br>1 - enable<br>default: 0 |
| I-[1-4] | Read value of current for chargers and batteries outputs | - | + (auto-push every 1 minute) | Amper |
| I-[1-4]-a | Read value of auto-push current for chargers and batteries outputs | - | - | 0 - disabled<br>1 - enabled |
| I-[1-4]-a=[0,1] | Disable/enable auto-push for read values of current for chargers and batteries outputs | + | - | 0 - disable<br>1 - enable<br>default: 0 |
| P-[1-4] | Read value of power consumption for chargers and batteries outputs | - | + (auto-push every 1 minute) | Watt (Volt * Amper) |
| P-[1-4]-a | Read value of auto-push power consumption for chargers and batteries outputs | - | - | 0 - disabled<br>1 - enabled |
| P-[1-4]-a=[0,1] | Disable/enable auto-push for read values of power consumption for chargers and batteries outputs | + | - | 0 - disable<br>1 - enable<br>default: 0 |

where,  
[V,I,P]-1 - Battery #1  
[V,I,P]-2 - Battery #2  
[V,I,P]-3 - DC charger #1  
[V,I,P]-4 - DC charger #2  
***EEPROM*** - memory values are kept when the board is turned off  
***Auto-push*** - periodically send data to server

## Charge logic diagram

[<img src="images/slvu_module1_charge_diagram.jpeg" width="700"/>](images/slvu_module1_charge_diagram.jpeg)

## Device Photos

### Module board 1.1

[<img src="images/slvu_module1.1_1.jpg" width="257"/>](images/slvu_module1-1_1.jpg)
[<img src="images/slvu_module1.1_2.jpg" width="450"/>](images/slvu_module1-1_2.jpg)
[<img src="images/slvu_module1.1_3.jpg" width="340"/>](images/slvu_module1-1_3.jpg)
[<img src="images/slvu_module1.1_4.jpg" width="348"/>](images/slvu_module1-1_4.jpg)
[<img src="images/slvu_module1.1_5.jpg" width="301"/>](images/slvu_module1-1_5.jpg)
[<img src="images/slvu_module1.1_6.jpg" width="362"/>](images/slvu_module1-1_6.jpg)
[<img src="images/slvu_module1.1_7.jpg" width="276"/>](images/slvu_module1-1_7.jpg)
[<img src="images/slvu_module1.1_8.jpg" width="353"/>](images/slvu_module1-1_8.jpg)

### Module board 1.2

[<img src="images/slvu_module1.2_1.jpg" width="350"/>](images/slvu_module1-2_1.jpg)
[<img src="images/slvu_module1.2_2.jpg" width="295"/>](images/slvu_module1-2_2.jpg)
[<img src="images/slvu_module1.2_3.jpg" width="340"/>](images/slvu_module1-2_3.jpg)
[<img src="images/slvu_module1.2_4.jpg" width="469"/>](images/slvu_module1-2_4.jpg)
[<img src="images/slvu_module1.2_5.jpg" width="312"/>](images/slvu_module1-2_5.jpg)
[<img src="images/slvu_module1.2_6.jpg" width="326"/>](images/slvu_module1-2_6.jpg)
