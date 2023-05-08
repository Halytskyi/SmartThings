/*
* Copyright (C) 2018 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#define PJON_INCLUDE_SWBB true // Include SoftwareBitBang
#include <PJON.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PZEM004T.h>
#include <EEPROM.h>

PJON<SoftwareBitBang> bus(14);
int master_id = 1;
int bus_receive_time = 7000;

#define ADDR_VERSION 255    //location of the software version in EEPROM
#define CURRENT_EEPROM_VERSION 1 //we are on revision 1 of the EEPROM storage structure (0xFF or 255 is never a valid value for version)

unsigned long prevMillis = millis();
unsigned long prevMillis_autopush = millis();
unsigned long prevMillis_buttons = millis();
unsigned long lastUpdateMillis = millis();
char wrong_command_reply[16] = ">failed command";
unsigned long min_push_interval = 3000; // interval between push data to master
unsigned long push_interval = 60000; // push interval (each sensor read once within this interval)

// Temperature sensors
#define num_t_sensors 3
OneWire oneWire(2);
DallasTemperature sensors(&oneWire);
DeviceAddress t_sensors_addr[num_t_sensors] = {
    {0x28, 0xFF, 0x56, 0xA0, 0x63, 0x15, 0x03, 0x58},
    {0x28, 0xFF, 0x4E, 0x73, 0x63, 0x15, 0x02, 0xCD},
    {0x28, 0xFF, 0x2E, 0xAD, 0x63, 0x15, 0x03, 0x96}};
// {"cmd path", "auto push", "last update"}
String t_sensors[num_t_sensors][3] = {
    {"T-1", "0", "0"},
    {"T-2", "0", "0"},
    {"T-3", "0", "0"}};
// {"auto push"}
int eeprom_t_sensors_addr[num_t_sensors] = {1, 2, 3};

// Power supply sensors
#define num_ps_param 12
// {"cmd path", "auto push", "last update"}
String ps_param[num_ps_param][3] = {
    {"P-v-1", "0", "0"},
    {"P-v-2", "0", "0"},
    {"P-v-3", "0", "0"},
    {"P-v-4", "0", "0"},
    {"P-i-1", "0", "0"},
    {"P-i-2", "0", "0"},
    {"P-i-3", "0", "0"},
    {"P-i-4", "0", "0"},
    {"P-p-1", "0", "0"},
    {"P-p-2", "0", "0"},
    {"P-p-3", "0", "0"},
    {"P-p-4", "0", "0"}};
// {"auto push"}
int eeprom_ps_param_addr[num_ps_param] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

// Energy monitor
PZEM004T pzem(7, 8); // (RX,TX) connect to TX,RX of PZEM
IPAddress ip(192,168,1,1);
#define num_in_param 4
// {"cmd path", "auto push", "last update"}
String in_param[num_in_param][3] = {
    {"I-v", "0", "0"},
    {"I-i", "0", "0"},
    {"I-p", "0", "0"},
    {"I-e", "0", "0"}};
// {"auto push"}
int eeprom_in_param_addr[num_in_param] = {16, 17, 18, 19};

// Switches
#define num_switches 3
// {"cmd path", "switch pin", "status"}
String switches[num_switches][3] = {
    {"S-1", "13", "0"},
    {"S-2", "4", "0"},
    {"S-3", "5", "0"}};
// {"status"}
int eeprom_switches_addr[num_switches] = {20, 21, 22};

// Buttons
int tv_button = 12;
int ps_button = 6;

// Events
int events_status = 0;
int eeprom_events_status_addr = 23;


String get_voltage(String cmd_path) {
  int voltage_pin = 3; // DC-DC 5V
  float r1 = 101000; // R1 = 100k
  float r2 = 9780; // R2 = 10k
  int countvalues = 10; // how many values must be averaged
  float curVoltage = 0.0;
  String voltage_value;

  if (cmd_path == "P-v-1") {
    voltage_pin = 0; // power supply 12.1
    r1 = 104350; // R1 = 100k
    r2 = 9700; // R2 = 10k
  } else if (cmd_path == "P-v-2") {
    voltage_pin = 1; // power supply 12.2
    r1 = 102090; // R1 = 100k
    r2 = 9700; // R2 = 10k
  } else if (cmd_path == "P-v-3") {
    voltage_pin = 2; // power supply 12.3
    r1 = 101890; // R1 = 100k
    r2 = 9700; // R2 = 10k
  }

  for (int i = 0; i < countvalues; i++) {
    curVoltage += analogRead(voltage_pin);
    delay(3);
  }
  curVoltage = curVoltage / countvalues;
  float v  = (curVoltage * 5.0) / 1024.0;
  voltage_value = v / (r2 / (r1 + r2));
  if (voltage_value.toFloat() < 0.03) voltage_value = 0.0;
  return voltage_value;
}

String get_current(String cmd_path) {
  int curPin = 7; // DC-DC 5V
  float zeroLvl = 2.517;
  int direct = 0; // direction: 1 - direct, 0 - revert
  int countvalues = 50; // how many values must be averaged
  float koeficient = 0.185; // 5A: 0.185; 20A: 0.1; 30A: 0.066
  float sensorValue = 0.0; // variable to store the value coming from the sensor
  String current_value;

  if (cmd_path == "P-i-1") {
    curPin = 4; // power supply 12.1
    zeroLvl = 2.504;
    direct = 1;
    koeficient = 0.1;
  } else if (cmd_path == "P-i-2") {
    curPin = 5; // power supply 12.2
    direct = 0;
    zeroLvl = 2.5;
    koeficient = 0.1;
  } else if (cmd_path == "P-i-3") {
    curPin = 6; // power supply 12.3
    direct = 1;
    zeroLvl = 2.506;
    koeficient = 0.1;
  }

  for (int i = 0; i < countvalues; i++) {
    // read the value from the sensor:
    sensorValue += analogRead(curPin);
    delay(3);
  }
  // make average value
  sensorValue = sensorValue/countvalues;

  if (direct == 1) {
    current_value = (sensorValue * (5.0 / 1023.0) - zeroLvl) / koeficient;
  } else {
    current_value = (zeroLvl - sensorValue * (5.0 / 1023.0)) / koeficient;
  }
  if (current_value.toFloat() < 0.03) current_value = 0.0;
  return current_value;
}

String get_power_consuming(String cmd_path) {
  String volt_cmd_path = "P-v-4";
  String current_cmd_path = "P-i-4";
  if (cmd_path == "P-p-1") {
    volt_cmd_path = "P-v-1";
    current_cmd_path = "P-i-1";
  } else if (cmd_path == "P-p-2") {
    volt_cmd_path = "P-v-2";
    current_cmd_path = "P-i-2";
  } else if (cmd_path == "P-p-3") {
    volt_cmd_path = "P-v-3";
    current_cmd_path = "P-i-3";
  }
  float volt = get_voltage(volt_cmd_path).toFloat();
  float current = get_current(current_cmd_path).toFloat();
  float power_consuming_value = volt * current;
  return String(power_consuming_value);
}

String get_input_values(String arg) {
  String value;
  if (arg == "I-v") {
    value = pzem.voltage(ip);
    if (value.toFloat() < 0.0) value = 0.0;
  } else if (arg == "I-i") {
    value = pzem.current(ip);
    if (value.toFloat() < 0.0) value = 0.0;
  } else if (arg == "I-p") {
    value = pzem.power(ip);
    if (value.toFloat() < 0.0) value = 0.0;
  } else if (arg == "I-e") {
    value = pzem.energy(ip);
    if (value.toFloat() < 0.0) value = 0.0;
  }
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
  } else if ((char)payload[0] == 'P') {
    if ((char)payload[1] == '-') {
      if ((char)payload[2] == 'v' or (char)payload[2] == 'i' or (char)payload[2] == 'p') {
        int replied = 0;
        for (int i = 0; i < num_ps_param; i += 1) {
          String cmd_path = "P-" + String((char)payload[2]) + String((char)payload[3]) + String((char)payload[4]);
          if (cmd_path == ps_param[i][0]) {
            if ((char)payload[5] == '-') {
              if ((char)payload[6] == 'a') {
                if ((char)payload[7] == '-') {
                  if ((char)payload[8] == '0' or (char)payload[8] == '1') {
                    String command = String((char)payload[8]);
                    ps_param[i][1] = command;
                    EEPROM.update(eeprom_ps_param_addr[i], command.toInt());
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
                  String response_str = cmd_path + "-a>" + ps_param[i][1];
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
                value = get_voltage(cmd_path);
              } else if ((char)payload[2] == 'i') {
                value = get_current(cmd_path);
              } else if ((char)payload[2] == 'p') {
                value = get_power_consuming(cmd_path);
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
  } else if ((char)payload[0] == 'I') {
    if ((char)payload[1] == '-') {
      if ((char)payload[2] == 'v' or (char)payload[2] == 'i' or (char)payload[2] == 'p' or (char)payload[2] == 'e') {
        int replied = 0;
        for (int i = 0; i < num_in_param; i += 1) {
          String cmd_path = "I-" + String((char)payload[2]);
          if (cmd_path == in_param[i][0]) {
            if ((char)payload[3] == '-') {
              if ((char)payload[4] == 'a') {
                if ((char)payload[5] == '-') {
                  if ((char)payload[6] == '0' or (char)payload[6] == '1') {
                    String command = String((char)payload[6]);
                    in_param[i][1] = command;
                    EEPROM.update(eeprom_in_param_addr[i], command.toInt());
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
                  String response_str = cmd_path + "-a>" + in_param[i][1];
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
              String value = get_input_values(in_param[i][0]);
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
  } else if ((char)payload[0] == 'S') {
    if ((char)payload[1] == '-') {
      int replied = 0;
      for (int i = 0; i < num_switches; i += 1) {
        String cmd_path = "S-" + String((char)payload[2]);
        if (cmd_path == switches[i][0]) {
          if ((char)payload[3] == '-') {
            if (isDigit(payload[4])) {
              String switch_value = String((char)payload[4]);
              if (isDigit(payload[5])) {
                bus.reply(wrong_command_reply, 15);
                bus.update();
                replied = 1;
                break;
              }
              int switch_value_int = switch_value.toInt();
              if (switch_value_int == 0 or switch_value_int == 1) {
                if (switch_value_int == 0) {
                  digitalWrite(switches[i][1].toInt(), 0);
                  EEPROM.update(eeprom_switches_addr[i], switch_value_int);
                } else if (switch_value_int == 1) {
                  digitalWrite(switches[i][1].toInt(), 1);
                  EEPROM.update(eeprom_switches_addr[i], switch_value_int);
                }
                switches[i][2] = switch_value;
                String response_str = cmd_path + "-" + switch_value + ">" + switch_value;
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
              bus.reply(wrong_command_reply, 15);
              bus.update();
              replied = 1;
              break;
            }
          } else {
            String response_str = cmd_path + ">" + switches[i][2];
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
  } else if ((char)payload[0] == 'E') {
    if ((char)payload[1] == '-') {
      if ((char)payload[2] == '0' or (char)payload[2] == '1') {
        String command = String((char)payload[2]);
        events_status = command.toInt();
        EEPROM.update(eeprom_events_status_addr, command.toInt());
        String response_str = "E-" + command + ">" + command;
        bus_reply(response_str);
      } else {
        bus.reply(wrong_command_reply, 15);
        bus.update();
      }
    } else {
      String response_str = "E>" + String(events_status);
      bus_reply(response_str);
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
      for (int i = 0; i < num_ps_param; i += 1) {
        if (ps_param[i][1] == "1") {
          if (curMillis - ps_param[i][2].toInt() >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
            String value;
            if (ps_param[i][0] == "P-v-1" or ps_param[i][0] == "P-v-2" or ps_param[i][0] == "P-v-3" or ps_param[i][0] == "P-v-4") {
              value = get_voltage(ps_param[i][0]);
            } else if (ps_param[i][0] == "P-i-1" or ps_param[i][0] == "P-i-2" or ps_param[i][0] == "P-i-3" or ps_param[i][0] == "P-i-4") {
              value = get_current(ps_param[i][0]);
            } else if (ps_param[i][0] == "P-p-1" or ps_param[i][0] == "P-p-2" or ps_param[i][0] == "P-p-3" or ps_param[i][0] == "P-p-4") {
              value = get_power_consuming(ps_param[i][0]);
            }
            ps_param[i][2] = curMillis;
            lastUpdateMillis = curMillis;
            String response_str = ps_param[i][0] + "<" + value;
            bus_send(response_str);
            msg_pushed = 1;
          }
        }
      }
    }
    if (msg_pushed == 0) {
      for (int i = 0; i < num_in_param; i += 1) {
        if (in_param[i][1] == "1") {
          if (curMillis - in_param[i][2].toInt() >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
            String value = get_input_values(in_param[i][0]);
            in_param[i][2] = curMillis;
            lastUpdateMillis = curMillis;
            String response_str = in_param[i][0] + "<" + value;
            bus_send(response_str);
            msg_pushed = 1;
          }
        }
      }
    }
    prevMillis_autopush = curMillis;
  }
}

void buttons() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillis_buttons >= 3000) {
    if (digitalRead(tv_button) == HIGH) {
      if (switches[0][2] == "0") {
        digitalWrite(switches[0][1].toInt(), HIGH);
        EEPROM.update(eeprom_switches_addr[0], 1);
        switches[0][2] = "1";
      } else {
        digitalWrite(switches[0][1].toInt(), LOW);
        EEPROM.update(eeprom_switches_addr[0], 0);
        switches[0][2] = "0";
      }
      if (events_status == 1) {
        String response_str = switches[0][0] + "<" + switches[0][2];
        bus_send(response_str);
      }
      prevMillis_buttons = curMillis;
    }
    if (digitalRead(ps_button) == HIGH) {
      if (switches[2][2] == "0") {
        digitalWrite(switches[2][1].toInt(), HIGH);
        EEPROM.update(eeprom_switches_addr[2], 1);
        switches[2][2] = "1";
      } else {
        digitalWrite(switches[2][1].toInt(), LOW);
        EEPROM.update(eeprom_switches_addr[2], 0);
        switches[2][2] = "0";
      }
      if (events_status == 1) {
        String response_str = switches[2][0] + "<" + switches[2][2];
        bus_send(response_str);
      }
      prevMillis_buttons = curMillis;
    }
  }
}


void loop() {
  bus.receive(bus_receive_time);
  bus.update();
  autopush();
  bus.receive(bus_receive_time);
  bus.update();
  buttons();
}

void setup() {
  for (int i = 0; i < num_switches; i += 1) {
    pinMode(switches[i][1].toInt(), OUTPUT);
  }

  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) {
    //EEprom is wrong version or was not programmed, write default values to the EEprom
    for (int i = 0; i < num_t_sensors; i += 1) {
      EEPROM.update(eeprom_t_sensors_addr[i], t_sensors[i][1].toInt());
    }
    for (int i = 0; i < num_ps_param; i += 1) {
      EEPROM.update(eeprom_ps_param_addr[i], ps_param[i][1].toInt());
    }
    for (int i = 0; i < num_in_param; i += 1) {
      EEPROM.update(eeprom_in_param_addr[i], in_param[i][1].toInt());
    }
    for (int i = 0; i < num_switches; i += 1) {
      digitalWrite(switches[i][1].toInt(), switches[i][2].toInt());
      EEPROM.update(eeprom_switches_addr[i], switches[i][2].toInt());
    }
    EEPROM.update(eeprom_events_status_addr, events_status);
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    for (int i = 0; i < num_t_sensors; i += 1) {
      t_sensors[i][1] = EEPROM.read(eeprom_t_sensors_addr[i]);
    }
    for (int i = 0; i < num_ps_param; i += 1) {
      ps_param[i][1] = EEPROM.read(eeprom_ps_param_addr[i]);
    }
    for (int i = 0; i < num_in_param; i += 1) {
      in_param[i][1] = EEPROM.read(eeprom_in_param_addr[i]);
    }
    for (int i = 0; i < num_switches; i += 1) {
      switches[i][2] = EEPROM.read(eeprom_switches_addr[i]);
      digitalWrite(switches[i][1].toInt(), switches[i][2].toInt());
    }
    events_status = EEPROM.read(eeprom_events_status_addr);
  }

  // Set temperature sensor resolution, valid values are 9, 10, 11 or 12 bit.
  for (int i = 0; i < num_t_sensors; i += 1) {
    sensors.setResolution(t_sensors_addr[i], 11);
  }

  // PZEM004T module
  pzem.setAddress(ip);

  // Buttons
  pinMode(tv_button, INPUT);
  pinMode(ps_button, INPUT);

  bus.strategy.set_pin(3);
  bus.set_receiver(receiver_function);
  bus.set_synchronous_acknowledge(true);
  bus.set_crc_32(true);
  bus.begin();
}
