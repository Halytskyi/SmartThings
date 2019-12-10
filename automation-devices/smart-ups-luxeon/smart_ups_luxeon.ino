/*
* Copyright (C) 2018 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#define PJON_INCLUDE_TS true
#define TS_RESPONSE_TIME_OUT 100000
#include <PJON.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FanMonitor.h>
#include <PZEM004T.h>
#include <EEPROM.h>

SoftwareSerial HC12(10, 11); // (RX, TX) connect to TX,RX of HC-12 module
PJON<ThroughSerial> bus(31);
int master_id = 1;
int bus_receive_time = 10000;

#define ADDR_VERSION 255    //location of the software version in EEPROM
#define CURRENT_EEPROM_VERSION 1 //we are on revision 1 of the EEPROM storage structure (0xFF or 255 is never a valid value for version)

unsigned long prevMillis = millis();
unsigned long prevMillis_autopush = millis();
unsigned long lastUpdateMillis = millis();
char wrong_command_reply[16] = ">failed command";
unsigned long min_push_interval = 4000; // interval between push data to master
unsigned long push_interval = 60000; // push interval (each sensor read once within this interval)

// Temperature sensors
#define num_t_sensors 4
OneWire oneWire(4);
DallasTemperature sensors(&oneWire);
DeviceAddress t_sensors_addr[num_t_sensors] = {
    {0x28, 0xFF, 0x30, 0x78, 0x63, 0x15, 0x02, 0x96},
    {0x28, 0x6F, 0xF5, 0xCF, 0x02, 0x00, 0x00, 0x61},
    {0x28, 0xFF, 0x6D, 0xA0, 0x63, 0x15, 0x03, 0xAC},
    {0x28, 0xFF, 0x43, 0x9F, 0x63, 0x15, 0x03, 0x24}};
// {"sensor path", "auto push", "last update"}
String t_sensors[num_t_sensors][3] = {
    {"T-1", "0", "0"},
    {"T-2", "0", "0"},
    {"T-3", "0", "0"},
    {"T-4", "0", "0"}};
// {"auto push"}
int eeprom_t_sensors_addr[num_t_sensors] = {1, 2, 3, 4};

int fan_pin = 9;
int fan_hallsensor = 2;
int auto_fanControl = 0;
int eeprom_auto_fanControl_addr = 5;
int tempC1_auto_fanControl = 0;
int tempLowLimit = 35; // temperature to start the fan
int eeprom_tempLowLimit_addr = 6;
int tempHighLimit = 55; // maximum temperature when fan is at 99%
int eeprom_tempHighLimit_addr = 7;
int fanSpeedPercent = 0; // Fan speed in percent
int fanSpeedRPM = 0; // Fan speed in RPM
#define num_fanSpeed 2
// {"cmd path", "auto push", "last update"}
String fanSpeed_arr[num_fanSpeed][3] = {
    {"F-p", "0", "0"},
    {"F-r", "0", "0"}};
// {"auto push"}
int eeprom_fanSpeed_addr[num_fanSpeed] = {8, 9};
FanMonitor _fanMonitor = FanMonitor(fan_hallsensor, FAN_TYPE_BIPOLE);

int voltage_pin = 0;
int current_pin = 3;
float current_sensetivity = 0.04; // set scale factor 40mV/A for 50A bi-directional (ACS758LCB-050B)
int current_offset = 508;
const float current_vpp = 0.0048828125;
int current_countValues = 15;
#define num_acc_param 3
// {"cmd path", "auto push", "last update"}
String acc_param[num_acc_param][3] = {
    {"A-v", "0", "0"},
    {"A-i", "0", "0"},
    {"A-p", "0", "0"}};
// {"auto push"}
int eeprom_acc_param_addr[num_acc_param] = {10, 11, 12};

PZEM004T pzem(7,8);  // (RX,TX) connect to TX,RX of PZEM
IPAddress ip(192,168,1,1);
#define num_out_param 4
// {"cmd path", "auto push", "last update"}
String out_param[num_out_param][3] = {
    {"O-v", "0", "0"},
    {"O-i", "0", "0"},
    {"O-p", "0", "0"},
    {"O-e", "0", "0"}};
// {"auto push"}
int eeprom_out_param_addr[num_out_param] = {13, 14, 15, 16};


String get_voltage() {
  float r1 = 101000; // R1 = 100k
  float r2 = 10030; // R2 = 10k
  float typVbg = 1.09; // calibration, constant for voltmeter (1.0 -- 1.2)
  int i;
  float Vcc = 0.0;
  float tmp = 0.0;
  int countvalues = 5; // how many values must be averaged
  float curVoltage = 0.0;
  String voltage_value;

  for (i = 0; i < 5; i++) {
    // Read 1.1V reference against AVcc
    // set the reference to Vcc and the measurement to the internal 1.1V reference
    #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
      ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
      ADMUX = _BV(MUX5) | _BV(MUX0);
    #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
      ADMUX = _BV(MUX3) | _BV(MUX2);
    #else
      // works on an Arduino 168 or 328
      ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    #endif

    delay(3); // Wait for Vref to settle
    ADCSRA |= _BV(ADSC); // Start conversion
    while (bit_is_set(ADCSRA,ADSC)); // measuring

    uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
    uint8_t high = ADCH; // unlocks both

    tmp = (high<<8) | low;
    tmp = (typVbg * 1023.0) / tmp;
    Vcc = Vcc + tmp;
    delay(5);
  }
  Vcc = Vcc / 5;

  for (i = 0; i < countvalues; i++) {
    curVoltage = curVoltage + analogRead(voltage_pin);
    delay(10);
  }
  curVoltage = curVoltage / countvalues;
  float v  = (curVoltage * Vcc) / 1024.0;
  voltage_value = v / (r2 / (r1 + r2));
  return voltage_value;
}

String get_current() {
  int sensorValue = 0;
  String current_value;
  for (int count = 0; count < current_countValues; count++) {
    sensorValue += analogRead(current_pin);
  }
  int points = sensorValue/current_countValues - current_offset;
  float voltage = points * current_vpp;
  current_value = voltage/current_sensetivity;
  return current_value;
}

String get_power_consuming() {
  float volt = get_voltage().toFloat();
  float current = get_current().toFloat();
  float power_consuming_value = volt * current;
  return String(power_consuming_value);
}

String get_output_values(String arg) {
  String value;
  if (arg == "O-v") {
    value = pzem.voltage(ip);
    if (value.toFloat() < 0.0) value = 0.0;
  } else if (arg == "O-i") {
    value = pzem.current(ip);
    if (value.toFloat() < 0.0) value = 0.0;
  } else if (arg == "O-p") {
    value = pzem.power(ip);
    if (value.toFloat() < 0.0) value = 0.0;
  } else if (arg == "O-e") {
    value = pzem.energy(ip);
    if (value.toFloat() < 0.0) value = 0.0;
  }
  HC12.listen();
  return value;
}

void bus_reply(String response_str) {
  int response_str_len = response_str.length();
  char response[response_str_len + 1];
  response_str.toCharArray(response, response_str_len + 1);
  bus.reply(response, response_str_len);
  bus.update();
}

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  if ((char)payload[0] == 'T') {
    if ((char)payload[1] == '-') {
      int replied = 0;
      for (int i = 0; i < num_t_sensors; i += 1) {
        String sensor_path = "T-" + String((char)payload[2]);
        if (sensor_path == t_sensors[i][0]) {
          if ((char)payload[3] == '-') {
            if ((char)payload[4] == 'a') {
              if ((char)payload[5] == '-') {
                if ((char)payload[6] == '0' or (char)payload[6] == '1') {
                  String command = String((char)payload[6]);
                  if (isDigit(payload[7]) or (char)payload[7] == '-') {
                    bus.reply(wrong_command_reply, 15);
                    bus.update();
                    replied = 1;
                    break;
                  }
                  t_sensors[i][1] = command;
                  EEPROM.update(eeprom_t_sensors_addr[i], command.toInt());
                  String response_str = sensor_path + "-a-" + command + ">" + command;
                  bus_reply(response_str);
                  replied = 1;
                  break;
                } else {
                  bus.reply(wrong_command_reply, 15);
                  bus.update();
                  replied = 1;
                  break;
                }
              } else {
                String response_str = sensor_path + "-a>" + t_sensors[i][1];
                bus_reply(response_str);
                replied = 1;
                break;
              }
            } else {
              bus.reply(wrong_command_reply, 15);
              bus.update();
              replied = 1;
              break;
            }
          } else {
            sensors.requestTemperaturesByAddress(t_sensors_addr[i]);
            String value = String(sensors.getTempC(t_sensors_addr[i]));
            String response_str = sensor_path + ">" + value;
            bus_reply(response_str);
            replied = 1;
            break;
          }
        }
      }
      if (replied == 0) {
        bus.reply(wrong_command_reply, 15);
        bus.update();
      }
    } else {
      bus.reply(wrong_command_reply, 15);
      bus.update();
    }
  } else if ((char)payload[0] == 'F') {
    if ((char)payload[1] == '-') {
      if ((char)payload[2] == 'c') {
        if ((char)payload[3] == '-') {
          if (isDigit(payload[4])) {
            String value = String((char)payload[4]);
            if (isDigit(payload[5])) {
              value += String((char)payload[5]);
              if (isDigit(payload[6])) {
                value += String((char)payload[6]);
              }
            }
            int value_int = value.toInt();
            if (value_int >= 0 and value_int <= 101) {
              if (value_int == 0) {
                digitalWrite(fan_pin, 0);
                fanSpeedPercent = 0;
                bus.reply("F-c-0>0", 7);
                bus.update();
              } else if (value_int == 100) {
                digitalWrite(fan_pin, 1);
                fanSpeedPercent = 100;
                bus.reply("F-c-100>100", 11);
                bus.update();
              } else if (value_int == 101) {
                tempC1_auto_fanControl = 0;
                bus.reply("F-c-101>101", 11);
                bus.update();
              } else {
                int fanSpeed = map(value_int, 1, 99, 70, 253);
                analogWrite(fan_pin, fanSpeed);
                fanSpeedPercent = value_int;
                String response_str = "F-c-" + value + ">" + value;
                bus_reply(response_str);
              }
              auto_fanControl = value_int;
              EEPROM.update(eeprom_auto_fanControl_addr, value_int);
            } else {
              bus.reply(wrong_command_reply, 15);
              bus.update();
            }
          } else if ((char)payload[4] == 't') {
            if ((char)payload[5] == 'l') {
              if ((char)payload[6] == '-') {
                if (isDigit(payload[7])) {
                  String value = String((char)payload[7]);
                  if (isDigit(payload[8])) {
                    value += String((char)payload[8]);
                    int value_int = value.toInt();
                    if (value_int >= 20 and value_int <= 40) {
                      tempLowLimit = value_int;
                      EEPROM.update(eeprom_tempLowLimit_addr, value_int);
                      String response_str = "F-c-tl-" + value + ">" + value;
                      bus_reply(response_str);
                    } else {
                      bus.reply(wrong_command_reply, 15);
                      bus.update();
                    }
                  } else {
                    bus.reply(wrong_command_reply, 15);
                    bus.update();
                  }
                } else {
                  bus.reply(wrong_command_reply, 15);
                  bus.update();
                }
              } else {
                String response_str = "F-c-tl>" + String(tempLowLimit);
                bus_reply(response_str);
              }
            } else if ((char)payload[5] == 'h') {
              if ((char)payload[6] == '-') {
                if (isDigit(payload[7])) {
                  String value = String((char)payload[7]);
                  if (isDigit(payload[8])) {
                    value += String((char)payload[8]);
                    int value_int = value.toInt();
                    if (value_int >= 41 and value_int <= 70) {
                      tempHighLimit = value_int;
                      EEPROM.update(eeprom_tempHighLimit_addr, value_int);
                      String response_str = "F-c-th-" + value + ">" + value;
                      bus_reply(response_str);
                    } else {
                      bus.reply(wrong_command_reply, 15);
                      bus.update();
                    }
                  } else {
                    bus.reply(wrong_command_reply, 15);
                    bus.update();
                  }
                } else {
                  bus.reply(wrong_command_reply, 15);
                  bus.update();
                }
              } else {
                String response_str = "F-c-th>" + String(tempHighLimit);
                bus_reply(response_str);
              }
            } else {
              bus.reply(wrong_command_reply, 15);
              bus.update();
            }
          } else {
            bus.reply(wrong_command_reply, 15);
            bus.update();
          }
        } else {
          String response_str = "F-c>" + String(auto_fanControl);
          bus_reply(response_str);
        }
      } else if ((char)payload[2] == 'p' or (char)payload[2] == 'r') {
        int replied = 0;
        for (int i = 0; i < num_fanSpeed; i += 1) {
          String cmd_path = "F-" + String((char)payload[2]);
          if (cmd_path == fanSpeed_arr[i][0]) {
            if ((char)payload[3] == '-') {
              if ((char)payload[4] == 'a') {
                if ((char)payload[5] == '-') {
                  if ((char)payload[6] == '0' or (char)payload[6] == '1') {
                    String command = String((char)payload[6]);
                    fanSpeed_arr[i][1] = command;
                    EEPROM.update(eeprom_fanSpeed_addr[i], command.toInt());
                    String response_str = cmd_path + "-a-" + command + ">" + command;
                    bus_reply(response_str);
                    replied = 1;
                    break;
                  } else {
                    bus.reply(wrong_command_reply, 15);
                    bus.update();
                    replied = 1;
                    break;
                  }
                } else {
                  String response_str = cmd_path + "-a>" + fanSpeed_arr[i][1];
                  bus_reply(response_str);
                  replied = 1;
                  break;
                }
              } else {
                bus.reply(wrong_command_reply, 15);
                bus.update();
                replied = 1;
                break;
              }
            } else {
              String value;
              if ((char)payload[2] == 'p') {
                value = String(fanSpeedPercent);
              } else if ((char)payload[2] == 'r') {
                if (fanSpeedPercent == 0) {
                  fanSpeedRPM = 0;
                } else {
                  fanSpeedRPM = _fanMonitor.getSpeed();
                }
                value = String(fanSpeedRPM);
              }
              String response_str = cmd_path + ">" + value;
              bus_reply(response_str);
              replied = 1;
              break;
            }
          }
        }
        if (replied == 0) {
          bus.reply(wrong_command_reply, 15);
          bus.update();
        }
      } else {
        bus.reply(wrong_command_reply, 15);
        bus.update();
      }
    } else {
      bus.reply(wrong_command_reply, 15);
      bus.update();
    }
  } else if ((char)payload[0] == 'A') {
    if ((char)payload[1] == '-') {
      if ((char)payload[2] == 'v' or (char)payload[2] == 'i' or (char)payload[2] == 'p') {
        int replied = 0;
        for (int i = 0; i < num_acc_param; i += 1) {
          String cmd_path = "A-" + String((char)payload[2]);
          if (cmd_path == acc_param[i][0]) {
            if ((char)payload[3] == '-') {
              if ((char)payload[4] == 'a') {
                if ((char)payload[5] == '-') {
                  if ((char)payload[6] == '0' or (char)payload[6] == '1') {
                    String command = String((char)payload[6]);
                    acc_param[i][1] = command;
                    EEPROM.update(eeprom_acc_param_addr[i], command.toInt());
                    String response_str = cmd_path + "-a-" + command + ">" + command;
                    bus_reply(response_str);
                    replied = 1;
                    break;
                  } else {
                    bus.reply(wrong_command_reply, 15);
                    bus.update();
                    replied = 1;
                    break;
                  }
                } else {
                  String response_str = cmd_path + "-a>" + acc_param[i][1];
                  bus_reply(response_str);
                  replied = 1;
                  break;
                }
              } else {
                bus.reply(wrong_command_reply, 15);
                bus.update();
                replied = 1;
                break;
              }
            } else {
              String value;
              if ((char)payload[2] == 'v') {
                value = get_voltage();
              } else if ((char)payload[2] == 'i') {
                value = get_current();
              } else if ((char)payload[2] == 'p') {
                value = get_power_consuming();
              }
              String response_str = cmd_path + ">" + value;
              bus_reply(response_str);
              replied = 1;
              break;
            }
          }
        }
        if (replied == 0) {
          bus.reply(wrong_command_reply, 15);
          bus.update();
        }
      } else {
        bus.reply(wrong_command_reply, 15);
        bus.update();
      }
    } else {
      bus.reply(wrong_command_reply, 15);
      bus.update();
    }
  } else if ((char)payload[0] == 'O') {
    if ((char)payload[1] == '-') {
      if ((char)payload[2] == 'v' or (char)payload[2] == 'i' or (char)payload[2] == 'p' or (char)payload[2] == 'e') {
        int replied = 0;
        for (int i = 0; i < num_out_param; i += 1) {
          String cmd_path = "O-" + String((char)payload[2]);
          if (cmd_path == out_param[i][0]) {
            if ((char)payload[3] == '-') {
              if ((char)payload[4] == 'a') {
                if ((char)payload[5] == '-') {
                  if ((char)payload[6] == '0' or (char)payload[6] == '1') {
                    String command = String((char)payload[6]);
                    out_param[i][1] = command;
                    EEPROM.update(eeprom_out_param_addr[i], command.toInt());
                    String response_str = cmd_path + "-a-" + command + ">" + command;
                    bus_reply(response_str);
                    replied = 1;
                    break;
                  } else {
                    bus.reply(wrong_command_reply, 15);
                    bus.update();
                    replied = 1;
                    break;
                  }
                } else {
                  String response_str = cmd_path + "-a>" + out_param[i][1];
                  bus_reply(response_str);
                  replied = 1;
                  break;
                }
              } else {
                bus.reply(wrong_command_reply, 15);
                bus.update();
                replied = 1;
                break;
              }
            } else {
              String value = get_output_values(out_param[i][0]);
              String response_str = cmd_path + ">" + value;
              bus_reply(response_str);
              replied = 1;
              break;
            }
          }
        }
        if (replied == 0) {
          bus.reply(wrong_command_reply, 15);
          bus.update();
        }
      } else {
        bus.reply(wrong_command_reply, 15);
        bus.update();
      }
    } else {
      bus.reply(wrong_command_reply, 15);
      bus.update();
    }
  } else {
    bus.reply(wrong_command_reply, 15);
    bus.update();
  }
};

void bus_send(String response_str) {
  int response_str_len = response_str.length();
  char response[response_str_len + 1];
  response_str.toCharArray(response, response_str_len + 1);
  bus.send(master_id, response, response_str_len);
  bus.update();
  bus.receive(bus_receive_time);
}

void autopush() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillis_autopush >= min_push_interval) {
    int msg_pushed = 0;
    for (int i = 0; i < num_t_sensors; i += 1) {
      if (t_sensors[i][1] == "1") {
        if (curMillis - t_sensors[i][2].toInt() >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
          sensors.requestTemperaturesByAddress(t_sensors_addr[i]);
          String value = String(sensors.getTempC(t_sensors_addr[i]));
          t_sensors[i][2] = curMillis;
          lastUpdateMillis = curMillis;
          String response_str = t_sensors[i][0] + "<" + value;
          bus_send(response_str);
          msg_pushed = 1;
        }
      }
    }
    if (msg_pushed == 0) {
      for (int i = 0; i < num_fanSpeed; i += 1) {
        if (fanSpeed_arr[i][1] == "1") {
          if (curMillis - fanSpeed_arr[i][2].toInt() >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
            String value;
            if (fanSpeed_arr[i][0] == "F-p") {
              value = String(fanSpeedPercent);
            } else if (fanSpeed_arr[i][0] == "F-r") {
              if (fanSpeedPercent == 0) {
                fanSpeedRPM = 0;
              } else {
                fanSpeedRPM = _fanMonitor.getSpeed();
              }
              value = String(fanSpeedRPM);
            }
            fanSpeed_arr[i][2] = curMillis;
            lastUpdateMillis = curMillis;
            String response_str = fanSpeed_arr[i][0] + "<" + value;
            bus_send(response_str);
            msg_pushed = 1;
          }
        }
      }
    }
    if (msg_pushed == 0) {
      for (int i = 0; i < num_acc_param; i += 1) {
        if (acc_param[i][1] == "1") {
          if (curMillis - acc_param[i][2].toInt() >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
            String value;
            if (acc_param[i][0] == "A-v") {
              value = get_voltage();
            } else if (acc_param[i][0] == "A-i") {
              value = get_current();
            } else if (acc_param[i][0] == "A-p") {
              value = get_power_consuming();
            }
            acc_param[i][2] = curMillis;
            lastUpdateMillis = curMillis;
            String response_str = acc_param[i][0] + "<" + value;
            bus_send(response_str);
            msg_pushed = 1;
          }
        }
      }
    }
    if (msg_pushed == 0) {
      for (int i = 0; i < num_out_param; i += 1) {
        if (out_param[i][1] == "1") {
          if (curMillis - out_param[i][2].toInt() >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
            String value = get_output_values(out_param[i][0]);
            out_param[i][2] = curMillis;
            lastUpdateMillis = curMillis;
            String response_str = out_param[i][0] + "<" + value;
            bus_send(response_str);
            msg_pushed = 1;
          }
        }
      }
    }
    prevMillis_autopush = curMillis;
  }
}

// http://playground.arduino.cc/Code/PwmFrequency
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x07; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

void fan_control() {
  if (auto_fanControl == 101) {
    sensors.requestTemperaturesByAddress(t_sensors_addr[0]);
    int tempC1 = sensors.getTempC(t_sensors_addr[0]);
    if (tempC1 != tempC1_auto_fanControl) {
      if (tempC1 < tempLowLimit) {
        fanSpeedPercent = 0;
        digitalWrite(fan_pin, LOW);
      } else if (tempC1 > tempHighLimit) {
        fanSpeedPercent = 100;
        digitalWrite(fan_pin, HIGH);
      } else if ((tempC1 >= tempLowLimit) and (tempC1 <= tempHighLimit)) {
        int fanSpeed = map(tempC1, tempLowLimit, tempHighLimit, 70, 253); // define fan speed in analog value
        fanSpeedPercent = map(fanSpeed, 70, 253, 1, 99); // fan speed in percent
        analogWrite(fan_pin, fanSpeed); // spin the fan at the fanSpeed speed
      }
    }
    tempC1_auto_fanControl = tempC1;
  }
}


void loop() {
  bus.receive(bus_receive_time);
  bus.update();
  autopush();

  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillis >= 10000) {
    bus.receive(bus_receive_time);
    bus.update();
    fan_control();
    prevMillis = curMillis;
  }
}

void setup() {
  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) {
    //EEprom is wrong version or was not programmed, write default values to the EEprom
    for (int i = 0; i < num_t_sensors; i += 1) {
      EEPROM.update(eeprom_t_sensors_addr[i], t_sensors[i][1].toInt());
    }
    EEPROM.update(eeprom_auto_fanControl_addr, auto_fanControl);
    EEPROM.update(eeprom_tempLowLimit_addr, tempLowLimit);
    EEPROM.update(eeprom_tempHighLimit_addr, tempHighLimit);
    for (int i = 0; i < num_fanSpeed; i += 1) {
      EEPROM.update(eeprom_fanSpeed_addr[i], fanSpeed_arr[i][1].toInt());
    }
    for (int i = 0; i < num_acc_param; i += 1) {
      EEPROM.update(eeprom_acc_param_addr[i], acc_param[i][1].toInt());
    }
    for (int i = 0; i < num_out_param; i += 1) {
      EEPROM.update(eeprom_out_param_addr[i], out_param[i][1].toInt());
    }
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    for (int i = 0; i < num_t_sensors; i += 1) {
      t_sensors[i][1] = EEPROM.read(eeprom_t_sensors_addr[i]);
    }
    auto_fanControl = EEPROM.read(eeprom_auto_fanControl_addr);
    tempLowLimit = EEPROM.read(eeprom_tempLowLimit_addr);
    tempHighLimit = EEPROM.read(eeprom_tempHighLimit_addr);
    for (int i = 0; i < num_fanSpeed; i += 1) {
      fanSpeed_arr[i][1] = EEPROM.read(eeprom_fanSpeed_addr[i]);
    }
    for (int i = 0; i < num_acc_param; i += 1) {
      acc_param[i][1] = EEPROM.read(eeprom_acc_param_addr[i]);
    }
    for (int i = 0; i < num_out_param; i += 1) {
      out_param[i][1] = EEPROM.read(eeprom_out_param_addr[i]);
    }
  }

  // Set temperature sensor resolution, valid values are 9, 10, 11 or 12 bit.
  for (int i = 0; i < num_t_sensors; i += 1) {
    sensors.setResolution(t_sensors_addr[i], 11);
  }

  pinMode(fan_pin, OUTPUT);
  setPwmFrequency(fan_pin, 1024); // Set fan_pin PWM frequency to 31 Hz (31250/1024 = 31)
  _fanMonitor.begin();
  if (auto_fanControl != 101) {
    if (auto_fanControl == 0) {
      fanSpeedPercent = 0;
      digitalWrite(fan_pin, 0);
    } else if (auto_fanControl == 100) {
      fanSpeedPercent = 100;
      digitalWrite(fan_pin, 1);
    } else {
      int fanSpeed = map(auto_fanControl, 1, 99, 70, 253);
      fanSpeedPercent = auto_fanControl;
      analogWrite(fan_pin, fanSpeed);
    }
  }

  // PZEM004T module
  pzem.setAddress(ip);

  HC12.begin(9600);
  bus.strategy.set_serial(&HC12);
  bus.set_receiver(receiver_function);
  bus.set_synchronous_acknowledge(true);
  bus.set_crc_32(true);
  bus.begin();
}
