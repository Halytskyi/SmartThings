/*
* Copyright (C) 2020 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#define PJON_MAX_PACKETS 2
#define PJON_INCLUDE_PACKET_ID
#include <PJONSoftwareBitBang.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const byte deviceID = 16;
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

// For Auto charge mode
const unsigned int chargerAutomodeInterval = 60000; // interval for autocharge mode
unsigned long prevMillisChargerAutomode = 4294952295; // after start delay 45s (4294967295 - 15000 = 4294952295)
const unsigned long chargerAutomodePSInterval = 3600000; // 1h interval for enable PS charger in autocharge mode after fail
unsigned long prevMillisChargerAutomodePS = chargerAutomodePSInterval; // to start without waiting 1h

// Charger
const byte chargerTypePin = 5;
const byte chargerPSPin = 4;
byte chargerAutomode = 1;
const byte eepromChargerAutomodeAddr = 1;
byte chargerAutomodeMessages = 0;
const byte eepromChargerAutomodeMessagesAddr = 2;

// Batteries switches
const byte batSwitchesNum = 2;
// {"command"}
const char *const batSwitchCmd[batSwitchesNum] = {"B-1", "B-2"};
// {"pin"}
const byte batSwitchPin[batSwitchesNum] = {2, 3};

// Outputs
const byte outputsNum = 4;
// {"command"}
const char *const outputCmd[outputsNum] = {"O-1", "O-2", "O-3", "O-4"};
// {"pin"}
const byte outputPin[outputsNum] = {13, 11, 10, 9};
// Outputs control automode
byte outputsAutomode = 1;
const byte eepromOutputsAutomodeAddr = 3;
byte outputsAutomodeMessages = 0;
const byte eepromOutputsAutomodeMessagesAddr = 4;
const unsigned int outputsAutomodeInterval = 60000; // interval for outputs autocharge mode
unsigned long prevMillisOutputsAutomode = 4294937295; // after start delay 30s (4294967295 - 30000 = 4294937295)

// Voltage params
const float arduinoVoltage = 5.014;

// Voltage sensors
const byte voltageNum = 4;
// {"command"}
const char *const voltageCmd[voltageNum] = {"V-1", "V-2", "V-3", "V-4"};
// {"auto-push"}
byte voltageAutoPush[voltageNum] = {0, 0, 0, 0};
// {"auto-push last update"}
unsigned long voltageAutoPushLU[voltageNum] = {0, 0, 0, 0};
// {"EEProm auto-push"}
const byte eepromVoltageAutoPushAddr[voltageNum] = {5, 6, 7, 8};

// Current sensors
const byte currentNum = 4;
// {"command"}
const char *const currentCmd[currentNum] = {"I-1", "I-2", "I-3", "I-4"};
// {"auto-push"}
byte currentAutoPush[currentNum] = {0, 0, 0, 0};
// {"auto-push last update"}
unsigned long currentAutoPushLU[currentNum] = {0, 0, 0, 0};
// {"EEProm auto-push"}
const byte eepromCurrentAutoPushAddr[currentNum] = {9, 10, 11, 12};

// Power consumption
const byte powerNum = 4;
// {"command"}
const char *const powerCmd[powerNum] = {"P-1", "P-2", "P-3", "P-4"};
// {"auto-push"}
byte powerAutoPush[powerNum] = {0, 0, 0, 0};
// {"auto-push last update"}
unsigned long powerAutoPushLU[powerNum] = {0, 0, 0, 0};
// {"EEProm auto-push"}
const byte eepromPowerAutoPushAddr[powerNum] = {13, 14, 15, 16};

// Temperature sensors
const byte tSensorsNum = 3;
OneWire oneWire(6);
DallasTemperature sensors(&oneWire);
// {"command"}
const char *const tSensorCmd[tSensorsNum] = {"T-1", "T-2", "T-3"};
// {"auto-push"}
byte tSensorAutoPush[tSensorsNum] = {0, 0, 0};
// {"auto-push last update"}
unsigned long tSensorAutoPushLU[tSensorsNum] = {0, 0, 0};
// {"EEProm auto-push"}
const byte eepromTSensorAutoPushAddr[tSensorsNum] = {17, 18, 19};
const DeviceAddress tSensorAddr[tSensorsNum] = {
  {0x28, 0x7E, 0xBB, 0xC6, 0x38, 0x19, 0x01, 0x61}, // Battery 1
  {0x28, 0x3C, 0x7C, 0x5E, 0x39, 0x19, 0x01, 0x5F}, // Battery 2
  {0x28, 0x68, 0x90, 0x64, 0x39, 0x19, 0x01, 0x05}}; // PS charger

// Temperature control
// {"command"}
const char *const temperatureControlCmd[tSensorsNum] = {"T-c-1", "T-c-2", "T-c-3"};
// {"value"}
byte temperatureControl[tSensorsNum] = {1, 1, 1};
const byte eepromTemperatureControlAddr[tSensorsNum] = {20, 21, 22};


/* --- for debug only START --- */
void error_handlerA(uint8_t code, uint16_t data, void *custom_pointer) {
  if(code == PJON_CONNECTION_LOST) {
    Serial.print("A: Connection with device ID ");
    Serial.print(busA.packets[data].content[0], DEC);
    Serial.println(" is lost.");
  }
  if(code == PJON_PACKETS_BUFFER_FULL) {
    Serial.print("A: Packet buffer is full, has now a length of ");
    Serial.println(data, DEC);
    Serial.println("Possible wrong bus configuration!");
    Serial.println("higher PJON_MAX_PACKETS if necessary.");
  }
  if(code == PJON_CONTENT_TOO_LONG) {
    Serial.print("A: Content is too long, length: ");
    Serial.println(data);
  }
};

void error_handlerB(uint8_t code, uint16_t data, void *custom_pointer) {
  if(code == PJON_CONNECTION_LOST) {
    Serial.print("B: Connection with device ID ");
    Serial.print(busB.packets[data].content[0], DEC);
    Serial.println(" is lost.");
  }
  if(code == PJON_PACKETS_BUFFER_FULL) {
    Serial.print("B: Packet buffer is full, has now a length of ");
    Serial.println(data, DEC);
    Serial.println("Possible wrong bus configuration!");
    Serial.println("higher PJON_MAX_PACKETS if necessary.");
  }
  if(code == PJON_CONTENT_TOO_LONG) {
    Serial.print("B: Content is too long, length: ");
    Serial.println(data);
  }
};
/* --- for debug only END --- */

float get_voltage(const char command[]) {
  float voltageValue = 0.0;
  byte voltagePin = 0;
  float r1 = 0.0;
  float r2 = 0.0;
  const byte voltageCountValues = 50; // how many values must be averaged

  if (strcmp(command, "V-1") == 0) {
    voltagePin = 3; // Power supply charger
    r1 = 101000; // R1 = 100k
    r2 = 9800; // R2 = 10k
  } else if (strcmp(command, "V-2") == 0) {
    voltagePin = 1; // Solar charger
    r1 = 101500; // R1 = 100k
    r2 = 9830; // R2 = 10k
  } else if (strcmp(command, "V-3") == 0) {
    voltagePin = 7; // Battery 1
    r1 = 101500; // R1 = 100k
    r2 = 9860; // R2 = 10k
  } else if (strcmp(command, "V-4") == 0) {
    voltagePin = 5; // Battery 2
    r1 = 99510; // R1 = 100k
    r2 = 9900; // R2 = 10k
  }

  for (byte i = 0; i < voltageCountValues; i++) {
    voltageValue += analogRead(voltagePin);
    delay(3);
  }
  voltageValue = voltageValue / voltageCountValues;
  float v  = (voltageValue * arduinoVoltage) / 1024.0;
  voltageValue = v / (r2 / (r1 + r2));
  if (voltageValue < 0.1) voltageValue = 0.0;

  return voltageValue;
}

float get_current(const char command[]) {
  float currentValue = 0.0;
  byte currentPin = 0;
  byte direction = 0; // 1 - direct, 0 - revert
  float offsetVoltage = 0.0;
  const float sensitivity = 0.1; // 5A: 0.185; 20A: 0.1; 30A: 0.066
  const byte currentCountValues = 50; // how many values must be averaged

  if (strcmp(command, "I-1") == 0) {
    currentPin = 2; // Power supply charger
    offsetVoltage = 2.480;
    direction = 0;
  } else if (strcmp(command, "I-2") == 0) {
    currentPin = 0; // Solar charger
    offsetVoltage = 2.503;
    direction = 1;
  } else if (strcmp(command, "I-3") == 0) {
    currentPin = 6; // Battery 1
    offsetVoltage = 2.501;
    direction = 1;
  } else if (strcmp(command, "I-4") == 0) {
    currentPin = 4; // Battery 2
    offsetVoltage = 2.499;
    direction = 1;
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
  float powerConsumptionValue = 0.0;
  for (byte i = 0; i < powerNum; i += 1) {
    if (strcmp(command, powerCmd[i]) == 0) {
      powerConsumptionValue = get_voltage(voltageCmd[i]) * get_current(currentCmd[i]);
      break;
    }
  }
  return powerConsumptionValue;
}

float get_temperature(const byte i) {
  sensors.requestTemperaturesByAddress(tSensorAddr[i]);
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
  uint16_t result = busA.reply_blocking(responseFull, strlen(responseFull));
  Serial.print(F("Reply result: "));
  Serial.println(result);
}

void busB_send(const char command[], const char response[]) {
  char responseFull[22]; // 21 charters + 1 NULL termination
  strcpy(responseFull, command);
  strcat(responseFull, "<");
  strcat(responseFull, response);
  uint16_t result = busB.send_packet_blocking(masterIdBusB, responseFull, strlen(responseFull));
  Serial.println(responseFull);
  Serial.print(F("Send result: "));
  Serial.println(result);
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

  // Chargers type/state
  if (strcmp(command, "C") == 0) {
    if (valueLength == 0) {
      itoa(digitalRead(chargerTypePin), tmpBuf, 10);
      busA_reply(request, tmpBuf);
      return;
    } else {
      if (value_int == 0 or value_int == 1) {
        digitalWrite(chargerTypePin, value_int);
        busA_reply(request, goodCommandReply);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
  } else if (strcmp(command, "C-p") == 0) {
    if (valueLength == 0) {
      itoa(digitalRead(chargerPSPin), tmpBuf, 10);
      busA_reply(request, tmpBuf);
      return;
    } else {
      if (value_int == 0 or value_int == 1) {
        digitalWrite(chargerPSPin, value_int);
        busA_reply(request, goodCommandReply);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
  } else if (strcmp(command, "C-a") == 0) {
    if (valueLength == 0) {
      itoa(chargerAutomode, tmpBuf, 10);
      busA_reply(request, tmpBuf);
      return;
    } else {
      if (value_int == 0 or value_int == 1) {
        chargerAutomode = value_int;
        EEPROM.update(eepromChargerAutomodeAddr, value_int);
        busA_reply(request, goodCommandReply);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
  } else if (strcmp(command, "C-a-m") == 0) {
    if (valueLength == 0) {
      itoa(chargerAutomodeMessages, tmpBuf, 10);
      busA_reply(request, tmpBuf);
      return;
    } else {
      if (value_int == 0 or value_int == 1 or value_int == 2) {
        chargerAutomodeMessages = value_int;
        EEPROM.update(eepromChargerAutomodeMessagesAddr, value_int);
        busA_reply(request, goodCommandReply);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
  }
  // Batteries switches
  for (byte i = 0; i < batSwitchesNum; i += 1) {
    if (strcmp(command, batSwitchCmd[i]) == 0) {
      if (valueLength == 0) {
        itoa(digitalRead(batSwitchPin[i]), tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1) {
          digitalWrite(batSwitchPin[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  // Outputs
  for (byte i = 0; i < outputsNum; i += 1) {
    if (strcmp(command, outputCmd[i]) == 0) {
      if (valueLength == 0) {
        itoa(digitalRead(outputPin[i]), tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1) {
          digitalWrite(outputPin[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  // Outputs control automode
  if (strcmp(command, "O-a") == 0) {
    if (valueLength == 0) {
      itoa(outputsAutomode, tmpBuf, 10);
      busA_reply(request, tmpBuf);
      return;
    } else {
      if (value_int == 0 or value_int == 1) {
        outputsAutomode = value_int;
        EEPROM.update(eepromOutputsAutomodeAddr, value_int);
        busA_reply(request, goodCommandReply);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
  } else if (strcmp(command, "O-a-m") == 0) {
    if (valueLength == 0) {
      itoa(outputsAutomodeMessages, tmpBuf, 10);
      busA_reply(request, tmpBuf);
      return;
    } else {
      if (value_int == 0 or value_int == 1 or value_int == 2) {
        outputsAutomodeMessages = value_int;
        EEPROM.update(eepromOutputsAutomodeMessagesAddr, value_int);
        busA_reply(request, goodCommandReply);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
  }
  // Temperature control
  for (byte i = 0; i < tSensorsNum; i += 1) {
    if (strcmp(command, temperatureControlCmd[i]) == 0) {
      if (valueLength == 0) {
        itoa(digitalRead(temperatureControl[i]), tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1) {
          temperatureControl[i] = value_int;
          EEPROM.update(temperatureControl[i], value_int);
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
  // Voltage sensors
  for (byte i = 0; i < voltageNum; i += 1) {
    if (strcmp(command, voltageCmd[i]) == 0) {
      if (valueLength == 0) {
        dtostrf(get_voltage(voltageCmd[i]), 0, 2, tmpBuf);
        busA_reply(request, tmpBuf);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
    strcpy(tmpBuf, voltageCmd[i]);
    strcat(tmpBuf, "-a");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(voltageAutoPush[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1) {
          voltageAutoPush[i] = value_int;
          EEPROM.update(eepromVoltageAutoPushAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  // Current sensors
  for (byte i = 0; i < currentNum; i += 1) {
    if (strcmp(command, currentCmd[i]) == 0) {
      if (valueLength == 0) {
        dtostrf(get_current(currentCmd[i]), 0, 2, tmpBuf);
        busA_reply(request, tmpBuf);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
    strcpy(tmpBuf, currentCmd[i]);
    strcat(tmpBuf, "-a");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(currentAutoPush[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1) {
          currentAutoPush[i] = value_int;
          EEPROM.update(eepromCurrentAutoPushAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  // Power consumption
  for (byte i = 0; i < powerNum; i += 1) {
    if (strcmp(command, powerCmd[i]) == 0) {
      if (valueLength == 0) {
        dtostrf(get_power_consumption(powerCmd[i]), 0, 2, tmpBuf);
        busA_reply(request, tmpBuf);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
    strcpy(tmpBuf, powerCmd[i]);
    strcat(tmpBuf, "-a");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(powerAutoPush[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1) {
          powerAutoPush[i] = value_int;
          EEPROM.update(eepromPowerAutoPushAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  busA_reply(request, badCommandReply);
}

void autopush() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillisAutoPush >= minAutoPushInterval) {
    byte msgPushed = 0;
    for (byte i = 0; i < voltageNum; i += 1) {
      if (voltageAutoPush[i] == 1) {
        if (curMillis - voltageAutoPushLU[i] >= autoPushInterval and curMillis - lastUpdateMillisAutoPush >= minAutoPushInterval) {
          char tmpBuf[6];
          dtostrf(get_voltage(voltageCmd[i]), 0, 2, tmpBuf);
          busB_send(voltageCmd[i], tmpBuf);
          voltageAutoPushLU[i] = curMillis;
          lastUpdateMillisAutoPush = curMillis;
          msgPushed = 1;
        }
      }
    }
    if (msgPushed == 0) {
      for (byte i = 0; i < currentNum; i += 1) {
        if (currentAutoPush[i] == 1) {
          if (curMillis - currentAutoPushLU[i] >= autoPushInterval and curMillis - lastUpdateMillisAutoPush >= minAutoPushInterval) {
            char tmpBuf[6];
            dtostrf(get_current(currentCmd[i]), 0, 2, tmpBuf);
            busB_send(currentCmd[i], tmpBuf);
            currentAutoPushLU[i] = curMillis;
            lastUpdateMillisAutoPush = curMillis;
            msgPushed = 1;
          }
        }
      }
    }
    if (msgPushed == 0) {
      for (byte i = 0; i < powerNum; i += 1) {
        if (powerAutoPush[i] == 1) {
          if (curMillis - powerAutoPushLU[i] >= autoPushInterval and curMillis - lastUpdateMillisAutoPush >= minAutoPushInterval) {
            char tmpBuf[7];
            dtostrf(get_power_consumption(powerCmd[i]), 0, 2, tmpBuf);
            busB_send(powerCmd[i], tmpBuf);
            powerAutoPushLU[i] = curMillis;
            lastUpdateMillisAutoPush = curMillis;
            msgPushed = 1;
          }
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
    prevMillisAutoPush = curMillis;
  }
}

void non_block_delay(const unsigned int delayMs) {
  const unsigned long curMillis = millis(); // time now in ms
  unsigned long prevMillis = millis();
  while (prevMillis - curMillis <= delayMs) {
    busA.receive(receiveTimeBusA);
    autopush();
    prevMillis = millis();
  }
}

void autocharge_start() {
  if (chargerAutomode == 1 and digitalRead(chargerTypePin) == 0 and digitalRead(chargerPSPin) == 0) {
    unsigned long curMillis = millis(); // time now in ms
    if (curMillis - prevMillisChargerAutomode >= chargerAutomodeInterval) {
      for (byte i = 0; i < batSwitchesNum; i += 1) {
        float batCurrent = get_current(currentCmd[i+2]);
        if (batCurrent > 0.1) {
          if (chargerAutomodeMessages >= 1) {
            char tmpBuf[14];
            strcpy(tmpBuf, batSwitchCmd[i]);
            strcat(tmpBuf, ":");
            strcat(tmpBuf, currentCmd[i+2]);
            strcat(tmpBuf, "=");
            char tmpBuf2[6];
            dtostrf(batCurrent, 0, 2, tmpBuf2);
            strcat(tmpBuf, tmpBuf2);
            busB_send("C-a-m", tmpBuf);
          }
          continue;
        }

        float batVoltage = get_voltage(voltageCmd[i+2]);
        if (batVoltage > 5 and batVoltage < 12.8) {
          if (temperatureControl[i] == 1) {
            float batTemperature = get_temperature(i);
            if (batTemperature > 42) {
              if (chargerAutomodeMessages >= 1) {
                char tmpBuf[16];
                strcpy(tmpBuf, batSwitchCmd[i]);
                strcat(tmpBuf, ":");
                strcat(tmpBuf, tSensorCmd[i]);
                strcat(tmpBuf, "=");
                char tmpBuf2[8];
                dtostrf(batTemperature, 0, 2, tmpBuf2);
                strcat(tmpBuf, tmpBuf2);
                busB_send("C-a-m", tmpBuf);
              }
              continue;
            }
          }
          if (temperatureControl[2] == 1) {
            float chargerTemperature = get_temperature(2);
            if (chargerTemperature > 50) {
              if (chargerAutomodeMessages >= 1) {
                char tmpBuf[16];
                strcpy(tmpBuf, batSwitchCmd[i]);
                strcat(tmpBuf, ":");
                strcat(tmpBuf, tSensorCmd[2]);
                strcat(tmpBuf, "=");
                char tmpBuf2[8];
                dtostrf(chargerTemperature, 0, 2, tmpBuf2);
                strcat(tmpBuf, tmpBuf2);
                busB_send("C-a-m", tmpBuf);
              }
              continue;
            }
          }
          float solarVoltage = get_voltage(voltageCmd[1]);
          if (solarVoltage >= 13) {
            digitalWrite(chargerTypePin, HIGH);
            if (chargerAutomodeMessages == 2) {
              busB_send("C-a-m", "C=1");
            }
          } else {
            if (curMillis - prevMillisChargerAutomodePS >= chargerAutomodePSInterval) {
              digitalWrite(chargerPSPin, HIGH);
              non_block_delay(2000); // 2 seconds
              float chargerVoltage = get_voltage(voltageCmd[0]);
              if (chargerVoltage < 13.0) {
                // disable PS charger and wait for 1h
                digitalWrite(chargerPSPin, LOW);
                prevMillisChargerAutomodePS = curMillis;
                if (chargerAutomodeMessages >= 1) {
                  char tmpBuf[10];
                  strcpy(tmpBuf, batSwitchCmd[i]);
                  strcat(tmpBuf, ":C-p=2"); // "C-p=2" mean no output from PS charger
                  busB_send("C-a-m", tmpBuf);
                }
                continue;
              }
              if (chargerAutomodeMessages == 2) {
                busB_send("C-a-m", "C-p=1");
              }
              prevMillisChargerAutomodePS = curMillis + chargerAutomodePSInterval;
            } else {
              continue;
            }
          }
          digitalWrite(batSwitchPin[i], HIGH);
          if (chargerAutomodeMessages >= 1) {
            char tmpBuf[6];
            strcpy(tmpBuf, batSwitchCmd[i]);
            strcat(tmpBuf, ":1");
            busB_send("C-a-m", tmpBuf);
            if (chargerAutomodeMessages == 2) {
              char tmpBuf[6];
              strcpy(tmpBuf, batSwitchCmd[i]);
              strcat(tmpBuf, "=1");
              busB_send("C-a-m", tmpBuf);
            }
          }
          break;
        }
      }
      prevMillisChargerAutomode = curMillis;
    }
  }
}

void autocharge_stop() {
  if (chargerAutomode == 1 and (digitalRead(chargerTypePin) == 1 or digitalRead(chargerPSPin) == 1)) {
    unsigned long curMillis = millis(); // time now in ms
    if (curMillis - prevMillisChargerAutomode >= chargerAutomodeInterval) {
      for (byte i = 0; i < batSwitchesNum; i += 1) {
        if (digitalRead(batSwitchPin[i]) == 1) {
          float chargerCurrent = get_current(currentCmd[digitalRead(chargerTypePin)]);
          float batCurrent = get_current(currentCmd[i+2]);
          float batTemperature = 0.0;
          if (temperatureControl[i] == 1) {
            batTemperature = get_temperature(i);
          }
          float chargerTemperature = 0.0;
          if (temperatureControl[2] == 1) {
            chargerTemperature = get_temperature(2);
          }
          if (chargerCurrent < 0.15 or batCurrent > 0.1 or batTemperature > 45 or chargerTemperature > 60) {
            digitalWrite(batSwitchPin[i], LOW);
            if (chargerAutomodeMessages >= 1) {
              if (batCurrent > 0.1) {
                char tmpBuf[14];
                strcpy(tmpBuf, batSwitchCmd[i]);
                strcat(tmpBuf, ":");
                strcat(tmpBuf, currentCmd[i+2]);
                strcat(tmpBuf, "=");
                char tmpBuf2[6];
                dtostrf(batCurrent, 0, 2, tmpBuf2);
                strcat(tmpBuf, tmpBuf2);
                busB_send("C-a-m", tmpBuf);
              } else if (batTemperature > 45) {
                char tmpBuf[16];
                strcpy(tmpBuf, batSwitchCmd[i]);
                strcat(tmpBuf, ":");
                strcat(tmpBuf, tSensorCmd[i]);
                strcat(tmpBuf, "=");
                char tmpBuf2[8];
                dtostrf(batTemperature, 0, 2, tmpBuf2);
                strcat(tmpBuf, tmpBuf2);
                busB_send("C-a-m", tmpBuf);
              } else if (chargerTemperature > 60) {
                char tmpBuf[16];
                strcpy(tmpBuf, batSwitchCmd[i]);
                strcat(tmpBuf, ":");
                strcat(tmpBuf, tSensorCmd[2]);
                strcat(tmpBuf, "=");
                char tmpBuf2[8];
                dtostrf(chargerTemperature, 0, 2, tmpBuf2);
                strcat(tmpBuf, tmpBuf2);
                busB_send("C-a-m", tmpBuf);
              } else {
                char tmpBuf[6];
                strcpy(tmpBuf, batSwitchCmd[i]);
                strcat(tmpBuf, ":1");
                busB_send("C-a-m", tmpBuf);
              }
              if (chargerAutomodeMessages == 2) {
                char tmpBuf[6];
                strcpy(tmpBuf, batSwitchCmd[i]);
                strcat(tmpBuf, "=0");
                busB_send("C-a-m", tmpBuf);
              }
            }
            if (digitalRead(chargerTypePin) == 1) {
              digitalWrite(chargerTypePin, LOW);
              if (chargerAutomodeMessages == 2) {
                busB_send("C-a-m", "C=0");
              }
            } else {
              digitalWrite(chargerPSPin, LOW);
              if (chargerAutomodeMessages == 2) {
                busB_send("C-a-m", "C-p=0");
              }
            }
            break;
          }
        }
      }
      prevMillisChargerAutomode = curMillis;
    }
  }
}

void autooutputs_control() {
  if (outputsAutomode == 1) {
    unsigned long curMillis = millis(); // time now in ms
    if (curMillis - prevMillisOutputsAutomode >= outputsAutomodeInterval) {
      for (byte i = 0; i < batSwitchesNum; i += 1) {
        float batVoltage = get_voltage(voltageCmd[i+2]);
        float batTemperature = 0.0;
        if (temperatureControl[i] == 1) {
          batTemperature = get_temperature(i);
        }
        if (batVoltage > 12.0 and batTemperature <= 42) {
          byte k = 0;
          byte outputsNumJ = outputsNum - 2;
          byte msgSent = 0;
          if (i == 1) {
            k = 2;
            outputsNumJ = outputsNum;
            msgSent = 0;
          }
          for (byte j = k; j < outputsNumJ; j += 1) {
            if (digitalRead(outputPin[j]) == 0) {
              digitalWrite(outputPin[j], HIGH);
              if (outputsAutomodeMessages >= 1) {
                if (msgSent == 0) {
                  char tmpBuf[6];
                  strcpy(tmpBuf, batSwitchCmd[i]);
                  strcat(tmpBuf, ":1");
                  busB_send("O-a-m", tmpBuf);
                  msgSent = 1;
                }
                if (outputsAutomodeMessages == 2) {
                  char tmpBuf[6];
                  strcpy(tmpBuf, outputCmd[j]);
                  strcat(tmpBuf, "=1");
                  busB_send("O-a-m", tmpBuf);
                }
              }
            }
          }
        } else if (batVoltage < 11.5 or batTemperature > 45) {
          byte k = 0;
          byte outputsNumJ = outputsNum - 2;
          byte msgSent = 0;
          if (i == 1) {
            k = 2;
            outputsNumJ = outputsNum;
            msgSent = 0;
          }
          for (byte j = k; j < outputsNumJ; j += 1) {
            if (digitalRead(outputPin[j]) == 1) {
              digitalWrite(outputPin[j], LOW);
              if (outputsAutomodeMessages >= 1) {
                if (msgSent == 0) {
                  if (batVoltage < 11.5) {
                    char tmpBuf[14];
                    strcpy(tmpBuf, batSwitchCmd[i]);
                    strcat(tmpBuf, ":");
                    strcat(tmpBuf, voltageCmd[i+2]);
                    strcat(tmpBuf, "=");
                    char tmpBuf2[6];
                    dtostrf(batVoltage, 0, 2, tmpBuf2);
                    strcat(tmpBuf, tmpBuf2);
                    busB_send("O-a-m", tmpBuf);
                  } else if (batTemperature > 45) {
                    char tmpBuf[16];
                    strcpy(tmpBuf, batSwitchCmd[i]);
                    strcat(tmpBuf, ":");
                    strcat(tmpBuf, tSensorCmd[i]);
                    strcat(tmpBuf, "=");
                    char tmpBuf2[8];
                    dtostrf(batTemperature, 0, 2, tmpBuf2);
                    strcat(tmpBuf, tmpBuf2);
                    busB_send("O-a-m", tmpBuf);
                  }
                  msgSent = 1;
                }
                if (outputsAutomodeMessages == 2) {
                  char tmpBuf[6];
                  strcpy(tmpBuf, outputCmd[j]);
                  strcat(tmpBuf, "=0");
                  busB_send("O-a-m", tmpBuf);
                }
              }
            }
          }
        } else {
          if (outputsAutomodeMessages >= 1) {
            if (batVoltage <= 12.0) {
              char tmpBuf[14];
              strcpy(tmpBuf, batSwitchCmd[i]);
              strcat(tmpBuf, ":");
              strcat(tmpBuf, voltageCmd[i+2]);
              strcat(tmpBuf, "=");
              char tmpBuf2[6];
              dtostrf(batVoltage, 0, 2, tmpBuf2);
              strcat(tmpBuf, tmpBuf2);
              busB_send("O-a-m", tmpBuf);
            } else if (batTemperature > 42) {
              char tmpBuf[16];
              strcpy(tmpBuf, batSwitchCmd[i]);
              strcat(tmpBuf, ":");
              strcat(tmpBuf, tSensorCmd[i]);
              strcat(tmpBuf, "=");
              char tmpBuf2[8];
              dtostrf(batTemperature, 0, 2, tmpBuf2);
              strcat(tmpBuf, tmpBuf2);
              busB_send("O-a-m", tmpBuf);
            }
          }
        }
      }
      prevMillisOutputsAutomode = curMillis;
    }
  }
}


void loop() {
  busA.receive(receiveTimeBusA);
  autopush();
  autocharge_start();
  autocharge_stop();
  autooutputs_control();
}

void setup() {
  // for debug only
  Serial.begin(9600);

  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) { //EEprom is wrong version or was not programmed, write default values to the EEprom
    EEPROM.update(eepromChargerAutomodeAddr, chargerAutomode);
    EEPROM.update(eepromChargerAutomodeMessagesAddr, chargerAutomodeMessages);
    EEPROM.update(eepromOutputsAutomodeAddr, outputsAutomode);
    EEPROM.update(eepromOutputsAutomodeMessagesAddr, outputsAutomodeMessages);
    for (byte i = 0; i < voltageNum; i += 1) {
      EEPROM.update(eepromVoltageAutoPushAddr[i], voltageAutoPush[i]);
    }
    for (byte i = 0; i < currentNum; i += 1) {
      EEPROM.update(eepromCurrentAutoPushAddr[i], currentAutoPush[i]);
    }
    for (byte i = 0; i < powerNum; i += 1) {
      EEPROM.update(eepromPowerAutoPushAddr[i], powerAutoPush[i]);
    }
    for (byte i = 0; i < tSensorsNum; i += 1) {
      EEPROM.update(eepromTSensorAutoPushAddr[i], tSensorAutoPush[i]);
      EEPROM.update(eepromTemperatureControlAddr[i], temperatureControl[i]);
    }
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    chargerAutomode = EEPROM.read(eepromChargerAutomodeAddr);
    chargerAutomodeMessages = EEPROM.read(eepromChargerAutomodeMessagesAddr);
    outputsAutomode = EEPROM.read(eepromOutputsAutomodeAddr);
    outputsAutomodeMessages = EEPROM.read(eepromOutputsAutomodeMessagesAddr);
    for (byte i = 0; i < voltageNum; i += 1) {
      voltageAutoPush[i] = EEPROM.read(eepromVoltageAutoPushAddr[i]);
    }
    for (byte i = 0; i < currentNum; i += 1) {
      currentAutoPush[i] = EEPROM.read(eepromCurrentAutoPushAddr[i]);
    }
    for (byte i = 0; i < powerNum; i += 1) {
      powerAutoPush[i] = EEPROM.read(eepromPowerAutoPushAddr[i]);
    }
    for (byte i = 0; i < tSensorsNum; i += 1) {
      tSensorAutoPush[i] = EEPROM.read(eepromTSensorAutoPushAddr[i]);
      temperatureControl[i] = EEPROM.read(eepromTemperatureControlAddr[i]);
    }
  }

  // Set relays state
  pinMode(chargerTypePin, OUTPUT);
  digitalWrite(chargerTypePin, LOW);
  pinMode(chargerPSPin, OUTPUT);
  digitalWrite(chargerPSPin, LOW);

  // Set Batteries switches state
  for (byte i = 0; i < batSwitchesNum; i += 1) {
    pinMode(batSwitchPin[i], OUTPUT);
    digitalWrite(batSwitchPin[i], LOW);
  }

  // Set outputs state
  for (byte i = 0; i < outputsNum; i += 1) {
    pinMode(outputPin[i], OUTPUT);
    digitalWrite(outputPin[i], LOW);
  }

  // Set temperature sensor resolution, valid values are 9, 10, 11 or 12 bit.
  sensors.begin();
  for (byte i = 0; i < tSensorsNum; i += 1) {
    sensors.setResolution(tSensorAddr[i], 11);
  }

  busA.set_error(error_handlerA);
  busA.strategy.set_pin(pinBusA);
  busA.set_receiver(receiver_function);
  busA.set_acknowledge(true);
  busA.set_crc_32(true);
  busA.set_packet_id(true);
  busA.begin();

  busB.set_error(error_handlerB);
  busB.strategy.set_pin(pinBusB);
  busB.set_acknowledge(true);
  busB.set_crc_32(true);
  busB.set_packet_id(true);
  busB.begin();
};