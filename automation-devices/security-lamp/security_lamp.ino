/*
* Copyright (C) 2018 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#define PJON_INCLUDE_SWBB true // Include SoftwareBitBang
#include <PJON.h>
#include <EEPROM.h>
#include <DHT.h>

PJON<SoftwareBitBang> bus(15);
int master_id = 1;
int bus_receive_time = 7000;

#define DHTTYPE DHT22
#define DHTPIN10 10 // Temperature sensor
DHT dht(DHTPIN10, DHTTYPE);

#define ADDR_VERSION 255    //location of the software version in EEPROM
#define CURRENT_EEPROM_VERSION 1 //we are on revision 1 of the EEPROM storage structure (0xFF or 255 is never a valid value for version)

char wrong_command_reply[16] = ">failed command";
unsigned long min_push_interval = 3000; // interval between push data to master
unsigned long push_interval = 60000; // push interval (each sensor read once within this interval)
unsigned long prevMillis_autopush = millis();
unsigned long prevMillis_flashes = millis();
unsigned long prevMillis_camera = millis();
unsigned long lastUpdateMillis = millis();

#define num_leds 4
// {"leds path", "leds pin", "value", "max brightness"}
String leds[num_leds][4] = {
    {"L-w", "3", "0", "100"},
    {"L-r", "5", "0", "100"},
    {"L-g", "6", "0", "100"},
    {"L-b", "9", "0", "100"}};
int leds_fadeValue_start = 0;
// "max brightness"
int eeprom_leds_addr[num_leds] = {0, 1, 2, 3};
// leds automode
int leds_automode = 0;
int eeprom_leds_automode_addr = 4;
int max_brightness_white = 255;
int max_brightness_green = 255;
unsigned long max_flash_time = 600000;

// Ultrasonic Sensor HC-SR04
int trigPin = 7;
int echoPin = 8;
int ultrasonic_automode = 0;
int eeprom_ultrasonic_automode_addr = 5;
int ultrasonic_limit = 2;
int eeprom_ultrasonic_limit_addr = 6;
String ultrasonic_last_value;
int ultrasonic_start_reached_limit = 1;
int ultrasonic_reached_limit = 0;
int ultrasonic_reached_limit_end = 0;
unsigned long prevMillis_ultrasonic = millis();
unsigned long ultrasonic_last_update = millis();
unsigned long ultrasonic_reached_limit_continue = millis();

// Motion sensor
int motion_pin = 11;
int motion_status = 0;
int motion_autopush = 0;
int motion_time = 0;
unsigned long motion_last_update;
// {"auto push", "time"}
int eeprom_motion_sensor_addr[2] = {7, 8};

// Light sensors
int light_sensor_pin = 0;
int light_sensor_autopush = 0;
int light_sensor_brightness = 0;
unsigned long light_sensor_last_update;
// {"auto push", "brightness"}
int eeprom_light_sensor_addr[2] = {9, 10};

// Camera
int camera_pin = 13;
int camera_status = 0;
int camera_state = 0;
int eeprom_camera_status_addr = 11;

// Door sensor
int door_pin = 2;
int door_status = 0;
int door_autopush = 0;
int eeprom_door_autopush_addr = 12;

// Alarm
int alarm_status = 0;
int eeprom_alarm_status_addr = 13;

// Voltmeter
int voltage_pin = 1;
float voltage_r1 = 102100; // R1 = 100k
float voltage_r2 = 9800; // R2 = 10k
unsigned long voltage_last_update = millis();
int voltage_automode = 0;
int eeprom_voltage_automode_addr = 14; // for "automode"
int voltage_low = 10.7; // Voltage low limit
int voltage_high = 12.0; // Voltage high limit

// Events
int event_status = 0;
int eeprom_event_status_addr = 15;

// temperature sensor - DHT22
int t_sensor_t_autopush = 0;
int t_sensor_f_autopush = 0;
int t_sensor_h_autopush = 0;
int eeprom_t_sensor_t_autopush_addr = 16;
int eeprom_t_sensor_f_autopush_addr = 17;
int eeprom_t_sensor_h_autopush_addr = 18;
unsigned long t_sensor_t_last_update = millis();
unsigned long t_sensor_f_last_update = millis();
unsigned long t_sensor_h_last_update = millis();


String ultrasonic_sensor() {
  long duration;
  int distance;
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration*0.034/2;
  return String(distance);
}

String get_voltage() {
  int countvalues = 10; // how many values must be averaged
  float curVoltage = 0.0;
  String voltage_value;

  for (int i = 0; i < countvalues; i++) {
    curVoltage += analogRead(voltage_pin);
    delay(3);
  }
  curVoltage = curVoltage / countvalues;
  float v  = (curVoltage * 5.0) / 1024.0;
  voltage_value = v / (voltage_r2 / (voltage_r1 + voltage_r2));
  if (voltage_value.toFloat() < 0.03) voltage_value = 0.0;
  return voltage_value;
}

void bus_reply(String response_str) {
  int response_str_len = response_str.length();
  char response[response_str_len + 1];
  response_str.toCharArray(response, response_str_len + 1);
  bus.reply(response, response_str_len);
  bus.update();
}

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  if ((char)payload[0] == 'L') {
    if ((char)payload[1] == '-') {
      if ((char)payload[2] == 'w' or (char)payload[2] == 'r' or (char)payload[2] == 'g' or (char)payload[2] == 'b') {
        int replied = 0;
        for (int i = 0; i < num_leds; i += 1) {
          String led = "L-" + String((char)payload[2]);
          if (led == leds[i][0]) {
            if ((char)payload[3] == '-') {
              if (isDigit(payload[4])) {
                String led_value = String((char)payload[4]);
                if (isDigit(payload[5])) {
                  led_value += String((char)payload[5]);
                  if (isDigit(payload[6])) {
                    led_value += String((char)payload[6]);
                    if (isDigit(payload[7])) {
                      bus.reply(wrong_command_reply, 15);
                      bus.update();
                      replied = 1;
                      break;
                    }
                  }
                }
                int led_value_int = led_value.toInt();
                if (led_value_int >= 0 and led_value_int <= 100) {
                  if (led_value_int == 0) {
                    digitalWrite(leds[i][1].toInt(), 0);
                  } else if (led_value_int == 100) {
                    digitalWrite(leds[i][1].toInt(), 1);
                  } else {
                    int light_percent = map(led_value_int, 0, 100, 0, 255);
                    analogWrite(leds[i][1].toInt(), light_percent);
                  }
                  leds[i][2] = led_value;
                  String response_str = led + "-" + led_value + ">" + led_value;
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
              String response_str = led + ">" + leds[i][2];
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
      } else if ((char)payload[2] == 'a') {
        if ((char)payload[3] == '-') {
          String command = String((char)payload[4]);
          if (command == "0" or command == "1" or command == "2" or command == "3" or command == "4") {
            if (command == "0") {
              for (int i = 0; i < num_leds; i += 1) {
                digitalWrite(leds[i][1].toInt(), 0);
              }
            }
            leds_automode = command.toInt();
            EEPROM.update(eeprom_leds_automode_addr, command.toInt());
            String response_str = "L-a-" + command + ">" + command;
            bus_reply(response_str);
          } else {
            bus.reply(wrong_command_reply, 15);
            bus.update();
          }
        } else {
          String response_str = "L-a>" + String(leds_automode);
          bus_reply(response_str);
        }
      } else if ((char)payload[2] == 'l') {
        if ((char)payload[3] == '-') {
          if ((char)payload[4] == 'w' or (char)payload[4] == 'r' or (char)payload[4] == 'g' or (char)payload[4] == 'b') {
            int replied = 0;
            for (int i = 0; i < num_leds; i += 1) {
              String led = "L-" + String((char)payload[4]);
              if (led == leds[i][0]) {
                if ((char)payload[5] == '-') {
                  if (isDigit(payload[6])) {
                    String led_value = String((char)payload[6]);
                    if (isDigit(payload[7])) {
                      led_value += String((char)payload[7]);
                      if (isDigit(payload[8])) {
                        led_value += String((char)payload[8]);
                      }
                    }
                    int led_value_int = led_value.toInt();
                    if (led_value_int >= 0 and led_value_int <= 100) {
                      leds[i][3] = led_value;
                      EEPROM.update(eeprom_leds_addr[i], led_value_int);
                      String response_str = "L-l-" + String((char)payload[4]) + "-" + led_value + ">" + led_value;
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
                  String response_str = "L-l-" + String((char)payload[4]) + ">" + leds[i][3];
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
    } else {
      bus.reply(wrong_command_reply, 15);
      bus.update();
    }
  } else if ((char)payload[0] == 'T') {
    if ((char)payload[1] == '-') {
      if ((char)payload[2] == 't' or (char)payload[2] == 'f' or (char)payload[2] == 'h') {
        if ((char)payload[3] == '-') {
          if ((char)payload[4] == 'a') {
            if ((char)payload[5] == '-') {
              String command = String((char)payload[6]);
              if (command == "0" or command == "1") {
                if ((char)payload[2] == 't') {
                  t_sensor_t_autopush = command.toInt();
                  EEPROM.update(eeprom_t_sensor_t_autopush_addr, command.toInt());
                } else if ((char)payload[2] == 'f') {
                  t_sensor_f_autopush = command.toInt();
                  EEPROM.update(eeprom_t_sensor_f_autopush_addr, command.toInt());
                } else if ((char)payload[2] == 'h') {
                  t_sensor_h_autopush = command.toInt();
                  EEPROM.update(eeprom_t_sensor_h_autopush_addr, command.toInt());
                }
                String response_str = "T-" + String((char)payload[2]) + "-a-" + command + ">" + command;
                bus_reply(response_str);
              } else {
                bus.reply(wrong_command_reply, 15);
                bus.update();
              }
            } else {
              String t_sensor_autopush;
              if ((char)payload[2] == 't') {
                t_sensor_autopush = t_sensor_t_autopush;
              } else if ((char)payload[2] == 'f') {
                t_sensor_autopush = t_sensor_f_autopush;
              } else if ((char)payload[2] == 'h') {
                t_sensor_autopush = t_sensor_h_autopush;
              }
              String response_str = "T-" + String((char)payload[2]) + "-a>" + t_sensor_autopush;
              bus_reply(response_str);
            }
          } else {
            bus.reply(wrong_command_reply, 15);
            bus.update();
          }
        } else {
          String t_sensor_value;
          if ((char)payload[2] == 't') {
            t_sensor_value = dht.readTemperature();
          } else if ((char)payload[2] == 'f') {
            t_sensor_value = dht.computeHeatIndex(dht.readTemperature(), dht.readHumidity(), false);
          } else if ((char)payload[2] == 'h') {
            t_sensor_value = dht.readHumidity();
          }
          String response_str = "T-" + String((char)payload[2]) + ">" + t_sensor_value;
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
  } else if ((char)payload[0] == 'S') {
    if ((char)payload[1] == '-') {
      if ((char)payload[2] == 'u') {
        if ((char)payload[3] == '-') {
          if ((char)payload[4] == 'a') {
            if ((char)payload[5] == '-') {
              String command = String((char)payload[6]);
              if (command == "0" or command == "1") {
                ultrasonic_automode = command.toInt();
                EEPROM.update(eeprom_ultrasonic_automode_addr, command.toInt());
                String response_str = "S-u-a-" + command + ">" + command;
                bus_reply(response_str);
              } else {
                bus.reply(wrong_command_reply, 15);
                bus.update();
              }
            } else {
              String response_str = "S-u-a>" + String(ultrasonic_automode);
              bus_reply(response_str);
            }
          } else if ((char)payload[4] == 'l') {
            if ((char)payload[5] == '-') {
              if (isDigit(payload[6])) {
                String limit_value = String((char)payload[6]);
                if (isDigit(payload[7])) {
                  limit_value += String((char)payload[7]);
                  if (isDigit(payload[8])) {
                    limit_value += String((char)payload[8]);
                    if (isDigit(payload[9])) {
                      bus.reply(wrong_command_reply, 15);
                    }
                  }
                }
                int limit_value_int = limit_value.toInt();
                if (limit_value_int >= 2 and limit_value_int <= 255) {
                  ultrasonic_limit = limit_value_int;
                  EEPROM.update(eeprom_ultrasonic_limit_addr, ultrasonic_limit);
                  String response_str = "S-u-l-" + limit_value + ">" + limit_value;
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
              String response_str = "S-u-l>" + String(ultrasonic_limit);
              bus_reply(response_str);
            }
          } else {
            bus.reply(wrong_command_reply, 15);
            bus.update();
          }
        } else {
          String response_str = "S-u>" + ultrasonic_sensor();
          bus_reply(response_str);
        }
      } else if ((char)payload[2] == 'm') {
        if ((char)payload[3] == '-') {
          String command_type = String((char)payload[4]);
          if (command_type == "a") {
            if ((char)payload[5] == '-') {
              String command = String((char)payload[6]);
              int command_int = command.toInt();
              if (command == "0" or command == "1") {
                motion_autopush = command_int;
                EEPROM.update(eeprom_motion_sensor_addr[0], command_int);
                String response_str = "S-m-a-" + command + ">" + command;
                bus_reply(response_str);
              } else {
                bus.reply(wrong_command_reply, 15);
                bus.update();
              }
            } else {
              String response_str = "S-m-a>" + String(motion_autopush);
              bus_reply(response_str);
            }
          } else if (command_type == "t") {
            if ((char)payload[5] == '-') {
              if (isDigit(payload[6])) {
                String c_value = String((char)payload[6]);
                if (isDigit(payload[7])) {
                  c_value += String((char)payload[7]);
                  if (isDigit(payload[8])) {
                    c_value += String((char)payload[8]);
                    bus.reply(wrong_command_reply, 15);
                    bus.update();
                  }
                }
                int c_value_int = c_value.toInt();
                if (c_value_int >= 0 and c_value_int <= 99) {
                  motion_time = c_value_int;
                  EEPROM.update(eeprom_motion_sensor_addr[1], c_value_int);
                  String response_str = "S-m-t-" + c_value + ">" + c_value;
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
              String response_str = "S-m-t>" + String(motion_time);
              bus_reply(response_str);
            }
          } else {
            bus.reply(wrong_command_reply, 15);
            bus.update();
          }
        } else {
          String response_str = "S-m>" + String(digitalRead(motion_pin));
          bus_reply(response_str);
        }
      } else if ((char)payload[2] == 'l') {
        if ((char)payload[3] == '-') {
          String command_type = String((char)payload[4]);
          if (command_type == "a") {
            if ((char)payload[5] == '-') {
              String command = String((char)payload[6]);
              int command_int = command.toInt();
              if (command == "0" or command == "1") {
                light_sensor_autopush = command_int;
                EEPROM.update(eeprom_light_sensor_addr[0], command_int);
                String response_str = "S-l-a-" + command + ">" + command;
                bus_reply(response_str);
              } else {
                bus.reply(wrong_command_reply, 15);
                bus.update();
              }
            } else {
              String response_str = "S-l-a>" + String(light_sensor_autopush);
              bus_reply(response_str);
            }
          } else if (command_type == "b") {
            if ((char)payload[5] == '-') {
              if (isDigit(payload[6])) {
                String c_value = String((char)payload[6]);
                if (isDigit(payload[7])) {
                  c_value += String((char)payload[7]);
                  if (isDigit(payload[8])) {
                    c_value += String((char)payload[8]);
                    if (isDigit(payload[9])) {
                      c_value += String((char)payload[9]);
                      bus.reply(wrong_command_reply, 15);
                      bus.update();
                    }
                  }
                }
                int c_value_int = c_value.toInt();
                if (c_value_int >= 0 and c_value_int <= 100) {
                  light_sensor_brightness = c_value_int;
                  EEPROM.update(eeprom_light_sensor_addr[1], c_value_int);
                  String response_str = "S-l-b-" + c_value + ">" + c_value;
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
              String response_str = "S-l-b>" + String(light_sensor_brightness);
              bus_reply(response_str);
            }
          } else {
            bus.reply(wrong_command_reply, 15);
            bus.update();
          }
        } else {
          int brightness_percent = map(analogRead(light_sensor_pin), 0, 1023, 0, 100);
          delay(10);
          String response_str = "S-l>" + String(brightness_percent);
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
  } else if ((char)payload[0] == 'C') {
    if ((char)payload[1] == '-') {
      String camera_value = String((char)payload[2]);
      if (camera_value == "0" or camera_value == "1" or camera_value == "2" or camera_value == "3") {
        if (camera_value == "0" or camera_value == "3") {
          digitalWrite(camera_pin, 0);
          camera_state = 0;
        } else if (camera_value == "1") {
          digitalWrite(camera_pin, 1);
          camera_state = 1;
        } else if (camera_value == "2") {
          if (alarm_status == 1 or alarm_status == 2) {
            digitalWrite(camera_pin, 1);
            camera_state = 1;
          } else if (alarm_status == 0) {
            digitalWrite(camera_pin, 0);
            camera_state = 0;
          }
        }
        camera_status = camera_value.toInt();
        EEPROM.update(eeprom_camera_status_addr, camera_value.toInt());
        String response_str = "C-" + camera_value + ">" + camera_value;
        bus_reply(response_str);
      } else {
        bus.reply(wrong_command_reply, 15);
        bus.update();
      }
    } else {
      String response_str = "C>" + String(camera_status);
      bus_reply(response_str);
    }
  } else if ((char)payload[0] == 'V') {
    if ((char)payload[1] == '-') {
      if ((char)payload[2] == 'a') {
        if ((char)payload[3] == '-') {
          String command = String((char)payload[4]);
          if (command == "0" or command == "1") {
            voltage_automode = command.toInt();
            EEPROM.update(eeprom_voltage_automode_addr, command.toInt());
            String response_str = "V-a-" + command + ">" + command;
            bus_reply(response_str);
          } else {
            bus.reply(wrong_command_reply, 15);
            bus.update();
          }
        } else {
          String response_str = "V-a>" + String(voltage_automode);
          bus_reply(response_str);
        }
      } else {
        bus.reply(wrong_command_reply, 15);
        bus.update();
      }
    } else {
      String value = get_voltage();
      String response_str = "V>" + value;
      bus_reply(response_str);
    }
  } else if ((char)payload[0] == 'D') {
    if ((char)payload[1] == '-') {
      if ((char)payload[2] == 'a') {
        if ((char)payload[3] == '-') {
          String command = String((char)payload[4]);
          if (command == "0" or command == "1") {
            door_autopush = command.toInt();
            EEPROM.update(eeprom_door_autopush_addr, command.toInt());
            String response_str = "D-a-" + command + ">" + command;
            bus_reply(response_str);
          } else {
            bus.reply(wrong_command_reply, 15);
            bus.update();
          }
        } else {
          String response_str = "D-a>" + String(door_autopush);
          bus_reply(response_str);
        }
      } else {
        bus.reply(wrong_command_reply, 15);
        bus.update();
      }
    } else {
      String response_str = "D>" + String(digitalRead(door_pin));
      bus_reply(response_str);
    }
  } else if ((char)payload[0] == 'A') {
    if ((char)payload[1] == '-') {
      String alarm_value = String((char)payload[2]);
      if (alarm_value == "0" or alarm_value == "1" or alarm_value == "2") {
        alarm_status = alarm_value.toInt();
        EEPROM.update(eeprom_alarm_status_addr, alarm_value.toInt());
        prevMillis_flashes = millis();
        String response_str = "A-" + alarm_value + ">" + alarm_value;
        bus_reply(response_str);
      } else {
        bus.reply(wrong_command_reply, 15);
        bus.update();
      }
    } else {
      String response_str = "A>" + String(alarm_status);
      bus_reply(response_str);
    }
  } else if ((char)payload[0] == 'E') {
    if ((char)payload[1] == '-') {
      String event_value = String((char)payload[2]);
      if (event_value == "0" or event_value == "1") {
        event_status = event_value.toInt();
        EEPROM.update(eeprom_event_status_addr, event_value.toInt());
        String response_str = "E-" + event_value + ">" + event_value;
        bus_reply(response_str);
      } else {
        bus.reply(wrong_command_reply, 15);
        bus.update();
      }
    } else {
      String response_str = "E>" + String(event_status);
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

void motion_detect() {
  unsigned long curMillis = millis(); // time now in ms
  if (motion_time != 0) {
    int sensor_status = digitalRead(motion_pin);
    if (sensor_status == 1 and motion_status == 1)
      motion_last_update = curMillis;
    unsigned long time_limit = motion_time * 1000;
    if (time_limit <= curMillis - motion_last_update) {
      if (sensor_status != motion_status) {
        motion_status = sensor_status;
        if (motion_status == 1)
          motion_last_update = curMillis;
        if (motion_autopush == 1) {
          String response_str = "S-m<" + String(motion_status);
          bus_send(response_str);
        }
      }
    }
  }
}

void open_door_detect() {
  int sensor_status = digitalRead(door_pin);
  if (sensor_status != door_status) {
    door_status = sensor_status;
    if (door_autopush == 1) {
      String response_str = "D<" + String(door_status);
      bus_send(response_str);
    }
  }
}

void automode_camera() {
  if (camera_status == 2) {
    if ((alarm_status == 1 or alarm_status == 2) and camera_state == 0) {
      digitalWrite(camera_pin, 1);
      camera_state = 1;
    } else if (alarm_status == 0 and camera_state == 1) {
      digitalWrite(camera_pin, 0);
      camera_state = 0;
    }
  }
}

void autopush() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillis_autopush >= min_push_interval) {
    int msg_pushed = 0;
    if (light_sensor_autopush == 1) {
      if (curMillis - light_sensor_last_update >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
        int brightness_percent = map(analogRead(light_sensor_pin), 0, 1023, 0, 100);
        light_sensor_last_update = curMillis;
        lastUpdateMillis = curMillis;
        String response_str = "S-l<" + String(brightness_percent);
        bus_send(response_str);
        msg_pushed = 1;
      }
    }
    if (msg_pushed == 0) {
      if (voltage_automode == 1) {
        if (curMillis - voltage_last_update >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
          voltage_last_update = curMillis;
          lastUpdateMillis = curMillis;
          String response_str = "V<" + get_voltage();
          bus_send(response_str);
          msg_pushed = 1;
        }
      }
    }
    if (msg_pushed == 0) {
      if (ultrasonic_automode == 1) {
        if (curMillis - ultrasonic_last_update >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
          String ultrasonic_value = ultrasonic_sensor();
          if (ultrasonic_value != ultrasonic_last_value) {
            ultrasonic_last_value = ultrasonic_value;
            ultrasonic_last_update = curMillis;
            lastUpdateMillis = curMillis;
            String response_str = "S-u<" + ultrasonic_last_value;
            bus_send(response_str);
            msg_pushed = 1;
          }
        }
      }
    }
    if (msg_pushed == 0) {
      if (t_sensor_t_autopush == 1) {
        if (curMillis - t_sensor_t_last_update >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
          t_sensor_t_last_update = curMillis;
          lastUpdateMillis = curMillis;
          String response_str = "T-t<" + String(dht.readTemperature());
          bus_send(response_str);
          msg_pushed = 1;
        }
      }
    }
    if (msg_pushed == 0) {
      if (t_sensor_f_autopush == 1) {
        if (curMillis - t_sensor_f_last_update >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
          t_sensor_f_last_update = curMillis;
          lastUpdateMillis = curMillis;
          String response_str = "T-f<" + String(dht.computeHeatIndex(dht.readTemperature(), dht.readHumidity(), false));
          bus_send(response_str);
          msg_pushed = 1;
        }
      }
    }
    if (msg_pushed == 0) {
      if (t_sensor_h_autopush == 1) {
        if (curMillis - t_sensor_h_last_update >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
          t_sensor_h_last_update = curMillis;
          lastUpdateMillis = curMillis;
          String response_str = "T-h<" + String(dht.readHumidity());
          bus_send(response_str);
          msg_pushed = 1;
        }
      }
    }
    prevMillis_autopush = curMillis;
  }
}

int max_b_p(String max_brightness) {
  int max_brightness_percent;
  if (leds_automode == 2 or leds_automode == 4) {
    float voltage = get_voltage().toFloat();
    if (voltage <= voltage_low) {
      max_brightness_percent = 0;
    } else if (voltage >= voltage_high) {
      max_brightness_percent = 100;
    } else {
      int voltage_int = (int) (voltage * 10);
      int voltage_low_int = (int) (voltage_low * 10);
      int voltage_high_int = (int) (voltage_high * 10);
      max_brightness_percent = map(voltage_int, voltage_low_int, voltage_high_int, 0, 100);
    }
  } else {
    max_brightness_percent = max_brightness.toInt();
  }
  return max_brightness_percent;
}

void leds_automode_control() {
  if (leds_automode == 1 or leds_automode == 2 or leds_automode == 3 or leds_automode == 4) {
    if (alarm_status == 0) {
      if ((motion_status == 1 and leds[0][2] == "0" and ultrasonic_reached_limit == 0) or (leds_fadeValue_start != 0)) {
        int brightness_percent = map(analogRead(light_sensor_pin), 0, 1023, 0, 100);
        if (brightness_percent <= light_sensor_brightness or leds_fadeValue_start != 0) {
          int max_brightness_percent = max_b_p(leds[0][3]);
          max_brightness_white = map(max_brightness_percent, 0, 100, 0, 255);
          for (int fadeValue = leds_fadeValue_start; fadeValue <= max_brightness_white; fadeValue += 5) {
            analogWrite(leds[0][1].toInt(), fadeValue);
            bus.receive(bus_receive_time);
            bus.update();
            delay(50);
          }
          if (max_brightness_percent == 100)
            digitalWrite(leds[0][1].toInt(), 1);
          leds[0][2] = String(max_brightness_percent);
          leds_fadeValue_start = 0;
          if (event_status == 1) {
            String response_str = leds[0][0] + "<" + max_brightness_percent;
            bus_send(response_str);
          }
        }
      } else if (motion_status == 0 and leds[0][2].toInt() > 0) {
        for (int fadeValue = max_brightness_white; fadeValue >= 0; fadeValue -= 5) {
          analogWrite(leds[0][1].toInt(), fadeValue);
          bus.receive(bus_receive_time);
          bus.update();
          if (digitalRead(motion_pin) == 1) {
            motion_status = 1;
            leds_fadeValue_start = fadeValue;
            break;
          }
          delay(50);
        }
        if (leds_fadeValue_start == 0) {
          digitalWrite(leds[0][1].toInt(), 0);
          leds[0][2] = "0";
          if (event_status == 1) {
            String response_str = leds[0][0] + "<0";
            bus_send(response_str);
          }
        }
      }
      if (door_status == 1 and leds[2][2] == "0") {
        digitalWrite(leds[0][1].toInt(), 0);
        leds[0][2] = "0";
        leds_fadeValue_start = 0;
        motion_status = 0;
        if (event_status == 1) {
          String response_str = leds[0][0] + "<0";
          bus_send(response_str);
        }
        int max_brightness_percent = max_b_p(leds[2][3]);
        max_brightness_green = map(max_brightness_percent, 0, 100, 0, 255);
        for (int fadeValue = 0; fadeValue <= max_brightness_green; fadeValue += 5) {
          analogWrite(leds[2][1].toInt(), fadeValue);
          bus.receive(bus_receive_time);
          bus.update();
          delay(30);
        }
        leds[2][2] = String(max_brightness_percent);
      } else if (door_status == 0 and leds[2][2] != "0") {
        for (int fadeValue = max_brightness_green; fadeValue >= 0; fadeValue -= 5) {
          analogWrite(leds[2][1].toInt(), fadeValue);
          bus.receive(bus_receive_time);
          bus.update();
          delay(30);
        }
        digitalWrite(leds[2][1].toInt(), 0);
        leds[2][2] = "0";
      }
    } else if (alarm_status == 1 or alarm_status == 2) {
      unsigned long curMillis = millis(); // time now in ms
      if (curMillis - prevMillis_flashes <= max_flash_time) {
        int max_brightness_percent = max_b_p(leds[1][3]);
        int max_brightness_red = map(max_brightness_percent, 0, 100, 0, 255);
        max_brightness_percent = max_b_p(leds[3][3]);
        int max_brightness_blue = map(max_brightness_percent, 0, 100, 0, 255);
        if (leds[0][2] != "0") {
          digitalWrite(leds[0][1].toInt(), 0);
          leds[0][2] = "0";
          leds_fadeValue_start = 0;
          motion_status = 0;
          if (event_status == 1) {
            String response_str = leds[0][0] + "<0";
            bus_send(response_str);
          }
        }
        for (int i = 0; i < 3; i++) {
          analogWrite(leds[1][1].toInt(), max_brightness_red);
          delay(50);
          bus.receive(bus_receive_time);
          bus.update();
          digitalWrite(leds[1][1].toInt(), 0);
          delay(50);
          bus.receive(bus_receive_time);
          bus.update();
        }
        delay(200);
        bus.receive(bus_receive_time);
        bus.update();
        for (int i = 0; i < 3; i++) {
          analogWrite(leds[3][1].toInt(), max_brightness_blue);
          delay(50);
          bus.receive(bus_receive_time);
          bus.update();
          digitalWrite(leds[3][1].toInt(), 0);
          delay(50);
          bus.receive(bus_receive_time);
          bus.update();
        }
      }
    }
    if (leds_automode == 3 or leds_automode == 4) {
      int ultrasonic_value = ultrasonic_sensor().toInt();
      if (ultrasonic_value <= ultrasonic_limit and ultrasonic_value != 0) {
        unsigned long curMillis = millis(); // time now in ms
        if (ultrasonic_start_reached_limit == 1) {
          prevMillis_ultrasonic = curMillis;
          ultrasonic_start_reached_limit = 0;
        }
        if (curMillis - prevMillis_ultrasonic >= 15000) {
          if (camera_status == 3 and camera_state == 0) {
            digitalWrite(camera_pin, 1);
            camera_state = 1;
          }
          prevMillis_camera = curMillis;
          if (leds[0][2] != "0") {
            digitalWrite(leds[0][1].toInt(), 0);
            leds[0][2] = "0";
            leds_fadeValue_start = 0;
            ultrasonic_reached_limit = 1;
            if (event_status == 1) {
              String response_str = leds[0][0] + "<0";
              bus_send(response_str);
            }
          }
          if (ultrasonic_reached_limit_end == 0) {
            int max_brightness_percent = max_b_p(leds[3][3]);
            int max_brightness_blue = map(max_brightness_percent, 0, 100, 0, 255);
            for (int i = 0; i < 5; i++) {
              for (int fadeValue = 0; fadeValue <= max_brightness_blue; fadeValue += 5) {
                analogWrite(leds[3][1].toInt(), fadeValue);
                bus.receive(bus_receive_time);
                bus.update();
                delay(10);
              }
              delay(250);
              bus.receive(bus_receive_time);
              bus.update();
              digitalWrite(leds[3][1].toInt(), 0);
              delay(250);
              bus.receive(bus_receive_time);
              bus.update();
            }
            ultrasonic_reached_limit_end = 1;
          }
          int max_brightness_percent = max_b_p(leds[1][3]);
          int max_brightness_red = map(max_brightness_percent, 0, 100, 0, 255);
          analogWrite(leds[1][1].toInt(), max_brightness_red);
          delay(250);
          bus.receive(bus_receive_time);
          bus.update();
          digitalWrite(leds[1][1].toInt(), 0);
          delay(250);
          bus.receive(bus_receive_time);
          bus.update();
          if (event_status == 1) {
            unsigned long curMillis = millis(); // time now in ms
            if (curMillis - ultrasonic_reached_limit_continue >= push_interval) {
              ultrasonic_reached_limit_continue = curMillis;
              String response_str = "S-u<" + ultrasonic_sensor();
              bus_send(response_str);
            }
          }
        }
      } else {
        if (camera_status == 3 and motion_status == 0 and camera_state == 1) {
          unsigned long curMillis = millis(); // time now in ms
          if (curMillis - prevMillis_camera >= 60000) {
            digitalWrite(camera_pin, 0);
            camera_state = 0;
          }
        }
        if (ultrasonic_start_reached_limit == 0) {
          ultrasonic_start_reached_limit = 1;
        }
        if (ultrasonic_reached_limit == 1) {
          ultrasonic_reached_limit = 0;
        }
        if (ultrasonic_reached_limit_end == 1) {
          ultrasonic_reached_limit_end = 0;
          if (event_status == 1) {
            String response_str = "S-u<" + ultrasonic_sensor();
            bus_send(response_str);
          }
        }
      }
    }
  }
}


void loop() {
  bus.receive(bus_receive_time);
  bus.update();
  motion_detect();
  bus.receive(bus_receive_time);
  bus.update();
  open_door_detect();
  bus.receive(bus_receive_time);
  bus.update();
  automode_camera();
  bus.receive(bus_receive_time);
  bus.update();
  autopush();
  bus.receive(bus_receive_time);
  bus.update();
  leds_automode_control();
}

void setup() {
  for (int i = 0; i < num_leds; i += 1) {
    pinMode(leds[i][1].toInt(), OUTPUT);
    digitalWrite(leds[i][1].toInt(), 0);
  }

  // Ultrasonic Sensor HC-SR04
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  pinMode(camera_pin, OUTPUT);
  pinMode(door_pin, INPUT);

  dht.begin();

  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) {
    //EEprom is wrong version or was not programmed, write default values to the EEprom
    for (int i = 0; i < num_leds; i += 1) {
      EEPROM.update(eeprom_leds_addr[i], leds[i][3].toInt());
    }
    EEPROM.update(eeprom_leds_automode_addr, leds_automode);
    EEPROM.update(eeprom_ultrasonic_automode_addr, ultrasonic_automode);
    EEPROM.update(eeprom_ultrasonic_limit_addr, ultrasonic_limit);
    EEPROM.update(eeprom_motion_sensor_addr[0], motion_autopush);
    EEPROM.update(eeprom_motion_sensor_addr[1], motion_time);
    EEPROM.update(eeprom_light_sensor_addr[0], light_sensor_autopush);
    EEPROM.update(eeprom_light_sensor_addr[1], light_sensor_brightness);
    EEPROM.update(eeprom_camera_status_addr, camera_status);
    EEPROM.update(eeprom_door_autopush_addr, door_autopush);
    EEPROM.update(eeprom_alarm_status_addr, alarm_status);
    EEPROM.update(eeprom_voltage_automode_addr, voltage_automode);
    EEPROM.update(eeprom_event_status_addr, event_status);
    EEPROM.update(eeprom_t_sensor_t_autopush_addr, t_sensor_t_autopush);
    EEPROM.update(eeprom_t_sensor_f_autopush_addr, t_sensor_f_autopush);
    EEPROM.update(eeprom_t_sensor_h_autopush_addr, t_sensor_h_autopush);
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    for (int i = 0; i < num_leds; i += 1) {
      leds[i][3] = EEPROM.read(eeprom_leds_addr[i]);
    }
    leds_automode = EEPROM.read(eeprom_leds_automode_addr);
    ultrasonic_automode = EEPROM.read(eeprom_ultrasonic_automode_addr);
    ultrasonic_limit = EEPROM.read(eeprom_ultrasonic_limit_addr);
    motion_autopush = EEPROM.read(eeprom_motion_sensor_addr[0]);
    motion_time = EEPROM.read(eeprom_motion_sensor_addr[1]);
    light_sensor_autopush = EEPROM.read(eeprom_light_sensor_addr[0]);
    light_sensor_brightness = EEPROM.read(eeprom_light_sensor_addr[1]);
    camera_status = EEPROM.read(eeprom_camera_status_addr);
    door_autopush = EEPROM.read(eeprom_door_autopush_addr);
    alarm_status = EEPROM.read(eeprom_alarm_status_addr);
    voltage_automode = EEPROM.read(eeprom_voltage_automode_addr);
    event_status = EEPROM.read(eeprom_event_status_addr);
    t_sensor_t_autopush = EEPROM.read(eeprom_t_sensor_t_autopush_addr);
    t_sensor_f_autopush = EEPROM.read(eeprom_t_sensor_f_autopush_addr);
    t_sensor_h_autopush = EEPROM.read(eeprom_t_sensor_h_autopush_addr);
  }

  if (camera_status == 0 or camera_status == 1) {
    digitalWrite(camera_pin, camera_status);
    camera_state = camera_status;
  } else if (camera_status == 2) {
    if (alarm_status == 1 or alarm_status == 2) {
      digitalWrite(camera_pin, 1);
      camera_state = 1;
    } else if (alarm_status == 0) {
      digitalWrite(camera_pin, 0);
      camera_state = 0;
    }
  } else if (camera_status == 3) {
    digitalWrite(camera_pin, 0);
    camera_state = 0;
  }

  bus.strategy.set_pin(12);
  bus.set_receiver(receiver_function);
  bus.set_synchronous_acknowledge(true);
  bus.set_crc_32(true);
  bus.begin();
}
