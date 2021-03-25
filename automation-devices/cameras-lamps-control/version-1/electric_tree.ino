/*
* Copyright (C) 2019 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#define PJON_INCLUDE_PACKET_ID true
#define PJON_INCLUDE_SWBB true // Include SoftwareBitBang
#include <PJON.h>
#include <EEPROM.h>

PJON<SoftwareBitBang> busA(22);
PJON<SoftwareBitBang> busB(22);
int master_tr_pin = 7;
int master_ro_pin = 12;
int master_id = 1;
int bus_receive_time = 7000;

#define ADDR_VERSION 255    //location of the software version in EEPROM
#define CURRENT_EEPROM_VERSION 1 //we are on revision 1 of the EEPROM storage structure (0xFF or 255 is never a valid value for version)

unsigned long prevMillis_autopush = millis();
unsigned long lastUpdateMillis = millis();
char delimiter_value = '=';
String wrong_command_reply = ">wrong command";
unsigned long min_push_interval = 3000; // interval between push data to master
unsigned long push_interval = 60000; // push interval (each sensor read once within this interval)

#define num_lamps 2
// {"lamp path", "lamp pin", "value", "automode status", "max brightness", "fade value", "enabled"}
String lamps[num_lamps][7] = {
    {"L-1", "5", "0", "0", "100", "0", "0"},
    {"L-2", "9", "0", "0", "100", "0", "0"}};
// {"automode status", "max brightness"}
int eeprom_lamps_addr[num_lamps][2] = {
    {0, 1},
    {2, 3}};
// Other lamps params
int lamps_automode_mode = 1;
int eeprom_lamps_automode_mode_addr = 4;
int lamps_automode_fade = 5;
int eeprom_lamps_automode_fade_addr = 5;
int lamps_automode_mode2_fade_value = 0;
int lamps_automode_mode2_enabled = 0;
int max_brightness;
int max_brightness_lamp1;
int max_brightness_lamp2;

// Motion sensors
#define num_m_sensors 2
// {"sensor path", "sensor pin", "value", "auto push", "time", "last update"}
String m_sensors[num_m_sensors][6] = {
    {"S-m-1", "10", "0", "0", "0", "0"},
    {"S-m-2", "11", "0", "0", "0", "0"}};
// {"auto push", "time"}
int eeprom_m_sensors_addr[num_m_sensors][2] = {
    {6, 7},
    {8, 9}};

// Light sensor
int light_sensor_pin = 0;
int light_sensor_autopush = 0;
int light_sensor_brightness = 0;
int eeprom_light_sensor_autopush_addr = 10;
int eeprom_light_sensor_brightness_addr = 11;
unsigned long light_sensor_last_update = millis();

// Camera
int camera_pin = 3;
int camera_status = 0;
int eeprom_camera_status_addr = 12;

// Voltmeter
int voltage_pin = 17;
int voltage_autopush = 0;
int eeprom_voltage_autopush_addr = 13;
float voltage_low = 10.7; // Voltage low limit
int eeprom_voltage_low_addr = 14;
float voltage_high = 12.0; // Voltage high limit
int eeprom_voltage_high_addr = 15;
float voltage_r1 = 99850; // R1 = 100k
float voltage_r2 = 9750; // R2 = 10k
int voltage_countvalues = 10; // how many values must be averaged
float voltage_5V = 4.89; // Arduino input voltage
unsigned long voltage_last_update = millis();

// Alarm
int alarm_status = 0;
int eeprom_alarm_status_addr = 16;

// Events
int event_status = 0;
int eeprom_event_status_addr = 17;


String get_voltage() {
  float voltage_curVoltage = 0.0;

  for (int i = 0; i < voltage_countvalues; i++) {
    voltage_curVoltage += analogRead(voltage_pin);
    delay(3);
  }
  voltage_curVoltage = voltage_curVoltage / voltage_countvalues;
  float v  = (voltage_curVoltage * voltage_5V) / 1024.0;
  float voltage_value = v / (voltage_r2 / (voltage_r1 + voltage_r2));
  if (voltage_value < 0.03) voltage_value = 0.0;
  return String(voltage_value);
}

void bus_reply(String response_str) {
  int response_str_len = response_str.length();
  char response[response_str_len + 1];
  response_str.toCharArray(response, response_str_len + 1);
  busA.reply(response, response_str_len);
  busA.update();
}

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  String response_str;
  String command = "";
  String value = "";
  int request_value = 0;
  for (uint8_t i = 0; i != length; i++) {
    if ((char)payload[i] == delimiter_value) {
      request_value = 1;
    } else if (request_value == 0) {
      command += (char)payload[i];
    } else {
      value += (char)payload[i];
    }
  }
  // "wrong command" if defined 'delimiter_value' but no value
  if (request_value == 1 and value == "") {
    response_str = command + delimiter_value + value + wrong_command_reply;
    bus_reply(response_str);
    return;
  }
  // Lamps
  for (int i = 0; i < num_lamps; i += 1) {
    if (command == lamps[i][0]) {
      if (value == "") {
        response_str = command + ">" + lamps[i][2];
        bus_reply(response_str);
        return;
      } else {
        int value_int = value.toInt();
        if (value_int >= 0 and value_int <= 100) {
          if (value_int == 0) {
            digitalWrite(lamps[i][1].toInt(), 0);
          } else if (value_int == 100) {
            digitalWrite(lamps[i][1].toInt(), 1);
          } else {
            int light_percent = map(value_int, 0, 100, 0, 255);
            analogWrite(lamps[i][1].toInt(), light_percent);
          }
          lamps[i][2] = value;
          response_str = command + delimiter_value + value + ">done";
          bus_reply(response_str);
          return;
        } else {
          response_str = command + delimiter_value + value + wrong_command_reply;
          bus_reply(response_str);
          return;
        }
      }
    } else if (command == lamps[i][0] + "-a") {
      if (value == "") {
        response_str = command + ">" + lamps[i][3];
        bus_reply(response_str);
        return;
      } else {
        if (value == "0" or value == "1" or value == "2") {
          if (value == "0") {
            digitalWrite(lamps[i][1].toInt(), 0);
            lamps[i][2] = value;
          }
          lamps[i][3] = value;
          EEPROM.update(eeprom_lamps_addr[i][0], value.toInt());
          response_str = command + delimiter_value + value + ">done";
          bus_reply(response_str);
          return;
        } else {
          response_str = command + delimiter_value + value + wrong_command_reply;
          bus_reply(response_str);
          return;
        }
      }
    } else if (command == lamps[i][0] + "-a-b") {
      if (value == "") {
        response_str = command + ">" + lamps[i][4];
        bus_reply(response_str);
        return;
      } else {
        int value_int = value.toInt();
        if (value_int >= 0 and value_int <= 100) {
          lamps[i][4] = value;
          EEPROM.update(eeprom_lamps_addr[i][1], value.toInt());
          response_str = command + delimiter_value + value + ">done";
          bus_reply(response_str);
          return;
        } else {
          response_str = command + delimiter_value + value + wrong_command_reply;
          bus_reply(response_str);
          return;
        }
      }
    }
  }
  if (command == "L-a-m") {
    if (value == "") {
      response_str = command + ">" + lamps_automode_mode;
      bus_reply(response_str);
      return;
    } else {
      int value_int = value.toInt();
      if (value_int == 1 or value_int == 2) {
        lamps_automode_mode = value_int;
        EEPROM.update(eeprom_lamps_automode_mode_addr, value_int);
        response_str = command + delimiter_value + value + ">done";
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    }
  } else if (command == "L-a-f") {
    if (value == "") {
      response_str = command + ">" + lamps_automode_fade;
      bus_reply(response_str);
      return;
    } else {
      int value_int = value.toInt();
      if (value_int >= 1 and value_int <= 30) {
        lamps_automode_fade = value_int;
        EEPROM.update(eeprom_lamps_automode_fade_addr, value_int);
        response_str = command + delimiter_value + value + ">done";
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    }
  }
  // Motion sensors
  for (int i = 0; i < num_m_sensors; i += 1) {
    if (command == m_sensors[i][0]) {
      if (value == "") {
        response_str = command + ">" + digitalRead(m_sensors[i][1].toInt());
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    } else if (command == m_sensors[i][0] + "-a") {
      if (value == "") {
        response_str = command + ">" + m_sensors[i][3];
        bus_reply(response_str);
        return;
      } else {
        if (value == "0" or value == "1") {
          m_sensors[i][3] = value;
          EEPROM.update(eeprom_m_sensors_addr[i][0], value.toInt());
          response_str = command + delimiter_value + value + ">done";
          bus_reply(response_str);
          return;
        } else {
          response_str = command + delimiter_value + value + wrong_command_reply;
          bus_reply(response_str);
          return;
        }
      }
    } else if (command == m_sensors[i][0] + "-t") {
      if (value == "") {
        response_str = command + ">" + m_sensors[i][4];
        bus_reply(response_str);
        return;
      } else {
        int value_int = value.toInt();
        if (value_int >= 0 and value_int <= 120) {
          m_sensors[i][4] = value;
          EEPROM.update(eeprom_m_sensors_addr[i][1], value.toInt());
          response_str = command + delimiter_value + value + ">done";
          bus_reply(response_str);
          return;
        } else {
          response_str = command + delimiter_value + value + wrong_command_reply;
          bus_reply(response_str);
          return;
        }
      }
    }
  }
  // Light sensor
  if (command == "S-l") {
    if (value == "") {
      int brightness_percent = map(analogRead(light_sensor_pin), 0, 1023, 0, 100);
      delay(10);
      response_str = command + ">" + String(brightness_percent);
      bus_reply(response_str);
      return;
    } else {
      response_str = command + delimiter_value + value + wrong_command_reply;
      bus_reply(response_str);
      return;
    }
  } else if (command == "S-l-a") {
    if (value == "") {
      response_str = command + ">" + light_sensor_autopush;
      bus_reply(response_str);
      return;
    } else {
      int value_int = value.toInt();
      if (value_int == 0 or value_int == 1) {
        light_sensor_autopush = value_int;
        EEPROM.update(eeprom_light_sensor_autopush_addr, value_int);
        response_str = command + delimiter_value + value + ">done";
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    }
  } else if (command == "S-l-b") {
    if (value == "") {
      response_str = command + ">" + light_sensor_brightness;
      bus_reply(response_str);
      return;
    } else {
      int value_int = value.toInt();
      if (value_int >= 0 and value_int <= 100) {
        light_sensor_brightness = value_int;
        EEPROM.update(eeprom_light_sensor_brightness_addr, value_int);
        response_str = command + delimiter_value + value + ">done";
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    }
  }
  // Camera
  if (command == "C") {
    if (value == "") {
      response_str = command + ">" + camera_status;
      bus_reply(response_str);
      return;
    } else {
      int value_int = value.toInt();
      if (value_int == 0 or value_int == 1) {
        digitalWrite(camera_pin, value_int);
        camera_status = value_int;
        EEPROM.update(eeprom_camera_status_addr, value_int);
        response_str = command + delimiter_value + value + ">done";
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    }
  }
  // Voltmeter
  if (command == "V") {
    if (value == "") {
      String voltage = get_voltage();
      response_str = command + ">" + voltage;
      bus_reply(response_str);
      return;
    } else {
      response_str = command + delimiter_value + value + wrong_command_reply;
      bus_reply(response_str);
      return;
    }
  } else if (command == "V-a") {
    if (value == "") {
      response_str = command + ">" + voltage_autopush;
      bus_reply(response_str);
      return;
    } else {
      int value_int = value.toInt();
      if (value_int == 0 or value_int == 1) {
        voltage_autopush = value_int;
        EEPROM.update(eeprom_voltage_autopush_addr, value_int);
        response_str = command + delimiter_value + value + ">done";
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    }
  } else if (command == "V-l-l") {
    if (value == "") {
      response_str = command + ">" + voltage_low;
      bus_reply(response_str);
      return;
    } else {
      float value_float = value.toFloat();
      if (value_float >= 9 and value_float <= 11.5) {
        voltage_low = value_float;
        EEPROM.update(eeprom_voltage_low_addr, int(value_float * 10));
        response_str = command + delimiter_value + value + ">done";
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    }
  } else if (command == "V-l-h") {
    if (value == "") {
      response_str = command + ">" + voltage_high;
      bus_reply(response_str);
      return;
    } else {
      float value_float = value.toFloat();
      if (value_float >= 11.5 and value_float <= 14) {
        voltage_high = value_float;
        EEPROM.update(eeprom_voltage_high_addr, int(value_float * 10));
        response_str = command + delimiter_value + value + ">done";
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    }
  }
  // Alarm
  if (command == "A") {
    if (value == "") {
      response_str = command + ">" + alarm_status;
      bus_reply(response_str);
      return;
    } else {
      int value_int = value.toInt();
      if (value_int == 0 or value_int == 1 or value_int == 2) {
        alarm_status = value_int;
        EEPROM.update(eeprom_alarm_status_addr, value_int);
        response_str = command + delimiter_value + value + ">done";
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    }
  }
  // Events
  if (command == "E") {
    if (value == "") {
      response_str = command + ">" + event_status;
      bus_reply(response_str);
      return;
    } else {
      int value_int = value.toInt();
      if (value_int == 0 or value_int == 1) {
        event_status = value_int;
        EEPROM.update(eeprom_event_status_addr, value_int);
        response_str = command + delimiter_value + value + ">done";
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    }
  }
  response_str = command + wrong_command_reply;
  bus_reply(response_str);
}

void bus_send(String response_str) {
  int response_str_len = response_str.length();
  char response[response_str_len + 1];
  response_str.toCharArray(response, response_str_len + 1);
  busB.send_packet_blocking(master_id, response, response_str_len);
}

void motion_detect() {
  unsigned long curMillis = millis(); // time now in ms
  for (int i = 0; i < num_m_sensors; i += 1) {
    if (m_sensors[i][4] != "0") {
      int sensor_status = digitalRead(m_sensors[i][1].toInt());
      if (sensor_status == 1 and m_sensors[i][2] == "1")
        m_sensors[i][5] = curMillis;
      unsigned long time_limit = m_sensors[i][4].toInt() * 1000;
      if (time_limit <= curMillis - m_sensors[i][5].toInt()) {
        if (sensor_status != m_sensors[i][2].toInt()) {
          m_sensors[i][2] = sensor_status;
          if (m_sensors[i][2] == "1")
            m_sensors[i][5] = curMillis;
          if (m_sensors[i][3] == "1") {
            String response_str = m_sensors[i][0] + "<" + m_sensors[i][2];
            bus_send(response_str);
          }
        }
      }
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
      if (voltage_autopush == 1) {
        if (curMillis - voltage_last_update >= push_interval and curMillis - lastUpdateMillis >= min_push_interval) {
          voltage_last_update = curMillis;
          lastUpdateMillis = curMillis;
          String response_str = "V<" + get_voltage();
          bus_send(response_str);
          msg_pushed = 1;
        }
      }
    }
    prevMillis_autopush = curMillis;
  }
}

int max_b_p(int lamp) {
  int max_brightness_percent = 0;
  if (lamps[lamp][3] == "2") {
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
  } else if (lamps[lamp][3] == "1") {
    max_brightness_percent = lamps[lamp][4].toInt();
  }
  return max_brightness_percent;
}

void lamp_automode_control1() {
  for (int i = 0; i < num_lamps; i += 1) {
    if (lamps[i][3] == "1" or lamps[i][3] == "2") {
      if ((m_sensors[i][2] == "1" and lamps[i][6] == "0") or (lamps[i][5] != "0")) {
        int brightness_percent = map(analogRead(light_sensor_pin), 0, 1023, 0, 100);
        if (brightness_percent <= light_sensor_brightness or lamps[i][5] != "0") {
          int max_brightness_percent = max_b_p(i);
          max_brightness = map(max_brightness_percent, 0, 100, 0, 255);
          for (int fadeValue = lamps[i][5].toInt(); fadeValue <= max_brightness; fadeValue += lamps_automode_fade) {
            analogWrite(lamps[i][1].toInt(), fadeValue);
            busA.receive(bus_receive_time);
            busA.update();
            delay(10);
          }
          if (max_brightness_percent == 100)
            digitalWrite(lamps[i][1].toInt(), 1);
          lamps[i][6] = "1";
          lamps[i][5] = "0";
          lamps[i][2] = max_brightness_percent;
          if (event_status == 1) {
            String response_str = lamps[i][0] + "<" + max_brightness_percent;
            bus_send(response_str);
          }
        }
      } else if (m_sensors[i][2] == "0" and lamps[i][6] == "1") {
        for (int fadeValue = max_brightness; fadeValue >= 0; fadeValue -= lamps_automode_fade) {
          analogWrite(lamps[i][1].toInt(), fadeValue);
          busA.receive(bus_receive_time);
          busA.update();
          if (digitalRead(m_sensors[i][1].toInt()) == 1) {
            m_sensors[i][2] = 1;
            lamps[i][5] = fadeValue;
            break;
          }
          delay(10);
        }
        if (lamps[i][5] == "0") {
          digitalWrite(lamps[i][1].toInt(), 0);
          lamps[i][6] = "0";
          lamps[i][2] = "0";
          if (event_status == 1) {
            String response_str = lamps[i][0] + "<" + 0;
            bus_send(response_str);
          }
        }
      }
    }
  }
}

void lamp_automode_control2() {
  if (lamps[0][3] == "1" or lamps[0][3] == "2" or lamps[1][3] == "1" or lamps[1][3] == "2") {
    if (((m_sensors[0][2] == "1" or m_sensors[1][2] == "1") and lamps_automode_mode2_enabled == 0) or (lamps_automode_mode2_fade_value != 0)) {
      int brightness_percent = map(analogRead(light_sensor_pin), 0, 1023, 0, 100);
      if (brightness_percent <= light_sensor_brightness or lamps_automode_mode2_fade_value != 0) {
        int max_brightness_percent_lamp1 = max_b_p(0);
        int max_brightness_percent_lamp2 = max_b_p(1);
        max_brightness_lamp1 = map(max_brightness_percent_lamp1, 0, 100, 0, 255);
        max_brightness_lamp2 = map(max_brightness_percent_lamp2, 0, 100, 0, 255);
        if (max_brightness_lamp1 >= max_brightness_lamp2) {
          max_brightness = max_brightness_lamp1;
        } else {
          max_brightness = max_brightness_lamp2;
        }
        for (int fadeValue = lamps_automode_mode2_fade_value; fadeValue <= max_brightness; fadeValue += lamps_automode_fade) {
          if ((fadeValue <= max_brightness_lamp1) and (lamps[0][3] == "1" or lamps[0][3] == "2")) {
            analogWrite(lamps[0][1].toInt(), fadeValue);
          }
          if ((fadeValue <= max_brightness_lamp2) and (lamps[1][3] == "1" or lamps[1][3] == "2")) {
            analogWrite(lamps[1][1].toInt(), fadeValue);
          }
          busA.receive(bus_receive_time);
          busA.update();
          delay(10);
        }
        if (max_brightness_percent_lamp1 == 100)
          digitalWrite(lamps[0][1].toInt(), 1);
        if (max_brightness_percent_lamp2 == 100)
          digitalWrite(lamps[1][1].toInt(), 1);
        lamps_automode_mode2_enabled = 1;
        lamps_automode_mode2_fade_value = 0;
        if ((lamps[0][2] != String(max_brightness_percent_lamp1)) and (lamps[0][3] == "1" or lamps[0][3] == "2")) {
          lamps[0][2] = max_brightness_percent_lamp1;
          if (event_status == 1) {
            String response_str = lamps[0][0] + "<" + max_brightness_percent_lamp1;
            bus_send(response_str);
          }
        }
        if ((lamps[1][2] != String(max_brightness_percent_lamp2)) and (lamps[1][3] == "1" or lamps[1][3] == "2")) {
          lamps[1][2] = max_brightness_percent_lamp2;
          if (event_status == 1) {
            String response_str = lamps[1][0] + "<" + max_brightness_percent_lamp2;
            bus_send(response_str);
          }
        }
      }
    } else if (m_sensors[0][2] == "0" and m_sensors[1][2] == "0" and lamps_automode_mode2_enabled == 1) {
      for (int fadeValue = max_brightness; fadeValue >= 0; fadeValue -= lamps_automode_fade) {
        if ((fadeValue <= max_brightness_lamp1) and (lamps[0][3] == "1" or lamps[0][3] == "2")) {
          analogWrite(lamps[0][1].toInt(), fadeValue);
        }
        if ((fadeValue <= max_brightness_lamp2) and (lamps[1][3] == "1" or lamps[1][3] == "2")) {
          analogWrite(lamps[1][1].toInt(), fadeValue);
        }
        busA.receive(bus_receive_time);
        busA.update();
        int motion_sensor1 = digitalRead(m_sensors[0][1].toInt());
        int motion_sensor2 = digitalRead(m_sensors[1][1].toInt());
        if (motion_sensor1 == 1 or motion_sensor2 == 1) {
          if (m_sensors[0][2] != String(motion_sensor1)) {
            m_sensors[0][2] = motion_sensor1;
          }
          if (m_sensors[1][2] != String(motion_sensor2)) {
            m_sensors[1][2] = motion_sensor1;
          }
          lamps_automode_mode2_fade_value = fadeValue;
          break;
        }
        delay(10);
      }
      if (lamps_automode_mode2_fade_value == 0) {
        if (lamps[0][3] == "1" or lamps[0][3] == "2") {
          digitalWrite(lamps[0][1].toInt(), 0);
          lamps[0][2] = "0";
          if (event_status == 1) {
            String response_str = lamps[0][0] + "<" + 0;
            bus_send(response_str);
          }
        }
        if (lamps[1][3] == "1" or lamps[1][3] == "2") {
          digitalWrite(lamps[1][1].toInt(), 0);
          lamps[1][2] = "0";
          if (event_status == 1) {
            String response_str = lamps[1][0] + "<" + 0;
            bus_send(response_str);
          }
        }
        lamps_automode_mode2_enabled = 0;
      }
    }
  }
}

void alarm () {
  if (alarm_status == 1 or alarm_status == 2) {
    max_brightness_lamp1 = map(lamps[0][4].toInt(), 0, 100, 0, 255);
    max_brightness_lamp2 = map(lamps[1][4].toInt(), 0, 100, 0, 255);
    while (alarm_status == 1) {
      float voltage = get_voltage().toFloat();
      if (voltage > voltage_low) {
        for (int i = 0; i < 5; i++) {
          busA.receive(bus_receive_time);
          busA.update();
          digitalWrite(lamps[0][1].toInt(), max_brightness_lamp1);
          delay(30);
          busA.receive(bus_receive_time);
          busA.update();
          digitalWrite(lamps[0][1].toInt(), 0);
          delay(30);
        }
        delay(100);
        for (int i = 0; i < 5; i++) {
          busA.receive(bus_receive_time);
          busA.update();
          digitalWrite(lamps[1][1].toInt(), max_brightness_lamp2);
          delay(30);
          busA.receive(bus_receive_time);
          busA.update();
          digitalWrite(lamps[1][1].toInt(), 0);
          delay(30);
        }
      }
      delay(50);
      busA.receive(bus_receive_time);
      busA.update();
      motion_detect();
      busA.receive(bus_receive_time);
      busA.update();
      autopush();
    }
    while (alarm_status == 2) {
      float voltage = get_voltage().toFloat();
      if (voltage > voltage_low) {
        for (int i = 0; i < 5; i++) {
          busA.receive(bus_receive_time);
          busA.update();
          digitalWrite(lamps[0][1].toInt(), max_brightness_lamp1);
          digitalWrite(lamps[1][1].toInt(), max_brightness_lamp2);
          busA.receive(bus_receive_time);
          busA.update();
          motion_detect();
          busA.receive(bus_receive_time);
          busA.update();
          autopush();
          delay(30);
          digitalWrite(lamps[0][1].toInt(), 0);
          digitalWrite(lamps[1][1].toInt(), 0);
          busA.receive(bus_receive_time);
          busA.update();
          motion_detect();
          busA.receive(bus_receive_time);
          busA.update();
          autopush();
          delay(30);
        }
      }
    }
  }
}

void loop() {
  busA.receive(bus_receive_time);
  busA.update();
  motion_detect();
  busA.receive(bus_receive_time);
  busA.update();
  autopush();
  busA.receive(bus_receive_time);
  busA.update();
  if (lamps_automode_mode == 1) {
    lamp_automode_control1();
  } else if (lamps_automode_mode == 2) {
    lamp_automode_control2();
  }
  busA.receive(bus_receive_time);
  busA.update();
  alarm();
}

void setup() {
  pinMode(camera_pin, OUTPUT);
  for (int i = 0; i < num_lamps; i += 1) {
    pinMode(lamps[i][1].toInt(), OUTPUT);
    digitalWrite(lamps[i][1].toInt(), 0);
  }
  for (int i = 0; i < num_m_sensors; i += 1) {
    pinMode(m_sensors[i][1].toInt(), INPUT);
  }
  pinMode(light_sensor_pin, INPUT);
  pinMode(voltage_pin, INPUT);

  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) {
    //EEprom is wrong version or was not programmed, write default values to the EEprom
    digitalWrite(camera_pin, camera_status);
    EEPROM.update(eeprom_camera_status_addr, camera_status);
    for (int i = 0; i < num_lamps; i += 1) {
      EEPROM.update(eeprom_lamps_addr[i][0], lamps[i][3].toInt());
      EEPROM.update(eeprom_lamps_addr[i][1], lamps[i][4].toInt());
    }
    EEPROM.update(eeprom_lamps_automode_mode_addr, lamps_automode_mode);
    EEPROM.update(eeprom_lamps_automode_fade_addr, lamps_automode_fade);
    for (int i = 0; i < num_m_sensors; i += 1) {
      EEPROM.update(eeprom_m_sensors_addr[i][0], m_sensors[i][3].toInt());
      EEPROM.update(eeprom_m_sensors_addr[i][1], m_sensors[i][4].toInt());
    }
    EEPROM.update(eeprom_light_sensor_autopush_addr, light_sensor_autopush);
    EEPROM.update(eeprom_light_sensor_brightness_addr, light_sensor_brightness);
    EEPROM.update(eeprom_voltage_autopush_addr, voltage_autopush);
    EEPROM.update(eeprom_voltage_low_addr, int(voltage_low * 10));
    EEPROM.update(eeprom_voltage_high_addr, int(voltage_high * 10));
    EEPROM.update(eeprom_alarm_status_addr, alarm_status);
    EEPROM.update(eeprom_event_status_addr, event_status);
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    camera_status = EEPROM.read(eeprom_camera_status_addr);
    digitalWrite(camera_pin, camera_status);
    for (int i = 0; i < num_lamps; i += 1) {
      lamps[i][3] = EEPROM.read(eeprom_lamps_addr[i][0]);
      lamps[i][4] = EEPROM.read(eeprom_lamps_addr[i][1]);
    }
    lamps_automode_mode = EEPROM.read(eeprom_lamps_automode_mode_addr);
    lamps_automode_fade = EEPROM.read(eeprom_lamps_automode_fade_addr);
    for (int i = 0; i < num_m_sensors; i += 1) {
      m_sensors[i][3] = EEPROM.read(eeprom_m_sensors_addr[i][0]);
      m_sensors[i][4] = EEPROM.read(eeprom_m_sensors_addr[i][1]);
    }
    light_sensor_autopush = EEPROM.read(eeprom_light_sensor_autopush_addr);
    light_sensor_brightness = EEPROM.read(eeprom_light_sensor_brightness_addr);
    voltage_autopush = EEPROM.read(eeprom_voltage_autopush_addr);
    voltage_low = float(EEPROM.read(eeprom_voltage_low_addr)) / 10;
    voltage_high = float(EEPROM.read(eeprom_voltage_high_addr)) / 10;
    alarm_status = EEPROM.read(eeprom_alarm_status_addr);
    event_status = EEPROM.read(eeprom_event_status_addr);
  }

  busA.strategy.set_pin(master_tr_pin);
  busA.set_receiver(receiver_function);
  busA.set_synchronous_acknowledge(true);
  busA.set_crc_32(true);
  busA.set_packet_id(true);
  busA.begin();

  busB.strategy.set_pin(master_ro_pin);
  busB.set_receiver(receiver_function);
  busB.set_synchronous_acknowledge(true);
  busB.set_crc_32(true);
  busB.set_packet_id(true);
  busB.begin();
};
