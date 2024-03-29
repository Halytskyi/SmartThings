# Rack Alarm System

## Description

### Main functions

- send messages to server (via [PJON protocol](https://github.com/gioblu/PJON)) when detect smoke, flame or external motion;
- send "status" message to server each 1 minute;
- send message to server and HIGH signal within 1 minute after last any sensor triggered (except A5 external motion sensor). Can be used, for example, for turn off the air-conditioner system, rack/cluster cooling system, etc.

### PJON Specification

- PJON Tx Bus Server ID: _6_
- PJON Bus Device ID: _19_
- PJON Strategy: _SoftwareBitBang_

## Requirements and components

- 1 x Arduino Pro Mini 328 - 5V/16MHz
- 9 x flame sensor modules
- 5 x MQ-2 smoke sensor modules
- 1(2) x HC-SR501 pir sensor(s)
- 1 x buzzer
- 1 x 10kΩ resistor
- 1 x 100kΩ resistor (for external two motion sensors on the "external sensors" board)
- 1 x 1MΩ resistor
- 3 x 1N4001 diode (2 of them for external two motion sensors)
- HW-613 Mini DC-DC 3A Step Down Power Supply Module (for Arduino and sensors, 5V output)
- 1 x 0.5A fuse

| Arduino PIN | Component | Notes |
| --- | --- | --- |
| D2 (Ext. Int.) | flame sensor | Front side, sensor #1 |
| D3 (PWM) | flame sensor | Front side, sensor #2 |
| D4 | flame sensor | Left door, sensor #1 |
| D5 (PWM) | flame sensor | Left door, sensor #2 |
| D6 (PWM) | flame sensor | Back side, sensor #1 |
| D7 | flame sensor | Back side, sensor #2 |
| D8 | flame sensor | Right door, sensor #1 |
| D9 (PWM) | flame sensor | Right door, sensor #2 |
| D10 (PWM) | buzzer ||
| D11 (PWM) | flame sensor | External |
| D12 | [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0/src/strategies/SoftwareBitBang) | Communication with Server (TX only) |
| D13 | signal output ||
| A0 | MQ-2 smoke sensor | Front side |
| A1 | MQ-2 smoke sensor | Back side |
| A2 | MQ-2 smoke sensor ||
| A3 | MQ-2 smoke sensor ||
| A4 | MQ-2 smoke sensor | External |
| A5 | Motion sensor(s) | External, used with 10kΩ pull-down resistor |

### Components photos and schematics

| Name | Schema / Photo |
| --- | --- |
| Flame sensor | [<img src="images/Flame_sensor.jpg" alt="Flame Sensor" width="200"/>](images/Flame_sensor.jpg) |
| MQ-2 smoke sensor | [<img src="images/MQ2_sensor1.jpg" alt="MQ2 sensor" width="200"/>](images/MQ2_sensor1.jpg) [<img src="images/MQ2_sensor2.jpg" alt="MQ2 sensor" width="168"/>](images/MQ2_sensor2.jpg) |
| Buzzer | [<img src="images/Buzzer.jpg" alt="Buzzer" width="200"/>](images/Buzzer.jpg) |
| HW-613 | [<img src="images/HW-613_1.jpg" alt="HW-613" width="130"/>](images/HW-613_1.jpg) [<img src="images/HW-613_2.jpg" alt="HW-613" width="152"/>](images/HW-613_2.jpg) |
| Motion sensor | [<img src="images/motion_sensor.jpg" width="200"/>](images/motion_sensor.jpg) |
| Multiple motion sensors | [<img src="images/multiple_motion_sensors.jpg" width="300"/>](images/multiple_motion_sensors.jpg) |

| Command | Description | Notes |
| --- | --- | --- |
| F-[1-9] | Flame sensors status (D2-D9, D11) | 0 - not triggered, 1 - triggered |
| S-[1-5] | Flame sensors status (A0-A4) | 0 - not triggered, 1 - triggered |
| M | Motion sensor(s) status (A5) | 0 - not triggered, 1 - triggered |
| A | Alarm status for the last 1 minute | 0 - no alarm, 1 - alarm |

**Note:** Motion sensors (HC-SR501) configuration: timer set to the middle and sensitivity set to the minimum, the trigger jumper set to H.

## Device Photos

[<img src="images/rack-alarm_1.jpeg" width="306"/>](images/rack-alarm_1.jpeg)
[<img src="images/rack-alarm_2.jpeg" width="300"/>](images/rack-alarm_2.jpeg)
[<img src="images/rack-alarm_3.jpeg" width="310"/>](images/rack-alarm_3.jpeg)
[<img src="images/rack-alarm_4.jpeg" width="306"/>](images/rack-alarm_4.jpeg)

## Sensors photos

[<img src="images/sensors_1.jpeg" width="300"/>](images/sensors_1.jpeg)
[<img src="images/sensors_2.jpeg" width="304"/>](images/sensors_2.jpeg)
[<img src="images/sensors_3.jpeg" width="261"/>](images/sensors_3.jpeg)
[<img src="images/sensors_4.jpeg" width="227"/>](images/sensors_4.jpeg)
[<img src="images/sensors_5.jpeg" width="143"/>](images/sensors_5.jpeg)
[<img src="images/sensors_6.jpeg" width="143"/>](images/sensors_6.jpeg)
[<img src="images/sensors_7.jpeg" width="440"/>](images/sensors_7.jpeg)
