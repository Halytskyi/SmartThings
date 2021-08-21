/*
* Copyright (C) 2020 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#define PJON_INCLUDE_PACKET_ID
#include <PJONSoftwareBitBang.h>
#include <EEPROM.h>

const byte deviceID = 18;
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

// Voltage params
const float arduinoVoltage = 4.945;

// Voltage sensors
const byte voltageNum = 4;
// {"command"}
const char *const voltageCmd[voltageNum] = {"V-1", "V-2", "V-3", "V-4"};
// {"auto-push"}
byte voltageAutoPush[voltageNum] = {0, 0, 0, 0};
// {"auto-push last update"}
unsigned long voltageAutoPushLU[voltageNum] = {0, 0, 0, 0};
// {"EEProm auto-push"}
const byte eepromVoltageAutoPushAddr[voltageNum] = {1, 2, 3, 4};

// Current sensors
const byte currentNum = 4;
// {"command"}
const char *const currentCmd[currentNum] = {"I-1", "I-2", "I-3", "I-4"};
// {"auto-push"}
byte currentAutoPush[currentNum] = {0, 0, 0, 0};
// {"auto-push last update"}
unsigned long currentAutoPushLU[currentNum] = {0, 0, 0, 0};
// {"EEProm auto-push"}
const byte eepromCurrentAutoPushAddr[currentNum] = {5, 6, 7, 8};

// Power consumption
const byte powerNum = 4;
// {"command"}
const char *const powerCmd[powerNum] = {"P-1", "P-2", "P-3", "P-4"};
// {"auto-push"}
byte powerAutoPush[powerNum] = {0, 0, 0, 0};
// {"auto-push last update"}
unsigned long powerAutoPushLU[powerNum] = {0, 0, 0, 0};
// {"EEProm auto-push"}
const byte eepromPowerAutoPushAddr[powerNum] = {9, 10, 11, 12};


float get_voltage(const char command[]) {
  float voltageValue = 0.0;
  byte voltagePin = 0;
  float r1 = 0.0;
  float r2 = 0.0;
  const byte voltageCountValues = 50; // how many values must be averaged

  if (strcmp(command, "V-1") == 0) {
    voltagePin = 0; // UPS output #1
    r1 = 100470; // R1 = 100k
    r2 = 9710; // R2 = 10k
  } else if (strcmp(command, "V-2") == 0) {
    voltagePin = 1; // UPS output #2
    r1 = 99470; // R1 = 100k
    r2 = 9870; // R2 = 10k //9700
  } else if (strcmp(command, "V-3") == 0) {
    voltagePin = 2; // UPS output #3
    r1 = 100900; // R1 = 100k //97600
    r2 = 9700; // R2 = 10k //10100
  } else if (strcmp(command, "V-4") == 0) {
    voltagePin = 3; // UPS output #4
    r1 = 100500; // R1 = 100k
    r2 = 9717; // R2 = 10k
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
    currentPin = 6; // UPS output #1
    offsetVoltage = 2.457;
    direction = 0;
  } else if (strcmp(command, "I-2") == 0) {
    currentPin = 7; // UPS output #2
    offsetVoltage = 2.459;
    direction = 0;
  } else if (strcmp(command, "I-3") == 0) {
    currentPin = 4; // UPS output #3
    offsetVoltage = 2.479; //2.503
    direction = 1;
  } else if (strcmp(command, "I-4") == 0) {
    currentPin = 5; // UPS output #4
    offsetVoltage = 2.475; //2.486
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
    prevMillisAutoPush = curMillis;
  }
}

void loop() {
  busA.receive(receiveTimeBusA);
  autopush();
}

void setup() {
  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) { //EEprom is wrong version or was not programmed, write default values to the EEprom
    for (byte i = 0; i < voltageNum; i += 1) {
      EEPROM.update(eepromVoltageAutoPushAddr[i], voltageAutoPush[i]);
    }
    for (byte i = 0; i < currentNum; i += 1) {
      EEPROM.update(eepromCurrentAutoPushAddr[i], currentAutoPush[i]);
    }
    for (byte i = 0; i < powerNum; i += 1) {
      EEPROM.update(eepromPowerAutoPushAddr[i], powerAutoPush[i]);
    }
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    for (byte i = 0; i < voltageNum; i += 1) {
      voltageAutoPush[i] = EEPROM.read(eepromVoltageAutoPushAddr[i]);
    }
    for (byte i = 0; i < currentNum; i += 1) {
      currentAutoPush[i] = EEPROM.read(eepromCurrentAutoPushAddr[i]);
    }
    for (byte i = 0; i < powerNum; i += 1) {
      powerAutoPush[i] = EEPROM.read(eepromPowerAutoPushAddr[i]);
    }
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