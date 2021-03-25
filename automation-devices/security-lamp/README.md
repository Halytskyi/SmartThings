# Security Lamp

- [Security Lamp](#security-lamp)
  - [Description](#description)
    - [Main functions](#main-functions)
  - [Requirements and components](#requirements-and-components)
  - [Commands](#commands)

## Description

This device was developed as one of the part of automatization for smart-flat.

### Main functions

- **Led automodes**
  - Turn On WHITE leds when motion detected and reached "brightness limit" and send push (if "events" enabled);  
    If alarm NOT triggered and Disabled: Turn On GREEN leds when door opened and Turn Off after closed;  
    If alarm triggered or enabled: start blinking Blue/RED leds during 10 minutes and stop when alarm disabled.
  - Mode 1;  
    Voltage control.
  - Mode 2;  
    If reached limit from Ultrasonic sensor during 10 seconds, Turn On Camera (if Automode2 enabled), Turn Off WHITE leds (send push if "events" enabled) and blink 5 times Blue leds;  
    If still reached limit - start blinking RED leds. Send push "Ultrasonic sensor" status every 1 minute (if "events" enabled);  
    If stopped reached limit from Ultrasonic sensor, Turn Off blinking RED leds, send push (if "events" enabled), Turn Off Camera after 1 minute (if Automode2 enabled and no motion).
  - Mode 3;  
    Voltage control.  

## Requirements and components

**Controller:** Arduino Pro Mini 328 - 5V/16MHz

| Arduino PIN | Component | Notes |
| --- | --- | --- |
| RX, TX |     |     |
| D2 (Ext. Int.) | Open-door | Door sensor |
| D3 (PWM) | IRLR2905 | White LED |
| D4  |     |     |
| D5 (PWM) | IRLR2905 | Red LED |
| D6 (PWM) | IRLR2905 | Green LED |
| D7  | HC-SR04 (Trig) | Ultrasonic Sensor |
| D8  | HC-SR04 (Echo) | Ultrasonic Sensor |
| D9 (PWM) | IRLR2905 | Blue LED |
| D10 (PWM) | DHT22 | Temperature and humidity sensor |
| D11 (PWM) | PIR sensor | Motion sensor |
| D12 | PJON v11.0 ([SoftwareBitBang](https://github.com/gioblu/PJON/tree/master/strategies/SoftwareBitBang "SoftwareBitBang")) | Communication with Server |
| D13 | IRLR2905 | Camera |
| A0  | Light sensor (GL5528) | Light sensor |
| A1  | Voltmeter: r1=100k, r2=10k | Voltage of line (101.4k, 9.87k) |
| A2  |     |     |
| A3  |     |     |
| A4  |     |     |
| A5  |     |     |

## Commands

|     |     |     |     |     |
| --- | --- | --- | --- | --- |
| Command | Description | EEPROM | Auto push | Note |
| L-\[w,r,g,b\] | Read status of \[w,r,g,b\] leds | -   | -   | %, value from 0 to 100 |
| L-\[w,r,g,b\]-\[0-100\] | 0-100% brightness of \[w,r,g,b\] leds | -   | -   | %, value from 0 (default) to 100 |
| L-a | Read value of automode | -   | -   | 0 - disabled; 1 - enabled |
| L-a-\[0-4\] | Disable/Enable automode | +   | \+ (push value when status changed, if "events" enabled) | 0 - disable (default); 1-4 - enable |
| L-l-\[w,r,g,b\] | Read value of "max brightness" for automode of \[w,r,g,b\] leds | -   | -   | %, value from 0 to 100 |
| L-l-\[w,r,g,b\]-\[0-100\] | Define "max brightness" for automode of \[w,r,g,b\] leds | +   | -   | %, value from 0 to 100 (default) |
| T-\[t,f,h\] | Read \[temperature, feeling temperature, humidity\] | -   | \+ (auto push every 1 minute) | °C, °C, % (cellar, local) |
| T-\[t,f,h\]-a | Read value of "auto push" for DHT22 sensor | -   | -   | 0 - disabled; 1 - enabled |
| T-\[t,f,h\]-a-\[0,1\] | Disable/Enable "auto push" for DHT22 sensor | +   | -   | 0 - disable (default); 1 - enable |
| S-u | Read value of Ultrasonic sensor | -   | \+ (auto push every 1 minute if status changed) | cm, value from 2 to 400 |
| S-u-a | Read value of "auto push" for Ultrasonic sensor | -   | -   | 0 - disabled; 1 - enabled |
| S-u-a-\[0,1\] | Disable/Enable "auto push" for Ultrasonic sensor | +   | -   | 0 - disable (default); 1 - enable |
| S-u-l | Read value of "limit" for Ultrasonic sensor | -   | -   | cm, value from 2 to 255 |
| S-u-l-\[2-255\] | Define "limit" for Ultrasonic sensor | +   | -   | cm, value from 2 (default) to 255 |
| S-m | Read value of Motion sensor | -   | \+ (push value when status changed) | 0 - motion not detected; 1 - motion detected |
| S-m-a | Read value of "auto push" for Motion sensor | -   | -   | 0 - disabled; 1 - enabled |
| S-m-a-\[0,1\] | Disable/Enable "auto push" for Motion sensor | +   | -   | 0 - disable (default); 1 - enable |
| S-m-t | Read value of "time" for Motion sensor | -   | -   | Sec., value from 0 to 99 |
| S-m-t-\[0-99\] | Define "time" for Motion sensor | +   | -   | Sec., value from 0 (default \- disable "loop read") to 99 |
| S-l | Read value of Light sensor | -   | \+ (auto push every 1 minute) | %, value from 0 to 100 |
| S-l-a | Read value of automode (auto push) for Light sensor | -   | -   | 0 - disabled; 1 - enabled |
| S-l-a-\[0,1\] | Disable/Enable automode (auto push) for Light sensor | +   | -   | 0 - disable (default); 1 - enable |
| S-l-b | Read value of "brightness limit" for Light sensor | -   | -   | %, value from 0 to 100 |
| S-l-b-\[0-100\] | Define "brightness limit" for Light sensor | +   | -   | %, value from 0 (default) to 100 |
| C   | Read status of Camera | -   | -   | 0 - turned OFF; 1 - turned ON |
| C-\[0,1,2,3\] | OFF/ON/Automode1/Automode2 Camera  <br>Automode1 - Turn On Camera if alarm enabled or triggered; Off - if alarm disabled.  <br>Automode2 - On/Off camera in Lamp automode 3 | +   | -   | 0 - turn OFF (default); 1 - turn ON; 2 - automode1; 3 \- automode2 (for Lamp automode 3) |
| V   | Read value of voltage on line | -   | \+ (auto push every 1 minute) | V   |
| V-a | Read value of automode (auto push) for voltage on line | -   | -   | 0 - disabled; 1 - enabled |
| V-a-\[0,1\] | Disable/Enable automode (auto push) for read values ofvoltage on line | +   | -   | 0 - disable (default); 1 - enable |
| D   | Read value of Door sensor | -   | \+ (push value when status changed) | 0 - closed; 1 - opened |
| D-a | Read value of "auto push" for Door sensor | -   | -   | 0 - disabled; 1 - enabled |
| D-a-\[0,1\] | Disable/Enable "auto push" for Door sensor | +   | -   | 0 - disable (default); 1 - enable |
| A   | Read value of alarm | -   | -   | 0 - disabled; 1 - enabled; 2 - triggered |
| A-\[0,1,2\] | Alarm Disabled/Enabled/Triggered | +   | -   | 0 - disabled (default); 1 - enabled; 2 - triggered |
| E   | Read value of events | -   | -   | 0 - disabled; 1 - enabled |
| E-\[0,1\] | Events Disable/Enable | +   | -   | 0 - disable (default); 1 - enable |
