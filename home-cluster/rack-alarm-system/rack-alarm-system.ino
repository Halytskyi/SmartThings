/*
* Copyright (C) 2021 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#define PJON_INCLUDE_PACKET_ID
#include <PJONSoftwareBitBang.h>

const byte deviceID = 19;
PJONSoftwareBitBang bus(deviceID); // Tx bus
const byte pinBus = 12;
const byte masterIdBus = 6;

const byte buzzer = 10;
const unsigned int pushInterval = 5000; // 5 seconds
const byte signalOutput = 13;
byte signalOutputValue = 0;
const unsigned long signalOutputInterval = 60000; // 60 seconds
unsigned long signalOutputLastTriggered = 0;
byte alarmStatus = 0;
unsigned long alarmStatusLast = 0;

// Flame sensors
const byte flameSensorsNum = 9;
// {"command"}
const char *const flameSensorCmd[flameSensorsNum] = {"F-1", "F-2", "F-3", "F-4", "F-5", "F-6", "F-7", "F-8", "F-9"};
// {"pin"}
const byte flameSensorPin[flameSensorsNum] = {2, 3, 4, 5, 6, 7, 8, 9, 11};
// {"last value"}
byte flameSensorLastValue[flameSensorsNum] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
// {"last triggered"}
unsigned long flameSensorLastTriggered[flameSensorsNum] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

// Smoke sensors
const byte smokeSensorsNum = 5;
// {"command"}
const char *const smokeSensorCmd[smokeSensorsNum] = {"S-1", "S-2", "S-3", "S-4", "S-5"};
// {"pin"}
const byte smokeSensorPin[smokeSensorsNum] = {14, 15, 16, 17, 18};
// {"last value"}
byte smokeSensorLastValue[smokeSensorsNum] = {0, 0, 0, 0, 0};
// {"last triggered"}
unsigned long smokeSensorLastTriggered[smokeSensorsNum] = {0, 0, 0, 0, 0};

const byte motionSensorPin = 19;
byte motionSensorLastValue = 0;
unsigned long motionSensorLastTriggered = 0;


void alarm()  {
  tone(buzzer, 400, 500); //the buzzer emit sound at 400 MHz for 500 millis
  delay(500); //wait 500 millis
  tone(buzzer, 650, 500); //the buzzer emit sound at 650 MHz for 500 millis
  delay(500); //wait 500 millis
  digitalWrite(buzzer, HIGH); // for turn off buzzer
}

void bus_send(const char command[], const char response[]) {
  char responseFull[22]; // 21 charters + 1 NULL termination
  strcpy(responseFull, command);
  strcat(responseFull, "<");
  strcat(responseFull, response);
  bus.send_packet_blocking(masterIdBus, responseFull, strlen(responseFull));
}

void read_sensors() {
  unsigned long curMillis = millis(); // time now in ms
  // Flame sensors
  for (byte i = 0; i < flameSensorsNum; i += 1) {
    if (curMillis - flameSensorLastTriggered[i] >= pushInterval) {
      byte value = digitalRead(flameSensorPin[i]);
      if (value == HIGH) {
        value = LOW;
      } else {
        value = HIGH;
      }
      if (value != flameSensorLastValue[i]) {
        char tmpBuf[2];
        itoa(value, tmpBuf, 10);
        bus_send(flameSensorCmd[i], tmpBuf);
        flameSensorLastValue[i] = value;
        signalOutputValue = value;
        if (value == HIGH) {
          flameSensorLastTriggered[i] = curMillis;
          alarm();
        }
      }
    }
  }
  // Smoke sensors
  for (byte i = 0; i < smokeSensorsNum; i += 1) {
    if (curMillis - smokeSensorLastTriggered[i] >= pushInterval) {
      byte value = digitalRead(smokeSensorPin[i]);
      if (value == HIGH) {
        value = LOW;
      } else {
        value = HIGH;
      }
      if (value != smokeSensorLastValue[i]) {
        char tmpBuf[2];
        itoa(value, tmpBuf, 10);
        bus_send(smokeSensorCmd[i], tmpBuf);
        smokeSensorLastValue[i] = value;
        signalOutputValue = value;
        if (value == HIGH) {
          smokeSensorLastTriggered[i] = curMillis;
          alarm();
        }
      }
    }
  }
  // Motion sensor(s)
  if (curMillis - motionSensorLastTriggered >= pushInterval) {
    byte value = digitalRead(motionSensorPin);
    if (value == HIGH) {
      value = LOW;
    } else {
      value = HIGH;
    }
    if (value != motionSensorLastValue) {
      char tmpBuf[2];
      itoa(value, tmpBuf, 10);
      bus_send("M", tmpBuf);
      motionSensorLastValue = value;
      if (value == HIGH) {
        motionSensorLastTriggered = curMillis;
        alarm();
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
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - alarmStatusLast >= signalOutputInterval) {
    char tmpBuf[2];
    itoa(alarmStatus, tmpBuf, 10);
    bus_send("A", tmpBuf);
    alarmStatusLast = curMillis;
  }
}

void loop() {
  read_sensors();
  signal_output();
  send_status();
}

void setup() {
  for (byte i = 0; i < flameSensorsNum; i += 1) {
    pinMode(flameSensorPin[i], INPUT);
    digitalWrite(flameSensorPin[i], HIGH); // turn on pullup resistor
  }
  for (byte i = 0; i < smokeSensorsNum; i += 1) {
    pinMode(smokeSensorPin[i], INPUT);
    digitalWrite(smokeSensorPin[i], HIGH); // turn on pullup resistor
  }
  pinMode(motionSensorPin, INPUT);
  digitalWrite(motionSensorPin, HIGH); // turn on pullup resistor
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, HIGH); // for turn off buzzer
  pinMode(signalOutput, OUTPUT);

  bus.strategy.set_pin(pinBus);
  bus.set_acknowledge(true);
  bus.set_crc_32(true);
  bus.set_packet_id(true);
  bus.begin();
};