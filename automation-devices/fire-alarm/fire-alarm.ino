/*
* Copyright (C) 2020 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#define PJON_INCLUDE_PACKET_ID true
#define PJON_INCLUDE_SWBB true // Include SoftwareBitBang
#include <PJON.h>

const uint8_t DEVICE_ID = 19;

PJON<SoftwareBitBang> bus(DEVICE_ID); // Tx bus
int pinBus = 12;
int masterIdBus = 2;

int buzzer = 10;
unsigned int pushInterval = 5000; // 5 seconds
int signalOutput = 13;
int signalOutputValue = 0;
unsigned long signalOutputInterval = 60000; // 60 seconds
unsigned long signalOutputLastTriggered = 0;
int alarmStatus = 0;
unsigned long alarmStatusLast = 0;

// Fire sensors
#define numFireSensors 9
// {"command", "pin", "last value", "last triggered"}
String fireSensor[numFireSensors][4] = {
    {"F-1", "2", "0", "0"},
    {"F-2", "3", "0", "0"},
    {"F-3", "4", "0", "0"},
    {"F-4", "5", "0", "0"},
    {"F-5", "6", "0", "0"},
    {"F-6", "7", "0", "0"},
    {"F-7", "8", "0", "0"},
    {"F-8", "9", "0", "0"},
    {"F-9", "11", "0", "0"}};

// Smoke sensors
#define numSmokeSensors 6
// {"command", "pin", "last value", "last triggered"}
String smokeSensor[numSmokeSensors][4] = {
    {"S-1", "14", "0", "0"},
    {"S-2", "15", "0", "0"},
    {"S-3", "16", "0", "0"},
    {"S-4", "17", "0", "0"},
    {"S-5", "18", "0", "0"},
    {"S-6", "19", "0", "0"}};

void alarm()  {
  tone(buzzer, 400, 500); //the buzzer emit sound at 400 MHz for 500 millis
  delay(500); //wait 500 millis
  tone(buzzer, 650, 500); //the buzzer emit sound at 650 MHz for 500 millis
  delay(500); //wait 500 millis
}

void bus_send(String msgStr) {
  int msgStrLen = msgStr.length();
  char response[msgStrLen + 1];
  msgStr.toCharArray(response, msgStrLen + 1);
  bus.send_packet_blocking(masterIdBus, response, msgStrLen);
}

void read_sensors() {
  unsigned long curMillis = millis(); // time now in ms
  // Fire sensors
  for (int i = 0; i < numFireSensors; i += 1) {
    if (curMillis - fireSensor[i][3].toInt() >= pushInterval) {
      int value = digitalRead(fireSensor[i][1].toInt());
      if (value == HIGH) {
        value = LOW;
      } else {
        value = HIGH;
      }
      if (value != fireSensor[i][2].toInt()) {
        String msgStr = fireSensor[i][0] + "<" + String(value);
        bus_send(msgStr);
        fireSensor[i][2] = String(value);
        signalOutputValue = value;
        if (value == HIGH) {
          fireSensor[i][3] = curMillis;
          alarm();
        }
      }
    }
  }
  // Smoke sensors
  for (int i = 0; i < numSmokeSensors; i += 1) {
    if (curMillis - smokeSensor[i][3].toInt() >= pushInterval) {
      int value = digitalRead(smokeSensor[i][1].toInt());
      if (value == HIGH) {
        value = LOW;
      } else {
        value = HIGH;
      }
      if (value != smokeSensor[i][2].toInt()) {
        String msgStr = smokeSensor[i][0] + "<" + String(value);
        bus_send(msgStr);
        smokeSensor[i][2] = String(value);
        signalOutputValue = value;
        if (value == HIGH) {
          smokeSensor[i][3] = curMillis;
          alarm();
        }
      }
    }
  }
}

void signal_output() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - signalOutputLastTriggered >= signalOutputInterval) {
    digitalWrite(signalOutput, signalOutputValue);
    alarmStatus = signalOutputValue;
  }
  if (signalOutputValue == HIGH) {
    signalOutputLastTriggered = curMillis;
  }
}

void send_status() {
  String value = "";
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - alarmStatusLast >= signalOutputInterval) {
    String msgStr = "A<" + String(alarmStatus);
    bus_send(msgStr);
    alarmStatusLast = curMillis;
  }
}

void loop() {
  read_sensors();
  signal_output();
  send_status();
}

void setup() {
  for (int i = 0; i < numFireSensors; i += 1) {
    pinMode(fireSensor[i][1].toInt(), INPUT);
    digitalWrite(fireSensor[i][1].toInt(), HIGH); // turn on pullup resistor
  }
  for (int i = 0; i < numSmokeSensors; i += 1) {
    pinMode(smokeSensor[i][1].toInt(), INPUT);
    digitalWrite(smokeSensor[i][1].toInt(), HIGH); // turn on pullup resistor
  }
  pinMode(buzzer, OUTPUT);
  pinMode(signalOutput, OUTPUT);

  bus.strategy.set_pin(pinBus);
  bus.set_synchronous_acknowledge(true);
  bus.set_crc_32(true);
  bus.set_packet_id(true);
  bus.begin();
};