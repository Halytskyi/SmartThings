/*
* Copyright (C) 2018 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#define PJON_INCLUDE_PACKET_ID true
#define PJON_INCLUDE_SWBB true // Include SoftwareBitBang
#include <PJON.h>
#include <DHT.h>
#include <EEPROM.h>

PJON<SoftwareBitBang> busA(21);
PJON<SoftwareBitBang> busB(21);
int master_tr_pin = 7;
int master_ro_pin = 12;
int master_id = 1;
int bus_receive_time = 7000;

#define DHTTYPE DHT22
#define DHTPIN8 8 // Temperature sensor 1
#define DHTPIN18 18 // Temperature sensor 2
DHT dht1(DHTPIN8, DHTTYPE);
DHT dht2(DHTPIN18, DHTTYPE);

#define ADDR_VERSION 255    //location of the software version in EEPROM
#define CURRENT_EEPROM_VERSION 1 //we are on revision 1 of the EEPROM storage structure (0xFF or 255 is never a valid value for version)

unsigned long prevMillis_t = millis();
unsigned long prevMillis_l = millis();
unsigned long prevMillis_v = millis();
unsigned long lastUpdateMillis = millis();
char delimiter_value = '=';
String wrong_command_reply = ">wrong command";
unsigned long sensor_read_interval = 5000; // interval between sensors read, should be not often than 2 sec. for dht22 sensors
unsigned long push_interval = 60000; // push interval (each sensor read once within this interval)

int led_pin = 19;

#define num_lamps 3
// {"lamp path", "lamp pin", "value"}
String lamps[num_lamps][3] = {
    {"L-1", "3", "0"},
    {"L-2", "5", "0"},
    {"L-3", "6", "0"}};
int lamp1_enabled = 0;
int lamp1_fadeValue_start = 0;
// {"automode status", "max brightness"}
String lamp1_automode[2] = {"0", "100"};
// {"automode status", "max brightness"}
int eeprom_lamp1_addr[2] = {0, 1};
int max_brightness;

// Motion sensors
#define num_m_sensors 3
// {"sensor path", "sensor pin", "value", "auto push", "time", "last update"}
String m_sensors[num_m_sensors][6] = {
    {"S-m-1", "2", "0", "0", "0", "0"},
    {"S-m-2", "4", "0", "0", "0", "0"},
    {"S-m-3", "10", "0", "0", "0", "0"}};
// {"auto push", "time"}
int eeprom_m_sensors_addr[num_m_sensors][2] = {
    {2, 3},
    {4, 5},
    {6, 7}};

// Light sensors
#define num_l_sensors 2
// {"sensor path", "sensor pin", "auto push", "brightness"}
String l_sensors[num_l_sensors][4] = {
    {"S-l-1", "1", "0", "0"},
    {"S-l-2", "2", "0", "0"}};
// {"auto push", "brightness"}
int eeprom_l_sensors_addr[num_l_sensors][2] = {
    {8, 9},
    {10, 11}};

// temperature sensors - DHT22
#define num_t_sensors_types 2 * 3 // num_sensors * num_types
// {"sensor path", "value", "auto push", "last update", "maximum errors", "errors count"}
String t_sensors[num_t_sensors_types][6] = {
    {"S-h-1", "0", "0", "0", "10", "0"},
    {"S-t-1", "0", "0", "0", "10", "0"},
    {"S-f-1", "0", "0", "0", "10", "0"},
    {"S-h-2", "0", "0", "0", "10", "0"},
    {"S-t-2", "0", "0", "0", "10", "0"},
    {"S-f-2", "0", "0", "0", "10", "0"}};
// {"auto push", "maximum errors"}
int eeprom_t_sensors_addr[num_t_sensors_types][2] = {
    {12, 13},
    {14, 15},
    {16, 17},
    {18, 19},
    {20, 21},
    {22, 23}};

// Cameras
#define num_cams 2
// {"camera path", "camera pin", "status"}
String cams[num_cams][3] = {
    {"C-1", "11", "0"},
    {"C-2", "13", "0"}};
// {"status"}
int eeprom_cams_addr[num_cams] = {24, 25};

int voltage_pin = 6;
int voltage_automode = 0;
int eeprom_voltage_automode_addr = 26; // for "automode"
float voltage_low = 10.7; // Voltage low limit
int eeprom_voltage_low_addr = 27;
float voltage_high = 12.0; // Voltage high limit
int eeprom_voltage_high_addr = 28;
float voltage_r1 = 101000; // R1 = 100k
float voltage_r2 = 10090; // R2 = 10k
int voltage_countvalues = 10; // how many values must be averaged


String read_sensor(String sensor) {
  String value;
  if (sensor == "S-h-1") {
    value = dht1.readHumidity();
  } else if (sensor == "S-t-1") {
    value = dht1.readTemperature();
  } else if (sensor == "S-f-1") {
    value = dht1.computeHeatIndex(dht1.readTemperature(), dht1.readHumidity(), false); // Feeling temperature
  } else if (sensor == "S-h-2") {
    value = dht2.readHumidity();
  } else if (sensor == "S-t-2") {
    value = dht2.readTemperature();
  } else if (sensor == "S-f-2") {
    value = dht2.computeHeatIndex(dht2.readTemperature(), dht2.readHumidity(), false); // Feeling temperature
  }
  return value;
}

String get_voltage() {
  float voltage_curVoltage = 0.0;

  for (int i = 0; i < voltage_countvalues; i++) {
    voltage_curVoltage += analogRead(voltage_pin);
    delay(3);
  }
  voltage_curVoltage = voltage_curVoltage / voltage_countvalues;
  float v  = (voltage_curVoltage * 5.0) / 1024.0;
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
            digitalWrite(led_pin, 0);
          } else if (value_int == 100) {
            digitalWrite(lamps[i][1].toInt(), 1);
            digitalWrite(led_pin, 1);
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
        response_str = command + ">" + lamp1_automode[0];
        bus_reply(response_str);
        return;
      } else {
        if (value == "0" or value == "1" or value == "2") {
          if (value == "0") {
            digitalWrite(lamps[0][1].toInt(), 0);
            digitalWrite(led_pin, 0);
          }
          lamp1_automode[0] = value.toInt();
          EEPROM.update(eeprom_lamp1_addr[0], value.toInt());
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
        response_str = command + ">" + lamp1_automode[1];
        bus_reply(response_str);
        return;
      } else {
        int value_int = value.toInt();
        if (value_int >= 0 and value_int <= 100) {
          lamp1_automode[1] = value;
          EEPROM.update(eeprom_lamp1_addr[1], value_int);
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
  // Temperature sensors
  for (int i = 0; i < num_t_sensors_types; i += 1) {
    if (command == t_sensors[i][0]) {
      if (value == "") {
        response_str = command + ">" + read_sensor(t_sensors[i][0]);
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    } else if (command == t_sensors[i][0] + "-a") {
      if (value == "") {
        response_str = command + ">" + t_sensors[i][2];
        bus_reply(response_str);
        return;
      } else {
        if (value == "0" or value == "1") {
          t_sensors[i][2] = value;
          EEPROM.update(eeprom_t_sensors_addr[i][0], value.toInt());
          response_str = command + delimiter_value + value + ">done";
          bus_reply(response_str);
          return;
        } else {
          response_str = command + delimiter_value + value + wrong_command_reply;
          bus_reply(response_str);
          return;
        }
      }
    } else if (command == t_sensors[i][0] + "-m") {
      if (value == "") {
        response_str = command + ">" + t_sensors[i][4];
        bus_reply(response_str);
        return;
      } else {
        int value_int = value.toInt();
        if (value_int >= 0 and value_int <= 99) {
          t_sensors[i][4] = value;
          EEPROM.update(eeprom_t_sensors_addr[i][1], value_int);
          response_str = command + delimiter_value + value + ">done";
          bus_reply(response_str);
          return;
        } else {
          response_str = command + delimiter_value + value + wrong_command_reply;
          bus_reply(response_str);
          return;
        }
      }
    } else if (command == t_sensors[i][0] + "-e") {
      if (value == "") {
        response_str = command + ">" + t_sensors[i][5];
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
          EEPROM.update(eeprom_m_sensors_addr[i][1], value_int);
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
  // Light sensors
  for (int i = 0; i < num_l_sensors; i += 1) {
    if (command == l_sensors[i][0]) {
      if (value == "") {
        int brightness_percent = map(analogRead(l_sensors[i][1].toInt()), 0, 1023, 0, 100);
        response_str = command + ">" + String(brightness_percent);
        bus_reply(response_str);
        return;
      } else {
        response_str = command + delimiter_value + value + wrong_command_reply;
        bus_reply(response_str);
        return;
      }
    } else if (command == l_sensors[i][0] + "-a") {
      if (value == "") {
        response_str = command + ">" + l_sensors[i][2];
        bus_reply(response_str);
        return;
      } else {
        if (value == "0" or value == "1") {
          l_sensors[i][2] = value;
          EEPROM.update(eeprom_l_sensors_addr[i][0], value.toInt());
          response_str = command + delimiter_value + value + ">done";
          bus_reply(response_str);
          return;
        } else {
          response_str = command + delimiter_value + value + wrong_command_reply;
          bus_reply(response_str);
          return;
        }
      }
    } else if (command == l_sensors[i][0] + "-b") {
      if (value == "") {
        response_str = command + ">" + l_sensors[i][3];
        bus_reply(response_str);
        return;
      } else {
        int value_int = value.toInt();
        if (value_int >= 0 and value_int <= 100) {
          l_sensors[i][3] = value;
          EEPROM.update(eeprom_l_sensors_addr[i][1], value_int);
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
  // Cameras
  for (int i = 0; i < num_cams; i += 1) {
    if (command == cams[i][0]) {
      if (value == "") {
        response_str = command + ">" + cams[i][2];
        bus_reply(response_str);
        return;
      } else {
        int value_int = value.toInt();
        if (value_int == 0 or value_int == 1) {
          if (value_int == 0) {
            digitalWrite(cams[i][1].toInt(), 0);
            EEPROM.update(eeprom_cams_addr[i], value_int);
          } else if (value_int == 1) {
            digitalWrite(cams[i][1].toInt(), 1);
            EEPROM.update(eeprom_cams_addr[i], value_int);
          }
          cams[i][2] = value;
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
  // Voltmeter
  if (command == "V") {
    if (value == "") {
      response_str = command + ">" + String(get_voltage());
      bus_reply(response_str);
      return;
    } else {
      response_str = command + delimiter_value + value + wrong_command_reply;
      bus_reply(response_str);
      return;
    }
  } else if (command == "V-a") {
    if (value == "") {
      response_str = command + ">" + String(voltage_automode);
      bus_reply(response_str);
      return;
    } else {
      if (value == "0" or value == "1") {
        voltage_automode = value.toInt();
        EEPROM.update(eeprom_voltage_automode_addr, value.toInt());
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

void lamp1_automode_control() {
  if (lamp1_automode[0] == "1" or lamp1_automode[0] == "2") {
    if ((m_sensors[0][2] == "1" and lamp1_enabled == 0) or (lamp1_fadeValue_start != 0)) {
      int brightness_percent = map(analogRead(l_sensors[0][1].toInt()), 0, 1023, 0, 100);
      if (brightness_percent <= l_sensors[0][3].toInt() or lamp1_fadeValue_start != 0) {
        int max_brightness_percent;
        if (lamp1_automode[0] == "2") {
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
          max_brightness_percent = lamp1_automode[1].toInt();
        }
        max_brightness = map(max_brightness_percent, 0, 100, 0, 255);
        for (int fadeValue = lamp1_fadeValue_start; fadeValue <= max_brightness; fadeValue += 5) {
          analogWrite(lamps[0][1].toInt(), fadeValue);
          busA.receive(bus_receive_time);
          busA.update();
          delay(50);
        }
        if (max_brightness_percent == 100)
          digitalWrite(lamps[0][1].toInt(), 1);
        digitalWrite(led_pin, 1);
        lamp1_enabled = 1;
        lamp1_fadeValue_start = 0;
        lamps[0][2] = max_brightness_percent;
        String response_str = lamps[0][0] + "<" + max_brightness_percent;
        bus_send(response_str);
      }
    } else if (m_sensors[0][2] == "0" and lamp1_enabled == 1) {
      for (int fadeValue = max_brightness; fadeValue >= 0; fadeValue -= 5) {
        analogWrite(lamps[0][1].toInt(), fadeValue);
        busA.receive(bus_receive_time);
        busA.update();
        if (digitalRead(m_sensors[0][1].toInt()) == 1) {
          m_sensors[0][2] = 1;
          lamp1_fadeValue_start = fadeValue;
          break;
        }
        delay(50);
      }
      if (lamp1_fadeValue_start == 0) {
        digitalWrite(lamps[0][1].toInt(), 0);
        digitalWrite(led_pin, 0);
        lamp1_enabled = 0;
        lamps[0][2] = "0";
        String response_str = lamps[0][0] + "<" + 0;
        bus_send(response_str);
      }
    }
  }
}

void temperature_automode() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillis_t >= sensor_read_interval) {
    for (int i = 0; i < num_t_sensors_types; i += 1) {
      if (t_sensors[i][2] == "1") {
        if (curMillis - t_sensors[i][3].toInt() >= push_interval and curMillis - lastUpdateMillis >= sensor_read_interval) {
          String value = read_sensor(t_sensors[i][0]);
          if (value != " NAN") {
            t_sensors[i][1] = value;
            t_sensors[i][3] = curMillis;
            t_sensors[i][5] = "0";
            lastUpdateMillis = curMillis;
            String response_str = t_sensors[i][0] + "<" + value;
            bus_send(response_str);
          } else {
            if (t_sensors[i][5].toInt() < t_sensors[i][4].toInt()) {
              t_sensors[i][5] = t_sensors[i][5].toInt() + 1;
            } else {
              t_sensors[i][2] = "0";
              EEPROM.update(eeprom_t_sensors_addr[i][0], 0);
              String response_str = t_sensors[i][0] + "-a<0";
              bus_send(response_str);
              String response_str_e = t_sensors[i][0] + "-e<" + t_sensors[i][5];
              bus_send(response_str_e);
            }
          }
        }
      }
    }
    prevMillis_t = curMillis;
  }
}

void light_sensors_automode() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillis_l >= push_interval) {
    for (int i = 0; i < num_l_sensors; i += 1) {
      if (l_sensors[i][2] == "1") {
        int brightness_percent = map(analogRead(l_sensors[i][1].toInt()), 0, 1023, 0, 100);
        String response_str = l_sensors[i][0] + "<" + brightness_percent;
        bus_send(response_str);
      }
    }
    prevMillis_l = curMillis;
  }
}

void volt_automode() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillis_v >= push_interval) {
    String response_str = "V<" + get_voltage();
    bus_send(response_str);
    prevMillis_v = curMillis;
  }
}


void loop() {
  busA.receive(bus_receive_time);
  busA.update();
  motion_detect();

  busA.receive(bus_receive_time);
  busA.update();
  lamp1_automode_control();

  busA.receive(bus_receive_time);
  busA.update();
  temperature_automode();

  busA.receive(bus_receive_time);
  busA.update();
  light_sensors_automode();

  if (voltage_automode == 1) {
    busA.receive(bus_receive_time);
    busA.update();
    volt_automode();
  }
};

void setup() {
  for (int i = 0; i < num_lamps; i += 1) {
    pinMode(lamps[i][1].toInt(), OUTPUT);
    digitalWrite(lamps[i][1].toInt(), 0);
  }
  for (int i = 0; i < num_m_sensors; i += 1) {
    pinMode(m_sensors[i][1].toInt(), INPUT);
  }
  for (int i = 0; i < num_cams; i += 1) {
    pinMode(cams[i][1].toInt(), OUTPUT);
  }
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, 0);
  analogWrite(voltage_pin, 0); // prepare voltage analog PIN

  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) {
    //EEprom is wrong version or was not programmed, write default values to the EEprom
    EEPROM.update(eeprom_lamp1_addr[0], lamp1_automode[0].toInt());
    EEPROM.update(eeprom_lamp1_addr[1], lamp1_automode[1].toInt());
    for (int i = 0; i < num_t_sensors_types; i += 1) {
      EEPROM.update(eeprom_t_sensors_addr[i][0], t_sensors[i][2].toInt());
      EEPROM.update(eeprom_t_sensors_addr[i][1], t_sensors[i][4].toInt());
    }
    for (int i = 0; i < num_m_sensors; i += 1) {
      EEPROM.update(eeprom_m_sensors_addr[i][0], m_sensors[i][3].toInt());
      EEPROM.update(eeprom_m_sensors_addr[i][1], m_sensors[i][4].toInt());
    }
    for (int i = 0; i < num_l_sensors; i += 1) {
      EEPROM.update(eeprom_l_sensors_addr[i][0], l_sensors[i][2].toInt());
      EEPROM.update(eeprom_l_sensors_addr[i][1], l_sensors[i][3].toInt());
    }
    for (int i = 0; i < num_cams; i += 1) {
      digitalWrite(cams[i][1].toInt(), cams[i][2].toInt());
      EEPROM.update(eeprom_cams_addr[i], cams[i][2].toInt());
    }
    EEPROM.update(eeprom_voltage_automode_addr, voltage_automode);
    EEPROM.update(eeprom_voltage_low_addr, int(voltage_low * 10));
    EEPROM.update(eeprom_voltage_high_addr, int(voltage_high * 10));
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    lamp1_automode[0] = EEPROM.read(eeprom_lamp1_addr[0]);
    lamp1_automode[1] = EEPROM.read(eeprom_lamp1_addr[1]);
    for (int i = 0; i < num_t_sensors_types; i += 1) {
      t_sensors[i][2] = EEPROM.read(eeprom_t_sensors_addr[i][0]);
      t_sensors[i][4] = EEPROM.read(eeprom_t_sensors_addr[i][1]);
    }
    for (int i = 0; i < num_m_sensors; i += 1) {
      m_sensors[i][3] = EEPROM.read(eeprom_m_sensors_addr[i][0]);
      m_sensors[i][4] = EEPROM.read(eeprom_m_sensors_addr[i][1]);
    }
    for (int i = 0; i < num_l_sensors; i += 1) {
      l_sensors[i][2] = EEPROM.read(eeprom_l_sensors_addr[i][0]);
      l_sensors[i][3] = EEPROM.read(eeprom_l_sensors_addr[i][1]);
    }
    for (int i = 0; i < num_cams; i += 1) {
      cams[i][2] = EEPROM.read(eeprom_cams_addr[i]);
      digitalWrite(cams[i][1].toInt(), cams[i][2].toInt());
    }
    voltage_automode = EEPROM.read(eeprom_voltage_automode_addr);
    voltage_low = float(EEPROM.read(eeprom_voltage_low_addr)) / 10;
    voltage_high = float(EEPROM.read(eeprom_voltage_high_addr)) / 10;
  }

  dht1.begin();
  dht2.begin();

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
