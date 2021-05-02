/*
* Copyright (C) 2021 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#define PJON_INCLUDE_PACKET_ID
#include <PJONSoftwareBitBang.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const byte deviceID = 21;
PJONSoftwareBitBang busA(deviceID); // TxRx bus
PJONSoftwareBitBang busB(deviceID); // Tx bus
const byte pinBusA = 7;
const byte pinBusB = 12;
const byte masterIdBusB = 6;
const int receiveTimeBusA = 7000;
const byte maxRequestLength = 15;

// EEProm configuration
const byte ADDR_VERSION = 255; //location of the software version in EEPROM
const byte CURRENT_EEPROM_VERSION = 1; //we are on revision 1 of the EEPROM storage structure (0xFF or 255 is never a valid value for version)

// For auto-push
const unsigned int minAutoPushInterval = 3000; // interval between push data to master
const unsigned int autoPushInterval = 60000; // auto-push interval for each sensor
unsigned long prevMillisAutoPush = 0;
unsigned long lastUpdateMillisAutoPush = 0;
// For Fans auto-control
unsigned long prevMillisFanAutoControl = 0; 

// Fans
const byte fansNum = 4;
// {"command"}
const char *const fanCmd[fansNum] = {"F-1", "F-2", "F-3", "F-4"};
// {"pin"}
const byte fanPin[fansNum] = {3, 5, 6, 9};
// {"fan speed in %"}
byte fanSpeedPercent[fansNum] = {0, 0, 0, 0};
// {"fan automatic control mode"}
byte fanAutoMode[fansNum] = {0, 0, 0, 0};
// {"temperature low limit"}
byte fanTempLowLimit[fansNum] = {22, 22, 22, 22};
// {"temperature high limit"}
byte fanTempHighLimit[fansNum] = {30, 30, 30, 30};
// {"auto-push"}
byte fanAutoPush[fansNum] = {0, 0, 0, 0};
// {"auto-push last update"}
unsigned long fanAutoPushLU[fansNum] = {0, 0, 0, 0};
// {"EEProm fan speed in %"}
const byte eepromFanSpeedPercentAddr[fansNum] = {1, 2, 3, 4};
// {"EEProm fan state"}
const byte eepromFanAutoModeAddr[fansNum] = {5, 6, 7, 8};
// {"EEProm temperature low limit"}
const byte eepromFanTempLowLimitAddr[fansNum] = {9, 10, 11, 12};
// {"EEProm temperature high limit"}
const byte eepromFanTempHighLimitAddr[fansNum] = {13, 14, 15, 16};
// {"EEProm auto-push"}
const byte eepromFanAutoPushAddr[fansNum] = {17, 18, 19, 20};

// Temperature sensors
const byte tSensorsNum = 8;
OneWire oneWire(11);
DallasTemperature sensors(&oneWire);
// {"command"}
const char *const tSensorCmd[tSensorsNum] = {"T-1", "T-2", "T-3", "T-4", "T-5", "T-6", "T-7", "T-8"};
// {"value in auto mode"}
float tSensorAutomodeValue[tSensorsNum] = {0, 0, 0, 0, 0, 0, 0, 0};
// {"auto-push"}
byte tSensorAutoPush[tSensorsNum] = {0, 0, 0, 0, 0, 0, 0, 0};
// {"auto-push last update"}
unsigned long tSensorAutoPushLU[tSensorsNum] = {0, 0, 0, 0, 0, 0, 0, 0};
// {"EEProm auto-push"}
const byte eepromTSensorAutoPushAddr[tSensorsNum] = {21, 22, 23, 24, 25, 26, 27, 28};
const DeviceAddress tSensorAddr[tSensorsNum] = {
  {0x28, 0xA0, 0x17, 0x16, 0xA8, 0x01, 0x3C, 0x54},
  {...},
  {...},
  {...},
  {...},
  {...},
  {...},
  {...}};

// External alarm status
const byte alarmExternalPin = 10;
byte alarmExternalStatus = 0;
byte alarmExternalStatusFansDisabled = 0;
const byte eepromAlarmExternalStatus = 29;
unsigned long alarmExternalStatusAutoPushLastUpdate = 0;


float get_temperature(const byte i) {
  sensors.requestTemperaturesByAddress(tSensorAddr[i]);
  delay(100);
  float tempC = sensors.getTempC(tSensorAddr[i]);
  if (tempC == DEVICE_DISCONNECTED_C) {
    tempC = sqrt(-1);
  }
  return tempC;
}

void busA_reply(const char request[], const char response[]) {
  char responseFull[maxRequestLength + 5];
  strcpy(responseFull, request);
  strcat(responseFull, ">");
  strcat(responseFull, response);
  busA.reply_blocking(responseFull, strlen(responseFull));
}

void busB_send(const char command[], const char response[]) {
  char responseFull[22]; // 21 charters + 1 NULL termination
  strcpy(responseFull, command);
  strcat(responseFull, "<");
  strcat(responseFull, response);
  busB.send_packet_blocking(masterIdBusB, responseFull, strlen(responseFull));
}

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  char tmpBuf[8]; // for able storing number like "-127.00" + 1 NULL termination
  const char goodCommandReply[] = "ok";
  const char badCommandReply[] = "err";
  // Check if request not too long
  if (length > maxRequestLength) {
    char response[14] = "max_req_is_"; // Length string + 2 digits + 1 NULL termination
    itoa(maxRequestLength, tmpBuf, 10);
    strcat(response, tmpBuf);
    busA_reply(badCommandReply, response);
    return;
  }

  // Copy full request to new char array
  char request[length + 1];
  for (byte i = 0; i != length; i++) {
    request[i] = (char)payload[i];
  }
  request[length] = 0;

  const char delimiter[2] = "=";
  byte delimiterAddr = 0;
  byte commandLength = length;
  byte valueLength = 0;

  for (byte i = 0; i != length; i++) {
    if ((char)payload[i] == delimiter[0]) {
      delimiterAddr = i;
      commandLength = i;
      valueLength = length - i - 1;
      break;
    }
  }
  
  char command[commandLength + 1];
  char value[valueLength + 1];
  if (delimiterAddr == 0) {
    for (byte i = 0; i != length; i++) {
      command[i] = (char)payload[i];
    }
    command[commandLength] = 0;
    value[0] = 0;
  } else {
    for (byte i = 0; i != delimiterAddr; i++) {
      command[i] = (char)payload[i];
    }
    command[commandLength] = 0;
    for (byte i = 0; i != valueLength; i++) {
      value[i] = (char)payload[delimiterAddr + 1 + i];
    }
    value[valueLength] = 0;
  }

  // Return "err" if defined "delimiter" but no "value"
  if (delimiterAddr != 0 and valueLength == 0) {
    busA_reply(request, badCommandReply);
    return;
  }

  // Check if "value" contain no more 3 symbols and if it's consists only digits
  if (valueLength > 3) {
    busA_reply(request, badCommandReply);
    return;
  }
  for (byte i = 0; i != valueLength; i++) {
    if (! isDigit(value[i])) {
      busA_reply(request, badCommandReply);
      return;
    }
  }
  const byte value_int = atoi(value);

  // Fans
  for (int i = 0; i < fansNum; i += 1) {
    if (strcmp(command, fanCmd[i]) == 0) {
      if (valueLength == 0) {
        itoa(fanSpeedPercent[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else if (alarmExternalStatus == 1) {
          busA_reply(request, badCommandReply);
          return;
      } else {
        if (value_int >= 0 and value_int <= 100) {
          if (value_int == 0) {
            digitalWrite(fanPin[i], LOW);
          } else if (value_int == 100) {
            digitalWrite(fanPin[i], HIGH);
          } else {
            int fanSpeed = map(value_int, 1, 99, 90, 200);
            analogWrite(fanPin[i], fanSpeed);
          }
          fanSpeedPercent[i] = value_int;
          busA_reply(request, goodCommandReply);
          alarmExternalStatusFansDisabled = 0;
          EEPROM.update(eepromFanSpeedPercentAddr[i], value_int);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, fanCmd[i]);
    strcat(tmpBuf, "-a");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(fanAutoPush[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1) {
          fanAutoPush[i] = value_int;
          EEPROM.update(eepromFanAutoPushAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, fanCmd[i]);
    strcat(tmpBuf, "-ac");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(fanAutoMode[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1 or value_int == 2) {
          fanAutoMode[i] = value_int;
          tSensorAutomodeValue[i] = 0.0; // reset temperature to force changes
          EEPROM.update(eepromFanAutoModeAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, fanCmd[i]);
    strcat(tmpBuf, "-tl");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(fanTempLowLimit[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int >= 20 and value_int <= 25) {
          fanTempLowLimit[i] = value_int;
          tSensorAutomodeValue[i] = 0.0; // reset temperature to force changes
          EEPROM.update(eepromFanTempLowLimitAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, fanCmd[i]);
    strcat(tmpBuf, "-th");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(fanTempHighLimit[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int >= 26 and value_int <= 39) {
          fanTempHighLimit[i] = value_int;
          tSensorAutomodeValue[i] = 0.0; // reset temperature to force changes
          EEPROM.update(eepromFanTempHighLimitAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  // Temperature sensors
  for (byte i = 0; i < tSensorsNum; i += 1) {
    if (strcmp(command, tSensorCmd[i]) == 0) {
      if (valueLength == 0) {
        dtostrf(get_temperature(i), 0, 2, tmpBuf);
        busA_reply(request, tmpBuf);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
    strcpy(tmpBuf, tSensorCmd[i]);
    strcat(tmpBuf, "-a");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(tSensorAutoPush[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1) {
          tSensorAutoPush[i] = value_int;
          EEPROM.update(eepromTSensorAutoPushAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  // External alarm status
  if (strcmp(command, "A") == 0) {
    if (valueLength == 0) {
      itoa(alarmExternalStatus, tmpBuf, 10);
      busA_reply(request, tmpBuf);
      return;
    } else {
      if (value_int == 0 or value_int == 1) {
        alarmExternalStatus = value_int;
        alarmExternalStatusFansDisabled = 0;
        EEPROM.update(eepromAlarmExternalStatus, value_int);
        busA_reply(request, goodCommandReply);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
  }
  busA_reply(request, badCommandReply);
}

void autopush() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillisAutoPush >= minAutoPushInterval) {
    byte msgPushed = 0;
    for (byte i = 0; i < fansNum; i += 1) {
      if (fanAutoPush[i] == 1) {
        if (curMillis - fanAutoPushLU[i] >= autoPushInterval and curMillis - lastUpdateMillisAutoPush >= minAutoPushInterval) {
          char tmpBuf[4];
          itoa(fanSpeedPercent[i], tmpBuf, 10);
          busB_send(fanCmd[i], tmpBuf);
          fanAutoPushLU[i] = curMillis;
          lastUpdateMillisAutoPush = curMillis;
          msgPushed = 1;
        }
      }
    }
    if (msgPushed == 0) {
      for (byte i = 0; i < tSensorsNum; i += 1) {
        if (tSensorAutoPush[i] == 1) {
          if (curMillis - tSensorAutoPushLU[i] >= autoPushInterval and curMillis - lastUpdateMillisAutoPush >= minAutoPushInterval) {
            char tmpBuf[8];
            dtostrf(get_temperature(i), 0, 2, tmpBuf);
            busB_send(tSensorCmd[i], tmpBuf);
            tSensorAutoPushLU[i] = curMillis;
            lastUpdateMillisAutoPush = curMillis;
            msgPushed = 1;
          }
        }
      }
    }
    if (msgPushed == 0) {
      if (alarmExternalStatus == 1) {
        if (curMillis - alarmExternalStatusAutoPushLastUpdate >= autoPushInterval and curMillis - lastUpdateMillisAutoPush >= minAutoPushInterval) {
          busB_send("A", "1");
          alarmExternalStatusAutoPushLastUpdate = curMillis;
          lastUpdateMillisAutoPush = curMillis;
          msgPushed = 1;
        }
      }
    }
    prevMillisAutoPush = curMillis;
  }
}

void fan_auto_control() {
  int tempSensorPos1 = 0;
  int tempSensorPos2 = 0;
  for (byte i = 0; i < fansNum; i += 1) {
    if (fanAutoMode[i] == 1 or fanAutoMode[i] == 2) {
      tempSensorPos1 = i * 2;
      tempSensorPos2 = i * 2 + 1;
      float tempC1 = get_temperature(tempSensorPos1);
      float tempC2 = get_temperature(tempSensorPos2);
      if (tempC1 != tSensorAutomodeValue[tempSensorPos1] or tempC2 != tSensorAutomodeValue[tempSensorPos2]) {
        if (tempC1 < fanTempLowLimit[i] and tempC2 < fanTempLowLimit[i]) { // temp low limit
          fanSpeedPercent[i] = 0; // fan speed in %
          digitalWrite(fanPin[i], LOW);
        } else if (tempC1 > fanTempHighLimit[i] or tempC2 > fanTempHighLimit[i]) { // temp high limit
          fanSpeedPercent[i] = 100; // fan speed in %
          digitalWrite(fanPin[i], HIGH);
        } else if (fanAutoMode[i] == 2 and (tempC1 >= fanTempLowLimit[i] and tempC1 <= fanTempHighLimit[i] or tempC2 >= fanTempLowLimit[i] and tempC2 <= fanTempHighLimit[i])) {
          float tempC = tempC1;
          if (tempC2 > tempC1) {
            tempC = tempC2;
          }
          byte fanSpeed = map(tempC, fanTempLowLimit[i], fanTempHighLimit[i], 90, 200); // define fan speed in analog value
          fanSpeedPercent[i] = map(fanSpeed, 90, 200, 1, 99); // fan speed in %
          analogWrite(fanPin[i], fanSpeed); // spin the fan at the fanSpeed speed
        }
      }
      tSensorAutomodeValue[tempSensorPos1] = tempC1;
      tSensorAutomodeValue[tempSensorPos2] = tempC2;
    }
  }
}

void external_alarm() {
  byte pinStatus = digitalRead(alarmExternalPin);
  if (pinStatus == HIGH and pinStatus != alarmExternalStatus) {
    delay(100); // avoid fake alarm
    pinStatus = digitalRead(alarmExternalPin);
    if (pinStatus == HIGH and pinStatus != alarmExternalStatus) {
      alarmExternalStatus = pinStatus;
      EEPROM.update(eepromAlarmExternalStatus, alarmExternalStatus);
    }
  }
  if (alarmExternalStatus == 1 and alarmExternalStatusFansDisabled == 0) {
    for (byte i = 0; i < fansNum; i += 1) {
      fanSpeedPercent[i] = 0;
      fanAutoMode[i] = 0;
      digitalWrite(fanPin[i], LOW);
      EEPROM.update(eepromFanSpeedPercentAddr[i], 0);
      EEPROM.update(eepromFanAutoModeAddr[i], 0);
    }
    alarmExternalStatusFansDisabled = 1;
    busB_send("A", "1");
  }
}

void loop() {
  busA.receive(receiveTimeBusA);
  autopush();
  external_alarm();

  // run each 10 seconds
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillisFanAutoControl >= 10000) {
    busA.receive(receiveTimeBusA);
    fan_auto_control();
    prevMillisFanAutoControl = curMillis;
  }
}

void setup() {
  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) { //EEprom is wrong version or was not programmed, write default values to the EEprom
    for (byte i = 0; i < fansNum; i += 1) {
      EEPROM.update(eepromFanSpeedPercentAddr[i], fanSpeedPercent[i]);
      EEPROM.update(eepromFanAutoModeAddr[i], fanAutoMode[i]);
      EEPROM.update(eepromFanTempLowLimitAddr[i], fanTempLowLimit[i]);
      EEPROM.update(eepromFanTempHighLimitAddr[i], fanTempHighLimit[i]);
      EEPROM.update(eepromFanAutoPushAddr[i], fanAutoPush[i]);
    }
    for (byte i = 0; i < tSensorsNum; i += 1) {
      EEPROM.update(eepromTSensorAutoPushAddr[i], tSensorAutoPush[i]);
    }
    EEPROM.update(eepromAlarmExternalStatus, alarmExternalStatus);
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    for (byte i = 0; i < fansNum; i += 1) {
      fanSpeedPercent[i] = EEPROM.read(eepromFanSpeedPercentAddr[i]);
      fanAutoMode[i] = EEPROM.read(eepromFanAutoModeAddr[i]);
      fanTempLowLimit[i] = EEPROM.read(eepromFanTempLowLimitAddr[i]);
      fanTempHighLimit[i] = EEPROM.read(eepromFanTempHighLimitAddr[i]);
      fanAutoPush[i] = EEPROM.read(eepromFanAutoPushAddr[i]);
    }
    for (byte i = 0; i < tSensorsNum; i += 1) {
      tSensorAutoPush[i] = EEPROM.read(eepromTSensorAutoPushAddr[i]);
    }
    alarmExternalStatus = EEPROM.read(eepromAlarmExternalStatus);
  }

  // Set Fans control mode
  for (byte i = 0; i < fansNum; i += 1) {
    pinMode(fanPin[i], OUTPUT);
    if (fanSpeedPercent[i] == 0) {
      digitalWrite(fanPin[i], LOW);
    } else if (fanSpeedPercent[i] == 100) {
      digitalWrite(fanPin[i], HIGH);
    } else {
      byte fanSpeed = map(fanSpeedPercent[i], 1, 99, 90, 200);
      analogWrite(fanPin[i], fanSpeed);
    }
  }

  // Set temperature sensor resolution, valid values are 9, 10, 11 or 12 bit.
  sensors.begin();
  for (byte i = 0; i < tSensorsNum; i += 1) {
    sensors.setResolution(tSensorAddr[i], 11);
  }

  // Initialize External Alarm input PIN
  pinMode(alarmExternalPin, INPUT);

  busA.strategy.set_pin(pinBusA);
  busA.set_receiver(receiver_function);
  busA.set_acknowledge(true);
  busA.set_crc_32(true);
  busA.set_packet_id(true);
  busA.begin();

  busB.strategy.set_pin(pinBusB);
  busB.set_acknowledge(true);
  busB.set_crc_32(true);
  busB.set_packet_id(true);
  busB.begin();
};