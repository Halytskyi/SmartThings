/*
* Copyright (C) 2021 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#include <Wire.h>
#include <EEPROM.h>

#define SLAVE_ADDRESS 0x03

const byte ADDR_VERSION = 255;         // Location of the software version in EEPROM
const byte CURRENT_EEPROM_VERSION = 1; // We are on revision 1 of the EEPROM storage structure (0xFF or 255 is never a valid value for version)

const char badCommandReply[] = "err";
const char endReply[] = ";";

// Control outputs
const byte controlOutputsNum = 11;
// {"pin"}
const byte controlOutputsPin[controlOutputsNum] = {2, 3, 4, 5, 6, 7, 8, 9, 12, 11, 10};
// {"EEProm control outputs state"}
const byte eepromControlOutputsStateAddr[controlOutputsNum] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
// Reboot params
byte rebootPin = 255;
byte rebootStatus = 0;
const unsigned int rebootTime = 5000; // 5 seconds

// USB switch selectors
const byte usbSwitchSelectorsNum = 2;
// {"pin"}
const byte usbSwitchSelectorsButtonPin[usbSwitchSelectorsNum] = {13, 14};
// {"pin"}
const byte usbSwitchSelectorsLed1Pin[usbSwitchSelectorsNum] = {15, 16};
// {"EEProm for Led #1 state"}
const byte eepromUSBSwitchSelectorsLed1StateAddr[usbSwitchSelectorsNum] = {12, 13};
// USB switch params for switching inputs
byte usbSwitchSelectorIndex = 255;
byte usbSwitchSelectorLed1StatusBeforeSwitch = 255;
unsigned long prevMillisClearBeforeSwitch = 0;

const byte buttonControlPin = 17;
const unsigned int buttonTime = 2000; // 2 seconds

char sendStatus[controlOutputsNum + 1];
byte index = 0;


void sendData() {
  Wire.write(sendStatus[index]);
  ++index;
  if (index >= strlen(sendStatus)) {
    index = 0;
  }
}

void receiveData(int byteCount) {
  byte receiveByte = 0;
  char command[5];
  char value[3];
  while (Wire.available()) {
    command[receiveByte] = Wire.read();
    receiveByte++;
  }
  command[receiveByte] = 0;
  if (isDigit(command[2])) {
    value[0] = command[2];
    if (isDigit(command[3])) {
      value[1] = command[3];
      value[2] = 0;
    } else {
      value[1] = 0;
    }
  } else {
    strcpy(sendStatus, badCommandReply);
    strcat(sendStatus, endReply);
    return;
  }
  byte value_int = atoi(value);

  if (command[0] == 'c') {
    if (command[1] == 'e') {
      if (value_int >= 1 && value_int <= controlOutputsNum) {
        if (rebootPin != controlOutputsPin[value_int - 1]) {
          digitalWrite(controlOutputsPin[value_int - 1], HIGH);
          EEPROM.update(eepromControlOutputsStateAddr[value_int - 1], 1);
          itoa(digitalRead(controlOutputsPin[value_int - 1]), sendStatus, 10);
          strcat(sendStatus, endReply);
          return;
        } else {
          strcpy(sendStatus, "2");
          strcat(sendStatus, endReply);
          return;
        }
      } else {
        strcpy(sendStatus, badCommandReply);
        strcat(sendStatus, endReply);
        return;
      }
    } else if (command[1] == 'd') {
      if (value_int >= 1 && value_int <= controlOutputsNum) {
        if (rebootPin != controlOutputsPin[value_int - 1]) {
          digitalWrite(controlOutputsPin[value_int - 1], LOW);
          EEPROM.update(eepromControlOutputsStateAddr[value_int - 1], 0);
          itoa(digitalRead(controlOutputsPin[value_int - 1]), sendStatus, 10);
          strcat(sendStatus, endReply);
          return;
        } else {
          strcpy(sendStatus, "2");
          strcat(sendStatus, endReply);
          return;
        }
      } else {
        strcpy(sendStatus, badCommandReply);
        strcat(sendStatus, endReply);
        return;
      }
    } else if (command[1] == 'r') {
      if (value_int >= 1 && value_int <= controlOutputsNum) {
        byte rResponse = digitalRead(controlOutputsPin[value_int - 1]);
        if (rResponse == 1) {
          if (rebootStatus == 0) {
            rebootPin = controlOutputsPin[value_int - 1];
            strcpy(sendStatus, "1");
            strcat(sendStatus, endReply);
            return;
          } else {
            strcpy(sendStatus, "2");
            strcat(sendStatus, endReply);
            return;
          }
        } else {
          strcpy(sendStatus, "0");
          strcat(sendStatus, endReply);
          return;
        }
      } else {
        strcpy(sendStatus, badCommandReply);
        strcat(sendStatus, endReply);
        return;
      }
    } else if (command[1] == 's') {
      if (value_int >= 1 && value_int <= controlOutputsNum) {
        itoa(digitalRead(controlOutputsPin[value_int - 1]), sendStatus, 10);
        strcat(sendStatus, endReply);
        return;
      } else if (value_int == 0) {
        for (byte i = 0; i < controlOutputsNum; i += 1) {
          sendStatus[i] = digitalRead(controlOutputsPin[i]) + '0';
        }
        sendStatus[controlOutputsNum] = 0;
        return;
      } else {
        strcpy(sendStatus, badCommandReply);
        strcat(sendStatus, endReply);
        return;
      }
    } else {
      strcpy(sendStatus, badCommandReply);
      strcat(sendStatus, endReply);
      return;
    }
  } else if (command[0] == 'u') {
    if (command[1] == 'e') {
      byte usbSwitchSelectorsLed1Status;
      if (value_int == 11) {
        usbSwitchSelectorIndex = 0;
        usbSwitchSelectorsLed1Status = 1;
      } else if (value_int == 12) {
        usbSwitchSelectorIndex = 0;
        usbSwitchSelectorsLed1Status = 0;
      } else if (value_int == 21) {
        usbSwitchSelectorIndex = 1;
        usbSwitchSelectorsLed1Status = 1;
      } else if (value_int == 22) {
        usbSwitchSelectorIndex = 1;
        usbSwitchSelectorsLed1Status = 0;
      } else {
        strcpy(sendStatus, badCommandReply);
        strcat(sendStatus, endReply);
        return;
      }
      if (digitalRead(usbSwitchSelectorsLed1Pin[usbSwitchSelectorIndex]) != usbSwitchSelectorsLed1Status) {
        usbSwitchSelectorLed1StatusBeforeSwitch = digitalRead(usbSwitchSelectorsLed1Pin[usbSwitchSelectorIndex]);
        prevMillisClearBeforeSwitch = millis();
        EEPROM.update(eepromUSBSwitchSelectorsLed1StateAddr[usbSwitchSelectorIndex], usbSwitchSelectorsLed1Status);
        strcpy(sendStatus, "1");
      } else {
        usbSwitchSelectorIndex = 255;
        strcpy(sendStatus, "0");
      }
      strcat(sendStatus, endReply);
      return;
    } else if (command[1] == 's') {
      if (value_int == 0) {
        for (byte i = 0; i < usbSwitchSelectorsNum; i += 1) {
          sendStatus[i] = digitalRead(usbSwitchSelectorsLed1Pin[i]) + '0';
        }
        sendStatus[usbSwitchSelectorsNum] = 0;
      } else if (value_int == 1) {
        if (usbSwitchSelectorLed1StatusBeforeSwitch != 255 and usbSwitchSelectorLed1StatusBeforeSwitch == digitalRead(usbSwitchSelectorsLed1Pin[0])) {
          usbSwitchSelectorLed1StatusBeforeSwitch = 255;
          strcpy(sendStatus, "2");
        } else {
          itoa(digitalRead(usbSwitchSelectorsLed1Pin[0]), sendStatus, 10);
        }
      } else if (value_int == 2) {
        if (usbSwitchSelectorLed1StatusBeforeSwitch != 255 and usbSwitchSelectorLed1StatusBeforeSwitch == digitalRead(usbSwitchSelectorsLed1Pin[1])) {
          usbSwitchSelectorLed1StatusBeforeSwitch = 255;
          strcpy(sendStatus, "2");
        } else {
          itoa(digitalRead(usbSwitchSelectorsLed1Pin[1]), sendStatus, 10);
        }
      } else {
        strcpy(sendStatus, badCommandReply);
      }
      strcat(sendStatus, endReply);
      return;
    } else {
      strcpy(sendStatus, badCommandReply);
      strcat(sendStatus, endReply);
      return;
    }
  } else {
    strcpy(sendStatus, badCommandReply);
    strcat(sendStatus, endReply);
    return;
  }
}

void reboot() {
  if (rebootPin != 255) {
    rebootStatus = 1;
    digitalWrite(rebootPin, LOW);
    delay(rebootTime);
    digitalWrite(rebootPin, HIGH);
    rebootPin = 255;
    rebootStatus = 0;
  }
}

void usbSwitchSelector() {
  if (usbSwitchSelectorIndex != 255) {
    digitalWrite(usbSwitchSelectorsButtonPin[usbSwitchSelectorIndex], HIGH);
    delay(300);
    digitalWrite(usbSwitchSelectorsButtonPin[usbSwitchSelectorIndex], LOW);
    usbSwitchSelectorIndex = 255;
  }
}

void usbSwitchSelectorClearBeforeSwitch() {
  unsigned long curMillis = millis(); // time now in ms
  if (curMillis - prevMillisClearBeforeSwitch >= 3000) {
    if (usbSwitchSelectorLed1StatusBeforeSwitch != 255) {
      usbSwitchSelectorLed1StatusBeforeSwitch = 255;
    }
  }
}

void buttonsControl() {
  // Button for control outputs
  byte buttonControlStatus = digitalRead(buttonControlPin);
  if (buttonControlStatus == HIGH) {
    for (byte i = 0; i < controlOutputsNum; i += 1) {
      if (digitalRead(controlOutputsPin[i]) == 0) {
        digitalWrite(controlOutputsPin[i], HIGH);
        EEPROM.update(eepromControlOutputsStateAddr[i], 1);
        break;
      }
    }
    delay(buttonTime);
  }
}

void loop() {
  reboot();
  usbSwitchSelector();
  usbSwitchSelectorClearBeforeSwitch();
  buttonsControl();
  delay(100);
}

void setup() {
  // Join I2C bus as slave
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  // Initialize input PINs
  pinMode(buttonControlPin, INPUT);
  for (byte i = 0; i < usbSwitchSelectorsNum; i += 1) {
    pinMode(usbSwitchSelectorsLed1Pin[i], INPUT);
  }

  // Initialize output PINs
  for (byte i = 0; i < controlOutputsNum; i += 1) {
    pinMode(controlOutputsPin[i], OUTPUT);
  }
  for (byte i = 0; i < usbSwitchSelectorsNum; i += 1) {
    pinMode(usbSwitchSelectorsButtonPin[i], OUTPUT);
  }

  // Read status of outputs
  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) {
    // EEprom is wrong version or was not programmed, write default values to the EEprom
    for (byte i = 0; i < controlOutputsNum; i += 1) {
      EEPROM.update(eepromControlOutputsStateAddr[i], 0);
    }
    for (byte i = 0; i < usbSwitchSelectorsNum; i += 1) {
      EEPROM.update(eepromUSBSwitchSelectorsLed1StateAddr[i], 0);
    }
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    for (byte i = 0; i < controlOutputsNum; i += 1) {
      digitalWrite(controlOutputsPin[i], EEPROM.read(eepromControlOutputsStateAddr[i]));
    }
    for (byte i = 0; i < usbSwitchSelectorsNum; i += 1) {
      if (digitalRead(usbSwitchSelectorsLed1Pin[i]) != EEPROM.read(eepromUSBSwitchSelectorsLed1StateAddr[i])) {
        digitalWrite(usbSwitchSelectorsButtonPin[i], HIGH);
        delay(300);
        digitalWrite(usbSwitchSelectorsButtonPin[i], LOW);
        strcpy(sendStatus, "1");
      }
    }
  }
}
