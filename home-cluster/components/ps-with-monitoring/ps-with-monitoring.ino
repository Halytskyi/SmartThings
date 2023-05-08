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
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>

#define SLAVE_ADDRESS 0x14

const byte ADDR_VERSION = 255;         // Location of the software version in EEPROM
const byte CURRENT_EEPROM_VERSION = 1; // We are on revision 1 of the EEPROM storage structure (0xFF or 255 is never a valid value for version)

const byte debugMode = 0;
char answerData[38];
byte index = 0;
byte receivingData = 0;

// Solar or DC-DC source
byte solarDcSrc = 0;
// EEProm solar or DC-DC source
const byte eepromSolarDcSrcAddr = 1;

// Voltage params
const float arduinoVoltage = 4.92;

// Voltage sensors
const byte voltageNum = 4;
// {"command"}
const char *const voltageCmd[voltageNum] = {"v1", "v2", "v3", "v4"};

// Current sensors
const byte currentNum = 4;
// {"command"}
const char *const currentCmd[currentNum] = {"i1", "i2", "i3", "i4"};

// Power consumption
const byte powerNum = 4;
// {"command"}
const char *const powerCmd[powerNum] = {"p1", "p2", "p3", "p4"};

// Temperature sensors
const byte tSensorsNum = 4;
OneWire oneWire(10);
DallasTemperature sensors(&oneWire);
// {"command"}
const char *const tSensorCmd[tSensorsNum] = {"t1", "t2", "t3", "t4"};
const DeviceAddress tSensorAddr[tSensorsNum] = {
  {0x28, 0x4E, 0xD9, 0x5E, 0x39, 0x19, 0x01, 0x48}, // 24V 15A PS (t1)
  {0x28, 0xEF, 0xC7, 0x11, 0x39, 0x19, 0x01, 0x87}, // 18V 20A PS (t2)
  {0x28, 0x70, 0x3B, 0x1B, 0x39, 0x19, 0x01, 0x8E}, // 12V DC-DC2, from 24V 10A PS(t3)
  {0x28, 0xDE, 0xF9, 0x5D, 0x39, 0x19, 0x01, 0x89}}; // 12V DC-DC1, from 18V 20A PS (t4)
// {"value"}
float temperatureValue[tSensorsNum] = {0.0, 0.0, 0.0, 0.0};
// Temperature auto mode
byte temperatureAutoMode = 0;
// EEProm temperature auto mode
const byte eepromTemperatureAutoModeAddr = 2;
unsigned long lastTemperatureAutoModeTime = 0;

// PZEM004T v3.0
SoftwareSerial pzemSWSerial(2, 3);
PZEM004Tv30 pzem;
const byte acLineParamsNum = 6;
// {"command"}
const char *const acLineParamCmd[acLineParamsNum] = {"lv", "lc", "lp", "le", "lf", "lpf"};
// {"value"}
float acLineParamValue[acLineParamsNum] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
// Line auto mode
byte lineAutoMode = 0;
// EEProm line auto mode
const byte eepromLineAutoModeAddr = 3;
unsigned long lastLineAutoModeTime = 0; 

float get_voltage(const char command[]) {
  if (strcmp(command, "v3") == 0 && solarDcSrc == 1) {
    return sqrt(-1);
  } else if (strcmp(command, "v4") == 0 && solarDcSrc == 0) {
    return sqrt(-1);
  }

  float voltageValue = 0.0;
  byte voltagePin = 0;
  float r1 = 0.0;
  float r2 = 0.0;
  const byte voltageCountValues = 50; // how many values must be averaged

  if (strcmp(command, "v1") == 0) {
    voltagePin = 0; // 24V 15A PS output
    r1 = 101300; // R1 = 100k
    r2 = 9700; // R2 = 10k
  } else if (strcmp(command, "v2") == 0) {
    voltagePin = 3; // 18V 20A PS output
    r1 = 99650; // R1 = 100k
    r2 = 9700; // R2 = 10k
  } else if (strcmp(command, "v3") == 0) {
    voltagePin = 6; // 24V Solar Battaries output
    r1 = 100000; // R1 = 100k
    r2 = 9700; // R2 = 10k
  } else if (strcmp(command, "v4") == 0) {
    voltagePin = 6; // 12V DC-DC output
    r1 = 100000; // R1 = 100k
    r2 = 9610; // R2 = 10k
  }

  for (byte i = 0; i < voltageCountValues; i++) {
    voltageValue += analogRead(voltagePin);
    delay(3);
  }
  voltageValue = voltageValue / voltageCountValues;
  float v  = (voltageValue * arduinoVoltage) / 1024.0;
  voltageValue = v / (r2 / (r1 + r2));
  if (voltageValue < 0.99) voltageValue = 0.0;

  return voltageValue;
}

float get_current(const char command[]) {
  if (strcmp(command, "i3") == 0 && solarDcSrc == 1) {
    return sqrt(-1);
  } else if (strcmp(command, "i4") == 0 && solarDcSrc == 0) {
    return sqrt(-1);
  }

  float currentValue = 0.0;
  byte currentPin = 0;
  byte direction = 0; // 1 - direct, 0 - revert
  float offsetVoltage = 0.0;
  const float sensitivity = 0.1; // 5A: 0.185; 20A: 0.1; 30A: 0.066
  const byte currentCountValues = 50; // how many values must be averaged

  if (strcmp(command, "i1") == 0) {
    currentPin = 1; // 24V 15A PS output
    offsetVoltage = 2.461;
    direction = 1;
  } else if (strcmp(command, "i2") == 0) {
    currentPin = 2; // 18V 20A PS output
    offsetVoltage = 2.443;
    direction = 0;
  } else if (strcmp(command, "i3") == 0) {
    currentPin = 7; // 24V Solar Battaries output
    offsetVoltage = 2.465;
    direction = 1;
  } else if (strcmp(command, "i4") == 0) {
    currentPin = 7; // 12V DC-DC output
    offsetVoltage = 2.450;
    direction = 0;
  }

  for (byte i = 0; i < currentCountValues; i++) {
    // read the value from the sensor:
    currentValue += analogRead(currentPin);
    delay(3);
  }
  // make average value
  currentValue = currentValue / currentCountValues;

  float voltage = (currentValue / 1024.0) * arduinoVoltage; // Gets mV
  if (direction == 1) {
    currentValue = ((voltage - offsetVoltage) / sensitivity);
  } else {
    currentValue = ((offsetVoltage - voltage) / sensitivity);
  }
  if (currentValue < 0.04) currentValue = 0.0;

  // Uncomment to get "offsetVoltage" for sensor AND CHANGE "precision" to "3" in "dtostrf"
  //currentValue = voltage;

  return currentValue;
}

float get_power_consumption(const char command[]) {
  if (strcmp(command, "p3") == 0 && solarDcSrc == 1) {
    return sqrt(-1);
  } else if (strcmp(command, "p4") == 0 && solarDcSrc == 0) {
    return sqrt(-1);
  }

  float powerConsumptionValue = 0.0;
  for (byte i = 0; i < powerNum; i += 1) {
    if (strcmp(command, powerCmd[i]) == 0) {
      powerConsumptionValue = get_voltage(voltageCmd[i]) * get_current(currentCmd[i]);
      break;
    }
  }
  return powerConsumptionValue;
}

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

void read_ac_line_params() {
  acLineParamValue[0] = pzem.voltage();
  acLineParamValue[1] = pzem.current();
  acLineParamValue[2] = pzem.power();
  acLineParamValue[3] = pzem.energy();
  acLineParamValue[4] = pzem.frequency();
  acLineParamValue[5] = pzem.pf();
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
  char tmpBuf2[37] = {0}; // for able to store all sensor values (5*6+6 + 1 NULL termination)
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

  // Source: Solar Battaries or DC-DC converter
  if (strcmp(command, "sd") == 0) {
    if (valueLength == 0) {
      itoa(solarDcSrc, tmpBuf, 10);
      answer(tmpBuf);
      return;
    } else {
      if (value_int == 0 or value_int == 1) {
        solarDcSrc = value_int;
        EEPROM.update(eepromSolarDcSrcAddr, value_int);
        answer(goodCommandReply);
        return;
      } else {
        answer(badCommandReply);
        return;
      }
    }
  }
  // Voltage sensors
  if (strcmp(command, "v") == 0) {
    if (valueLength == 0) {
      for (int i = 0; i < voltageNum; i += 1) {
        dtostrf(get_voltage(voltageCmd[i]), 0, 2, tmpBuf);
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
  for (byte i = 0; i < voltageNum; i += 1) {
    if (strcmp(command, voltageCmd[i]) == 0) {
      if (valueLength == 0) {
        dtostrf(get_voltage(voltageCmd[i]), 0, 2, tmpBuf);
        answer(tmpBuf);
        return;
      } else {
        answer(badCommandReply);
        return;
      }
    }
  }
  // Current sensors
  if (strcmp(command, "i") == 0) {
    if (valueLength == 0) {
      for (int i = 0; i < currentNum; i += 1) {
        dtostrf(get_current(currentCmd[i]), 0, 2, tmpBuf);
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
  for (byte i = 0; i < currentNum; i += 1) {
    if (strcmp(command, currentCmd[i]) == 0) {
      if (valueLength == 0) {
        dtostrf(get_current(currentCmd[i]), 0, 2, tmpBuf);
        answer(tmpBuf);
        return;
      } else {
        answer(badCommandReply);
        return;
      }
    }
  }
  // Power consumption
  if (strcmp(command, "p") == 0) {
    if (valueLength == 0) {
      for (int i = 0; i < powerNum; i += 1) {
        dtostrf(get_power_consumption(powerCmd[i]), 0, 2, tmpBuf);
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
  for (byte i = 0; i < powerNum; i += 1) {
    if (strcmp(command, powerCmd[i]) == 0) {
      if (valueLength == 0) {
        dtostrf(get_power_consumption(powerCmd[i]), 0, 2, tmpBuf);
        answer(tmpBuf);
        return;
      } else {
        answer(badCommandReply);
        return;
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
  // AC line params
  if (strcmp(command, "l") == 0) {
    if (valueLength == 0) {
      for (int i = 0; i < acLineParamsNum; i += 1) {
        if (lineAutoMode > 0) {
          dtostrf(acLineParamValue[i], 0, 2, tmpBuf);
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
  if (strcmp(command, "la") == 0) {
    if (valueLength == 0) {
      itoa(lineAutoMode, tmpBuf, 10);
      answer(tmpBuf);
      return;
    } else {
      if (value_int == 0 || (value_int >= 10 && value_int <= 120)) {
        lineAutoMode = value_int;
        EEPROM.update(eepromLineAutoModeAddr, value_int);
        answer(goodCommandReply);
        return;
      } else {
        answer(badCommandReply);
        return;
      }
    }
  }
  for (byte i = 0; i < acLineParamsNum; i += 1) {
    if (strcmp(command, acLineParamCmd[i]) == 0) {
      if (valueLength == 0) {
        if (lineAutoMode > 0) {
          dtostrf(acLineParamValue[i], 0, 2, tmpBuf);
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
  answer(badCommandReply);
}

void loop() {
  if (lineAutoMode > 0) {
    unsigned long curMillis = millis(); // time now in ms
    if (curMillis - lastLineAutoModeTime > (unsigned long) lineAutoMode * 1000) {
      if (receivingData == 0) {
        read_ac_line_params();
        if (debugMode == 1) {
          for (int i = 0; i < acLineParamsNum; i += 1) {
            Serial.print(acLineParamCmd[i]);
            Serial.print(": ");
            Serial.println(acLineParamValue[i]);
          }
        }
        lastLineAutoModeTime = curMillis;
      }
    }
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
    EEPROM.update(eepromSolarDcSrcAddr, solarDcSrc);
    EEPROM.update(eepromTemperatureAutoModeAddr, temperatureAutoMode);
    EEPROM.update(eepromLineAutoModeAddr, lineAutoMode);
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    solarDcSrc = EEPROM.read(eepromSolarDcSrcAddr);
    temperatureAutoMode = EEPROM.read(eepromTemperatureAutoModeAddr);
    lineAutoMode = EEPROM.read(eepromLineAutoModeAddr);
  }

  // Set temperature sensor resolution, valid values are 9, 10, 11 or 12 bit.
  sensors.begin();
  for (byte i = 0; i < tSensorsNum; i += 1) {
    sensors.setResolution(tSensorAddr[i], 11);
  }
  if (temperatureAutoMode > 0) {
    read_temperature();
  }

  pzem = PZEM004Tv30(pzemSWSerial);
  if (lineAutoMode > 0) {
    read_ac_line_params();
  }

  // Join I2C bus as slave
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  // Start Serial (for debug)
  if (debugMode == 1) {
    Serial.begin(115200);
  }
};