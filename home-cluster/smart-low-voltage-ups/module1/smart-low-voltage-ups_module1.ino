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

// DC chargers
const byte dcChargersNum = 2;
// {"command"}
const char *const dcChargerCmd[dcChargersNum] = {"C-1", "C-2"};
// {"switch pin"}
const byte dcChargerSwitchPin[dcChargersNum] = {11, 8};
// {"status led pin"}
const byte dcChargerStatusLedPin[dcChargersNum] = {10, 9};
// {"automode"}
byte dcChargerAutomode[dcChargersNum] = {1, 1};
// {"EEProm automode"}
const byte eepromDcChargerAutomodeAddr[dcChargersNum] = {1, 2};
// Automode messages
byte dcChargersAutomodeMessages = 0;
const byte eepromDcChargersAutomodeMessagesAddr = 3;
// Autocharge start mode
const unsigned int chargerAutomodeInterval = 60000; // interval for autocharge mode
unsigned long prevMillisChargerAutomodeStart = 4294937295; // after start delay 30s (4294967295 - 30000 = 4294937295)
unsigned long prevMillisChargerAutomodeStop = 4294952295; // after start delay 45s (4294967295 - 15000 = 4294952295)

// Outputs
const byte outputsNum = 4;
// {"command"}
const char *const outputCmd[outputsNum] = {"O-1", "O-2", "O-3", "O-4"};
// {"pin"}
const byte outputPin[outputsNum] = {2, 3, 4, 5};
// Outputs control automode
byte outputsAutomode = 1;
const byte eepromOutputsAutomodeAddr = 4;
byte outputsAutomodeMessages = 0;
const byte eepromOutputsAutomodeMessagesAddr = 5;
const unsigned int outputsAutomodeInterval = 60000; // interval for outputs autocharge mode
unsigned long prevMillisOutputsAutomode = 4294937295; // after start delay 30s (4294967295 - 30000 = 4294937295)

// Voltage params
const float arduinoVoltage = 5.012;

// Voltage sensors
const byte voltageNum = 4;
// {"command"}
const char *const voltageCmd[voltageNum] = {"V-1", "V-2", "V-3", "V-4"};
// {"auto-push"}
byte voltageAutoPush[voltageNum] = {0, 0, 0, 0};
// {"auto-push last update"}
unsigned long voltageAutoPushLU[voltageNum] = {0, 0, 0, 0};
// {"EEProm auto-push"}
const byte eepromVoltageAutoPushAddr[voltageNum] = {6, 7, 8, 9};

// Current sensors
const byte currentNum = 4;
// {"command"}
const char *const currentCmd[currentNum] = {"I-1", "I-2", "I-3", "I-4"};
// {"auto-push"}
byte currentAutoPush[currentNum] = {0, 0, 0, 0};
// {"auto-push last update"}
unsigned long currentAutoPushLU[currentNum] = {0, 0, 0, 0};
// {"EEProm auto-push"}
const byte eepromCurrentAutoPushAddr[currentNum] = {10, 11, 12, 13};

// Power consumption
const byte powerNum = 4;
// {"command"}
const char *const powerCmd[powerNum] = {"P-1", "P-2", "P-3", "P-4"};
// {"auto-push"}
byte powerAutoPush[powerNum] = {0, 0, 0, 0};
// {"auto-push last update"}
unsigned long powerAutoPushLU[powerNum] = {0, 0, 0, 0};
// {"EEProm auto-push"}
const byte eepromPowerAutoPushAddr[powerNum] = {14, 15, 16, 17};

// Temperature sensors
const byte tSensorsNum = 2;
OneWire oneWire(6);
DallasTemperature sensors(&oneWire);
// {"command"}
const char *const tSensorCmd[tSensorsNum] = {"T-1", "T-2"};
// {"auto-push"}
byte tSensorAutoPush[tSensorsNum] = {0, 0};
// {"auto-push last update"}
unsigned long tSensorAutoPushLU[tSensorsNum] = {0, 0};
// {"EEProm auto-push"}
const byte eepromTSensorAutoPushAddr[tSensorsNum] = {18, 19};
const DeviceAddress tSensorAddr[tSensorsNum] = {
  {0x28, 0x7E, 0xBB, 0xC6, 0x38, 0x19, 0x01, 0x61},  // Battery 1
  {0x28, 0x3C, 0x7C, 0x5E, 0x39, 0x19, 0x01, 0x5F}}; // Battery 2

// Temperature control
// {"command"}
const char *const temperatureControlCmd[tSensorsNum] = {"T-c-1", "T-c-2"};
// {"value"}
byte temperatureControl[tSensorsNum] = {1, 1};
const byte eepromTemperatureControlAddr[tSensorsNum] = {20, 21};


float get_voltage(const char command[]) {
  float voltageValue = 0.0;
  byte voltagePin = 0;
  float r1 = 0.0;
  float r2 = 0.0;
  const byte voltageCountValues = 50; // how many values must be averaged

  if (strcmp(command, "V-1") == 0) {
    voltagePin = 4; // Battery #1
    r1 = 99000; // R1 = 100k
    r2 = 9732; // R2 = 10k
  } else if (strcmp(command, "V-2") == 0) {
    voltagePin = 5; // Battery #2
    r1 = 99700; // R1 = 100k
    r2 = 9853; // R2 = 10k
  } else if (strcmp(command, "V-3") == 0) {
    voltagePin = 6; // DC charger #1
    r1 = 100300; // R1 = 100k
    r2 = 9869; // R2 = 10k
  } else if (strcmp(command, "V-4") == 0) {
    voltagePin = 7; // DC charger #2
    r1 = 99000; // R1 = 100k
    r2 = 10880; // R2 = 10k
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
    currentPin = 1; // Battery #1
    offsetVoltage = 2.504;
    direction = 0;
  } else if (strcmp(command, "I-2") == 0) {
    currentPin = 2; // Battery #2
    offsetVoltage = 2.511;
    direction = 1;
  } else if (strcmp(command, "I-3") == 0) {
    currentPin = 0; // DC charger #1
    offsetVoltage = 2.488;
    direction = 0;
  } else if (strcmp(command, "I-4") == 0) {
    currentPin = 3; // DC charger #2
    offsetVoltage = 2.518;
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

  // DC chargers state
  for (byte i = 0; i < dcChargersNum; i += 1) {
    if (strcmp(command, dcChargerCmd[i]) == 0) {
      if (valueLength == 0) {
        itoa(digitalRead(dcChargerSwitchPin[i]), tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1) {
          digitalWrite(dcChargerSwitchPin[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, dcChargerCmd[i]);
    strcat(tmpBuf, "-a");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(dcChargerAutomode[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1 or value_int == 2) {
          dcChargerAutomode[i] = value_int;
          EEPROM.update(eepromDcChargerAutomodeAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  if (strcmp(command, "C-a-m") == 0) {
    if (valueLength == 0) {
      itoa(dcChargersAutomodeMessages, tmpBuf, 10);
      busA_reply(request, tmpBuf);
      return;
    } else {
      if (value_int >= 0 and value_int <= 3) {
        dcChargersAutomodeMessages = value_int;
        EEPROM.update(eepromDcChargersAutomodeMessagesAddr, value_int);
        busA_reply(request, goodCommandReply);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
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
      if (value_int >= 0 and value_int <= 3) {
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

void autocharge_start() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillisChargerAutomodeStart >= chargerAutomodeInterval) {
    for (byte i = 0; i < dcChargersNum; i += 1) {
      if (dcChargerAutomode[i] >= 1 and digitalRead(dcChargerSwitchPin[i]) == 0) {
        float batVoltage = get_voltage(voltageCmd[i]);
        if (batVoltage > 5 and batVoltage < 12.5) {
          float batCurrent = get_current(currentCmd[i]);
          if (batCurrent > 0.1) {
            if (dcChargersAutomodeMessages == 1 or dcChargersAutomodeMessages == 3) {
              char tmpBuf[14];
              strcpy(tmpBuf, dcChargerCmd[i]);
              strcat(tmpBuf, ":");
              strcat(tmpBuf, currentCmd[i]);
              strcat(tmpBuf, "=");
              char tmpBuf2[6];
              dtostrf(batCurrent, 0, 2, tmpBuf2);
              strcat(tmpBuf, tmpBuf2);
              busB_send("C-a-m", tmpBuf);
            }
            continue;
          }
          if (temperatureControl[i] == 1) {
            float batTemperature = get_temperature(i);
            if (batTemperature > 42) {
              if (dcChargersAutomodeMessages == 1 or dcChargersAutomodeMessages == 3) {
                char tmpBuf[16];
                strcpy(tmpBuf, dcChargerCmd[i]);
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
          float inputChargerVoltage = get_voltage(voltageCmd[i+2]);
          if (inputChargerVoltage < 15) {
            if (dcChargersAutomodeMessages == 1 or dcChargersAutomodeMessages == 3) {
              char tmpBuf[6];
              strcpy(tmpBuf, dcChargerCmd[i]);
              strcat(tmpBuf, ":2");
              busB_send("C-a-m", tmpBuf);
            }
            continue;
          }
          digitalWrite(dcChargerSwitchPin[i], HIGH);
          if (dcChargersAutomodeMessages == 1 or dcChargersAutomodeMessages == 3) {
            char tmpBuf[6];
            strcpy(tmpBuf, dcChargerCmd[i]);
            strcat(tmpBuf, ":1");
            busB_send("C-a-m", tmpBuf);
            if (dcChargersAutomodeMessages >= 2) {
              char tmpBuf[6];
              strcpy(tmpBuf, dcChargerCmd[i]);
              strcat(tmpBuf, "=1");
              busB_send("C-a-m", tmpBuf);
            }
          }
        }
      }
    }
    prevMillisChargerAutomodeStart = curMillis;
  }
}

void autocharge_stop() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillisChargerAutomodeStop >= chargerAutomodeInterval) {
    for (byte i = 0; i < dcChargersNum; i += 1) {
      if (dcChargerAutomode[i] >= 1 and digitalRead(dcChargerSwitchPin[i]) == 1) {
        float inputChargerVoltage = get_voltage(voltageCmd[i+2]);
        float chargerCurrent = get_current(currentCmd[i+2]);
        byte chargerStatus = 1;
        float batCurrent = get_current(currentCmd[i]);
        float batTemperature = 0.0;
        if (dcChargerAutomode[i] == 2) {
          chargerStatus = digitalRead(dcChargerStatusLedPin[i]); // 0 - charged, 1 - charging
        }
        if (temperatureControl[i] == 1) {
          batTemperature = get_temperature(i);
        }
        if (inputChargerVoltage < 15 or chargerCurrent < 0.15 or chargerStatus == 0 or batCurrent > 0.1 or batTemperature > 45) {
          digitalWrite(dcChargerSwitchPin[i], LOW);
          if (dcChargersAutomodeMessages == 1 or dcChargersAutomodeMessages == 3) {
            if (batCurrent > 0.1) {
              char tmpBuf[14];
              strcpy(tmpBuf, dcChargerCmd[i]);
              strcat(tmpBuf, ":");
              strcat(tmpBuf, currentCmd[i]);
              strcat(tmpBuf, "=");
              char tmpBuf2[6];
              dtostrf(batCurrent, 0, 2, tmpBuf2);
              strcat(tmpBuf, tmpBuf2);
              busB_send("C-a-m", tmpBuf);
            }
            if (batTemperature > 45) {
              char tmpBuf[16];
              strcpy(tmpBuf, dcChargerCmd[i]);
              strcat(tmpBuf, ":");
              strcat(tmpBuf, tSensorCmd[i]);
              strcat(tmpBuf, "=");
              char tmpBuf2[8];
              dtostrf(batTemperature, 0, 2, tmpBuf2);
              strcat(tmpBuf, tmpBuf2);
              busB_send("C-a-m", tmpBuf);
            }
            if (inputChargerVoltage < 15) {
              char tmpBuf[6];
              strcpy(tmpBuf, dcChargerCmd[i]);
              strcat(tmpBuf, ":2");
              busB_send("C-a-m", tmpBuf);
            } else {
              char tmpBuf[6];
              strcpy(tmpBuf, dcChargerCmd[i]);
              strcat(tmpBuf, ":1");
              busB_send("C-a-m", tmpBuf);
            }
            if (dcChargersAutomodeMessages >= 2) {
              char tmpBuf[6];
              strcpy(tmpBuf, dcChargerCmd[i]);
              strcat(tmpBuf, "=0");
              busB_send("C-a-m", tmpBuf);
            }
          }
        }
      }
    }
    prevMillisChargerAutomodeStop = curMillis;
  }
}

void autooutputs_control() {
  if (outputsAutomode == 1) {
    unsigned long curMillis = millis(); // time now in ms
    if (curMillis - prevMillisOutputsAutomode >= outputsAutomodeInterval) {
      for (byte i = 0; i < 2; i += 1) {
        float batVoltage = get_voltage(voltageCmd[i]);
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
              if (outputsAutomodeMessages == 1 or outputsAutomodeMessages == 3) {
                if (msgSent == 0) {
                  char tmpBuf[6];
                  char tmpBuf2[2];
                  strcpy(tmpBuf, "B-");
                  itoa(i+1, tmpBuf2, 10);
                  strcat(tmpBuf, tmpBuf2);
                  strcat(tmpBuf, ":1");
                  busB_send("O-a-m", tmpBuf);
                  msgSent = 1;
                }
                if (outputsAutomodeMessages >= 2) {
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
              if (outputsAutomodeMessages == 1 or outputsAutomodeMessages == 3) {
                if (msgSent == 0) {
                  if (batVoltage < 11.5) {
                    char tmpBuf[14];
                    char tmpBuf2[2];
                    char tmpBuf3[6];
                    strcpy(tmpBuf, "B-");
                    itoa(i+1, tmpBuf2, 10);
                    strcat(tmpBuf, tmpBuf2);
                    strcat(tmpBuf, ":");
                    strcat(tmpBuf, voltageCmd[i]);
                    strcat(tmpBuf, "=");
                    dtostrf(batVoltage, 0, 2, tmpBuf3);
                    strcat(tmpBuf, tmpBuf3);
                    busB_send("O-a-m", tmpBuf);
                  }
                  if (batTemperature > 45) {
                    char tmpBuf[16];
                    char tmpBuf2[2];
                    char tmpBuf3[8];
                    strcpy(tmpBuf, "B-");
                    itoa(i+1, tmpBuf2, 10);
                    strcat(tmpBuf, tmpBuf2);
                    strcat(tmpBuf, ":");
                    strcat(tmpBuf, tSensorCmd[i]);
                    strcat(tmpBuf, "=");
                    dtostrf(batTemperature, 0, 2, tmpBuf3);
                    strcat(tmpBuf, tmpBuf3);
                    busB_send("O-a-m", tmpBuf);
                  }
                  msgSent = 1;
                }
                if (outputsAutomodeMessages >= 2) {
                  char tmpBuf[6];
                  strcpy(tmpBuf, outputCmd[j]);
                  strcat(tmpBuf, "=0");
                  busB_send("O-a-m", tmpBuf);
                }
              }
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
  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) { //EEprom is wrong version or was not programmed, write default values to the EEprom
    for (byte i = 0; i < dcChargersNum; i += 1) {
      EEPROM.update(eepromDcChargerAutomodeAddr[i], dcChargerAutomode[i]);
    }
    EEPROM.update(eepromDcChargersAutomodeMessagesAddr, dcChargersAutomodeMessages);
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
    for (byte i = 0; i < dcChargersNum; i += 1) {
      dcChargerAutomode[i] = EEPROM.read(eepromDcChargerAutomodeAddr[i]);
    }
    dcChargersAutomodeMessages = EEPROM.read(eepromDcChargersAutomodeMessagesAddr);
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

  // Set chargers Switch and LED PINs state
  for (byte i = 0; i < dcChargersNum; i += 1) {
    pinMode(dcChargerSwitchPin[i], OUTPUT);
    digitalWrite(dcChargerSwitchPin[i], LOW);
    pinMode(dcChargerStatusLedPin[i], INPUT);
  }

  // Set outputs PINs state
  for (byte i = 0; i < outputsNum; i += 1) {
    pinMode(outputPin[i], OUTPUT);
    digitalWrite(outputPin[i], LOW);
  }

  // Set temperature sensor resolution, valid values are 9, 10, 11 or 12 bit.
  sensors.begin();
  for (byte i = 0; i < tSensorsNum; i += 1) {
    sensors.setResolution(tSensorAddr[i], 11);
  }

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