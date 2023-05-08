/*
* Copyright (C) 2023 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#include <Wire.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SLAVE_ADDRESS 0x13

const byte ADDR_VERSION = 255;         // Location of the software version in EEPROM
const byte CURRENT_EEPROM_VERSION = 1; // We are on revision 1 of the EEPROM storage structure (0xFF or 255 is never a valid value for version)

const byte debugMode = 0;
char answerData[33];
byte index = 0;
byte receivingData = 0;

// Fans
const byte fansNum = 4;
// {"command"}
const char *const fanCmd[fansNum] = {"f1", "f2", "f3", "f4"};
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
// {"EEProm fan speed in %"}
const byte eepromFanSpeedPercentAddr[fansNum] = {1, 2, 3, 4};
// {"EEProm fan state"}
const byte eepromFanAutoModeAddr[fansNum] = {5, 6, 7, 8};
// {"EEProm temperature low limit"}
const byte eepromFanTempLowLimitAddr[fansNum] = {9, 10, 11, 12};
// {"EEProm temperature high limit"}
const byte eepromFanTempHighLimitAddr[fansNum] = {13, 14, 15, 16};
// Fans auto-control
unsigned long prevMillisFanAutoControl = 0; 

// Temperature sensors
const byte tSensorsNum = 4;
OneWire oneWire(11);
DallasTemperature sensors(&oneWire);
// {"command"}
const char *const tSensorCmd[tSensorsNum] = {"t1", "t2", "t3", "t4"};
// {"value in auto mode"}
float tSensorAutomodeValue[tSensorsNum] = {0, 0, 0, 0};
const DeviceAddress tSensorAddr[tSensorsNum] = {
  {0x28, 0xA0, 0x17, 0x16, 0xA8, 0x01, 0x3C, 0x54},  // back wall
  {0x28, 0xAC, 0x21, 0x16, 0xA8, 0x01, 0x3C, 0x31},  // left
  {0x28, 0xC1, 0xEB, 0x79, 0xA2, 0x00, 0x03, 0x7C},  // top
  {0x28, 0x27, 0x0B, 0x79, 0xA2, 0x00, 0x03, 0x3D}}; // right
// {"value"}
float temperatureValue[tSensorsNum] = {0.0, 0.0, 0.0, 0.0};
// Temperature auto mode
byte temperatureAutoMode = 0;
// EEProm temperature auto mode
const byte eepromTemperatureAutoModeAddr = 17;
unsigned long lastTemperatureAutoModeTime = 0;

// External alarm status
const byte alarmExternalPin = 10;
byte alarmExternalStatus = 0;
byte alarmExternalStatusFansDisabled = 0;
const byte eepromAlarmExternalStatus = 18;


void read_temperature() {
  sensors.requestTemperatures();
  for (int i = 0; i < tSensorsNum; i += 1) {
    float tempC = sensors.getTempC(tSensorAddr[i]);
    if (tempC == DEVICE_DISCONNECTED_C) {
      tempC = sqrt(-1);
    }
    temperatureValue[i] = tempC;
  }
}

void answer(const char *const data) {
  strcpy(answerData, data);
  strcat(answerData, ";");
}

void sendData() {
  Wire.write(answerData[index]);
  ++index;
  if (index >= strlen(answerData)) {
    index = 0;
  }
  receivingData = 0;
}

void receiveData(int length) {
  receivingData = 1;
  if (debugMode == 1) {
    Serial.println("--- Received data ---");
  }

  char tmpBuf[8]; // for able storing number like "-127.00" + 1 NULL termination
  char tmpBuf2[32] = {0}; // for able to store all sensor values (7*4+4 + 1 NULL termination)
  const char goodCommandReply[] = "ok";
  const char badCommandReply[] = "err";

  // Copy full request to new char array
  char request[length + 1];
  for (byte i = 0; i != length; i++) {
    request[i] = Wire.read();
  }
  request[length] = 0;
  if (debugMode == 1) {
    Serial.print("Length: ");
    Serial.println(length);
    Serial.print("Request: ");
    Serial.println(request);
  }

  const char delimiter[2] = "=";
  byte delimiterAddr = 0;
  byte commandLength = length;
  byte valueLength = 0;

  for (byte i = 0; i != length; i++) {
    if (request[i] == delimiter[0]) {
      delimiterAddr = i;
      commandLength = i;
      valueLength = length - i - 1;
      if (debugMode == 1) {
        Serial.print("Delimiter address: ");
        Serial.println(delimiterAddr);
        Serial.print("Command length: ");
        Serial.println(commandLength);
        Serial.print("Value length: ");
        Serial.println(valueLength);
      }
      break;
    }
  }
  
  char command[commandLength + 1];
  char value[valueLength + 1];
  if (delimiterAddr == 0) {
    if (debugMode == 1) {
      Serial.println("Delimiter not found");
    }
    for (byte i = 0; i != length; i++) {
      command[i] = request[i];
    }
    command[commandLength] = 0;
    value[0] = 0;
  } else {
    if (debugMode == 1) {
      Serial.println("Delimiter found");
    }
    for (byte i = 0; i != delimiterAddr; i++) {
      command[i] = request[i];
    }
    command[commandLength] = 0;
    for (byte i = 0; i != valueLength; i++) {
      value[i] = request[delimiterAddr + 1 + i];
    }
    value[valueLength] = 0;
  }
  if (debugMode == 1) {
    Serial.print("Command: ");
    Serial.println(command);
    Serial.print("Value: ");
    Serial.println(value);
  }

  // Return "err" if defined "delimiter" but no "value"
  if (delimiterAddr != 0 and valueLength == 0) {
    answer(badCommandReply);
    if (debugMode == 1) {
      Serial.println("Error: Delimiter found but no value");
    }
    return;
  }

  // Check if "value" contains only digits and not more than 3 digits
  if (valueLength > 3) {
    answer(badCommandReply);
    if (debugMode == 1) {
      Serial.println("Error: Value length more than 3");
    }
    return;
  }
  for (byte i = 0; i != valueLength; i++) {
    if (! isDigit(value[i])) {
      answer(badCommandReply);
      if (debugMode == 1) {
        Serial.println("Error: Value contains not only digits");
      }
      return;
    }
  }
  const byte value_int = atoi(value);

  // Fans
  if (strcmp(command, "f") == 0) {
    if (valueLength == 0) {
      for (int i = 0; i < fansNum; i += 1) {
        itoa(fanSpeedPercent[i], tmpBuf, 10);
        strcat(tmpBuf2, tmpBuf);
        strcat(tmpBuf2, ",");
      }
      answer(tmpBuf2);
      return;
    } else {
      answer(badCommandReply);
      return;
    }
  }
  if (strcmp(command, "f-ac") == 0) {
    if (valueLength == 0) {
      for (int i = 0; i < fansNum; i += 1) {
        itoa(fanAutoMode[i], tmpBuf, 10);
        strcat(tmpBuf2, tmpBuf);
        strcat(tmpBuf2, ",");
      }
      answer(tmpBuf2);
      return;
    } else {
      answer(badCommandReply);
      return;
    }
  }
  if (strcmp(command, "f-tl") == 0) {
    if (valueLength == 0) {
      for (int i = 0; i < fansNum; i += 1) {
        itoa(fanTempLowLimit[i], tmpBuf, 10);
        strcat(tmpBuf2, tmpBuf);
        strcat(tmpBuf2, ",");
      }
      answer(tmpBuf2);
      return;
    } else {
      answer(badCommandReply);
      return;
    }
  }
  if (strcmp(command, "f-th") == 0) {
    if (valueLength == 0) {
      for (int i = 0; i < fansNum; i += 1) {
        itoa(fanTempHighLimit[i], tmpBuf, 10);
        strcat(tmpBuf2, tmpBuf);
        strcat(tmpBuf2, ",");
      }
      answer(tmpBuf2);
      return;
    } else {
      answer(badCommandReply);
      return;
    }
  }
  for (int i = 0; i < fansNum; i += 1) {
    if (strcmp(command, fanCmd[i]) == 0) {
      if (valueLength == 0) {
        itoa(fanSpeedPercent[i], tmpBuf, 10);
        answer(tmpBuf);
        return;
      } else if (alarmExternalStatus == 1) {
          answer(badCommandReply);
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
          answer(goodCommandReply);
          alarmExternalStatusFansDisabled = 0;
          EEPROM.update(eepromFanSpeedPercentAddr[i], value_int);
          return;
        } else {
          answer(badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, fanCmd[i]);
    strcat(tmpBuf, "-ac");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(fanAutoMode[i], tmpBuf, 10);
        answer(tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1 or value_int == 2) {
          fanAutoMode[i] = value_int;
          tSensorAutomodeValue[i] = 0.0; // reset temperature to force changes
          EEPROM.update(eepromFanAutoModeAddr[i], value_int);
          if (value_int != 0 and temperatureAutoMode == 0) {
            temperatureAutoMode = 60;
            EEPROM.update(eepromTemperatureAutoModeAddr, 60);
          }
          answer(goodCommandReply);
          return;
        } else {
          answer(badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, fanCmd[i]);
    strcat(tmpBuf, "-tl");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(fanTempLowLimit[i], tmpBuf, 10);
        answer(tmpBuf);
        return;
      } else {
        if (value_int >= 20 and value_int <= 25) {
          fanTempLowLimit[i] = value_int;
          tSensorAutomodeValue[i] = 0.0; // reset temperature to force changes
          EEPROM.update(eepromFanTempLowLimitAddr[i], value_int);
          answer(goodCommandReply);
          return;
        } else {
          answer(badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, fanCmd[i]);
    strcat(tmpBuf, "-th");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(fanTempHighLimit[i], tmpBuf, 10);
        answer(tmpBuf);
        return;
      } else {
        if (value_int >= 26 and value_int <= 39) {
          fanTempHighLimit[i] = value_int;
          tSensorAutomodeValue[i] = 0.0; // reset temperature to force changes
          EEPROM.update(eepromFanTempHighLimitAddr[i], value_int);
          answer(goodCommandReply);
          return;
        } else {
          answer(badCommandReply);
          return;
        }
      }
    }
  }
  // Temperature sensors
  if (strcmp(command, "t") == 0) {
    if (valueLength == 0) {
      for (int i = 0; i < tSensorsNum; i += 1) {
        if (temperatureAutoMode > 0) {
          dtostrf(temperatureValue[i], 0, 2, tmpBuf);
        } else {
          dtostrf(sqrt(-1), 0, 2, tmpBuf);
        }
        strcat(tmpBuf2, tmpBuf);
        strcat(tmpBuf2, ",");
      }
      answer(tmpBuf2);
      return;
    } else {
      answer(badCommandReply);
      return;
    }
  }
  if (strcmp(command, "ta") == 0) {
    if (valueLength == 0) {
      itoa(temperatureAutoMode, tmpBuf, 10);
      answer(tmpBuf);
      return;
    } else {
      if (value_int == 0 || (value_int >= 10 && value_int <= 120)) {
        temperatureAutoMode = value_int;
        EEPROM.update(eepromTemperatureAutoModeAddr, value_int);
        answer(goodCommandReply);
        return;
      } else {
        answer(badCommandReply);
        return;
      }
    }
  }
  for (byte i = 0; i < tSensorsNum; i += 1) {
    if (strcmp(command, tSensorCmd[i]) == 0) {
      if (valueLength == 0) {
        if (temperatureAutoMode > 0) {
          dtostrf(temperatureValue[i], 0, 2, tmpBuf);
        } else {
          dtostrf(sqrt(-1), 0, 2, tmpBuf);
        }
        answer(tmpBuf);
        return;
      } else {
        answer(badCommandReply);
        return;
      }
    }
  }
  // External alarm status
  if (strcmp(command, "a") == 0) {
    if (valueLength == 0) {
      itoa(alarmExternalStatus, tmpBuf, 10);
      answer(tmpBuf);
      return;
    } else {
      if (value_int == 0 or value_int == 1) {
        alarmExternalStatus = value_int;
        alarmExternalStatusFansDisabled = 0;
        EEPROM.update(eepromAlarmExternalStatus, value_int);
        answer(goodCommandReply);
        return;
      } else {
        answer(badCommandReply);
        return;
      }
    }
  }
  answer(badCommandReply);
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
  }
}

void fan_auto_control() {
  for (byte i = 0; i < fansNum; i += 1) {
    if (fanAutoMode[i] == 1 or fanAutoMode[i] == 2) {
      float tempC = temperatureValue[i];
      if (tempC != tSensorAutomodeValue[i]) {
        if (tempC < fanTempLowLimit[i]) { // temp low limit
          if (fanSpeedPercent[i] != 0) {
            fanSpeedPercent[i] = 0; // fan speed in %
            digitalWrite(fanPin[i], LOW);
          }
        } else if (tempC > fanTempHighLimit[i]) { // temp high limit
          if (fanSpeedPercent[i] != 100) {
            fanSpeedPercent[i] = 100; // fan speed in %
            digitalWrite(fanPin[i], HIGH);
          }
        } else if (fanAutoMode[i] == 2 and tempC >= fanTempLowLimit[i] and tempC <= fanTempHighLimit[i]) {
          byte fanSpeed = map(tempC, fanTempLowLimit[i], fanTempHighLimit[i], 90, 200); // define fan speed in analog value
          byte fanSpeedPercentCurrent = map(fanSpeed, 90, 200, 1, 99); // fan speed in %
          if (fanSpeedPercent[i] != fanSpeedPercentCurrent) {
            fanSpeedPercent[i] = map(fanSpeed, 90, 200, 1, 99); // fan speed in %
            analogWrite(fanPin[i], fanSpeed); // spin the fan at the fanSpeed speed
          }
        }
      }
      tSensorAutomodeValue[i] = tempC;
    }
  }
}

void loop() {
  external_alarm();

  // run each 60 seconds
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillisFanAutoControl >= 60000) {
    fan_auto_control();
    prevMillisFanAutoControl = curMillis;
  }

  if (temperatureAutoMode > 0) {
    unsigned long curMillis = millis(); // time now in ms
    if (curMillis - lastTemperatureAutoModeTime > (unsigned long) temperatureAutoMode * 1000) {
      if (receivingData == 0) {
        read_temperature();
        if (debugMode == 1) {
          for (int i = 0; i < tSensorsNum; i += 1) {
            Serial.print(tSensorCmd[i]);
            Serial.print(": ");
            Serial.println(temperatureValue[i]);
          }
        }
        lastTemperatureAutoModeTime = curMillis;
      }
    }
  }
}

void setup() {
  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) { //EEprom is wrong version or was not programmed, write default values to the EEprom
    for (byte i = 0; i < fansNum; i += 1) {
      EEPROM.update(eepromFanSpeedPercentAddr[i], fanSpeedPercent[i]);
      EEPROM.update(eepromFanAutoModeAddr[i], fanAutoMode[i]);
      EEPROM.update(eepromFanTempLowLimitAddr[i], fanTempLowLimit[i]);
      EEPROM.update(eepromFanTempHighLimitAddr[i], fanTempHighLimit[i]);
    }
    EEPROM.update(eepromTemperatureAutoModeAddr, temperatureAutoMode);
    EEPROM.update(eepromAlarmExternalStatus, alarmExternalStatus);
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    for (byte i = 0; i < fansNum; i += 1) {
      fanSpeedPercent[i] = EEPROM.read(eepromFanSpeedPercentAddr[i]);
      fanAutoMode[i] = EEPROM.read(eepromFanAutoModeAddr[i]);
      fanTempLowLimit[i] = EEPROM.read(eepromFanTempLowLimitAddr[i]);
      fanTempHighLimit[i] = EEPROM.read(eepromFanTempHighLimitAddr[i]);
    }
    temperatureAutoMode = EEPROM.read(eepromTemperatureAutoModeAddr);
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
  if (temperatureAutoMode > 0) {
    read_temperature();
  }

  // Initialize External Alarm input PIN
  pinMode(alarmExternalPin, INPUT);

  // Join I2C bus as slave
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  // Start Serial (for debug)
  if (debugMode == 1) {
    Serial.begin(115200);
  }
}
