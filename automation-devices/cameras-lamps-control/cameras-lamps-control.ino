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

const byte deviceID = 25;
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
const unsigned long autoPushInterval = 60000; // auto-push interval for each sensor
const unsigned int minAutoPushInterval = 1000; // interval between push data to master
unsigned long prevMillisAutoPush = 0;
unsigned long lastUpdateMillisAutoPush = 0;

// Lamps
const byte lampNum = 2;
// {"command"}
const char *const lampCmd[lampNum] = {"L-1", "L-2"};
// {"pin"}
const byte lampPin[lampNum] = {3, 6};
// {"value"}
byte lampValue[lampNum] = {0, 0};
// {"enabled"}
byte lampEnabled[lampNum] = {0, 0};
// {"automode"}
byte lampAutoMode[lampNum] = {0, 0};
// {"automode max brightness"}
byte lampAutoModeMaxBrightness[lampNum] = {100, 100};
// {"automode motion sensor index"}
byte lampAutoModeMotionSensorIndex[lampNum] = {0, 0};
// {"automode fade speed"}
byte lampAutoModeFadeSpeed[lampNum] = {0, 0};
// {"automode fade value"}
int lampAutoModeFadeValue[lampNum] = {0, 0};
// {"num of light sensor for lamp automode"}
byte lampLightSensorAutoMode[lampNum] = {1, 2};
// {"lamp blinking"}
byte lampBlinking[lampNum] = {0, 0};
// {"lamp strobe Last Update"}
unsigned long lampBlinkLastUpdate[lampNum] = {0, 0};
// {"EEProm automode"}
const byte eepromLampAutoModeAddr[lampNum] = {1, 2};
// {"EEProm automode max brightness"}
const byte eepromLampAutoModeMaxBrightnessAddr[lampNum] = {3, 4};
// {"EEProm automode fade speed"}
const byte eepromLampAutoModeFadeSpeedAddr[lampNum] = {5, 6};
// {"EEProm automode light sensors"}
const byte eepromLampLightSensorAutoModeAddr[lampNum] = {7, 8};

// Motion sensors
const byte motionSensorNum = 3;
// {"command"}
const char *const motionSensorCmd[motionSensorNum] = {"S-m-1", "S-m-2", "S-m-3"};
// {"pin"}
const byte motionSensorPin[motionSensorNum] = {14, 15, 16};
// {"value"}
byte motionSensorValue[motionSensorNum] = {0, 0, 0};
// {"autopush"}
byte motionSensorAutoPush[motionSensorNum] = {0, 0, 0};
// {"lamp blink mode for lamp automode"}
byte motionSensorLampBlinkForAutoMode[motionSensorNum] = {0, 0, 0};
// {"num of lamp for lamp automode"}
byte motionSensorLampAutoMode[motionSensorNum] = {1, 2, 2};
// {"time"}
byte motionSensorTime[motionSensorNum] = {0, 0, 0};
// {"last update"}
unsigned long motionSensorLastUpdate[motionSensorNum] = {0, 0, 0};
// {"EEProm autopush"}
const byte eepromMotionSensorAutoPushAddr[motionSensorNum] = {9, 10, 11};
// {"EEProm amp blink for lamp automode"}
const byte eepromMotionSensorLampBlinkForAutoModeAddr[motionSensorNum] = {12, 13, 14};
// {"EEProm automode motion sensors"}
const byte eepromMotionSensorLampAutoModeAddr[motionSensorNum] = {15, 16, 17};
// {"EEProm time"}
const byte eepromMotionSensorTimeAddr[motionSensorNum] = {18, 19, 20};

// Light sensors
const byte lightSensorNum = 3;
// {"command"}
const char *const lightSensorCmd[lightSensorNum] = {"S-l-1", "S-l-2", "S-l-3"};
// {"pin"}
const byte lightSensorPin[lightSensorNum] = {A3, A4, A5};
// {"autopush"}
byte lightSensorAutoPush[lightSensorNum] = {0, 0, 0};
// {"autopush last value"}
byte lightSensorAutoPushLastValue[lightSensorNum] = {0, 0, 0};
// {"last update for autopush"}
unsigned long lightSensorAutoPushLU[lightSensorNum] = {0, 0, 0};
// {"brightness limit"}
byte lightSensorBrightnessLimit[lightSensorNum] = {0, 0, 0};
// {"EEProm autopush"}
const byte eepromLightSensorAutoPushAddr[lightSensorNum] = {21, 22, 23};
// {"EEProm brightness limit"}
const byte eepromLightSensorBrightnessLimitAddr[lightSensorNum] = {24, 25, 26};

// Cameras
const byte cameraNum = 2;
// {"command"}
const char *const cameraCmd[cameraNum] = {"C-1", "C-2"};
// {"pin"}
const byte cameraPin[cameraNum] = {5, 9};
// {"EEProm state"}
const byte eepromCameraStateAddr[cameraNum] = {27, 28};

// Alarm
const byte alarmPin = 13;
byte alarmState = 0;
unsigned long alarmStateLastUpdate = 0;

// Power supply from battery
byte batteryState = 0;


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
  // Lamps
  for (byte i = 0; i < lampNum; i += 1) {
    if (strcmp(command, lampCmd[i]) == 0) {
      if (valueLength == 0) {
        itoa(lampValue[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int >= 0 and value_int <= 100) {
          if (value_int == 0) {
            digitalWrite(lampPin[i], LOW);
          } else if (value_int == 100) {
            digitalWrite(lampPin[i], HIGH);
          } else {
            byte lightPercent = map(value_int, 0, 100, 0, 255);
            analogWrite(lampPin[i], lightPercent);
          }
          lampValue[i] = value_int;
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, lampCmd[i]);
    strcat(tmpBuf, "-a");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(lampAutoMode[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1 or value_int == 2) {
          lampAutoMode[i] = value_int;
          EEPROM.update(eepromLampAutoModeAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, lampCmd[i]);
    strcat(tmpBuf, "-a-b");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(lampAutoModeMaxBrightness[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int >= 0 and value_int <= 100) {
          lampAutoModeMaxBrightness[i] = value_int;
          EEPROM.update(eepromLampAutoModeMaxBrightnessAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, lampCmd[i]);
    strcat(tmpBuf, "-a-f");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(lampAutoModeFadeSpeed[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int >= 0 and value_int <= 9) {
          lampAutoModeFadeSpeed[i] = value_int;
          EEPROM.update(eepromLampAutoModeFadeSpeedAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, lampCmd[i]);
    strcat(tmpBuf, "-a-l");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(lampLightSensorAutoMode[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int >= 1 and value_int <= 3) {
          lampLightSensorAutoMode[i] = value_int;
          EEPROM.update(eepromLampLightSensorAutoModeAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, lampCmd[i]);
    strcat(tmpBuf, "-b");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(lampBlinking[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1 or value_int == 2) {
          if (value_int == 0) {
            digitalWrite(lampPin[i], LOW);
          }
          lampBlinking[i] = value_int;
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  // Motion sensors
  for (byte i = 0; i < motionSensorNum; i += 1) {
    if (strcmp(command, motionSensorCmd[i]) == 0) {
      if (valueLength == 0) {
        itoa(digitalRead(motionSensorPin[i]), tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
    strcpy(tmpBuf, motionSensorCmd[i]);
    strcat(tmpBuf, "-a");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(motionSensorAutoPush[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1) {
          motionSensorAutoPush[i] = value_int;
          EEPROM.update(eepromMotionSensorAutoPushAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, motionSensorCmd[i]);
    strcat(tmpBuf, "-b");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(motionSensorLampBlinkForAutoMode[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1 or value_int == 2) {
          motionSensorLampBlinkForAutoMode[i] = value_int;
          EEPROM.update(eepromMotionSensorLampBlinkForAutoModeAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, motionSensorCmd[i]);
    strcat(tmpBuf, "-l");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(motionSensorLampAutoMode[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int >= 1 and value_int <= 3) {
          byte lampIndex = motionSensorLampAutoMode[i] - 1;
          digitalWrite(lampPin[lampIndex], 0);
          lampEnabled[lampIndex] = 0;
          lampValue[lampIndex] = 0;
          motionSensorLampAutoMode[i] = value_int;
          EEPROM.update(eepromMotionSensorLampAutoModeAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, motionSensorCmd[i]);
    strcat(tmpBuf, "-t");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(motionSensorTime[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int >= 0 and value_int <= 180) {
          motionSensorTime[i] = value_int;
          EEPROM.update(eepromMotionSensorTimeAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  // Light sensor
  for (byte i = 0; i < lightSensorNum; i += 1) {
    if (strcmp(command, lightSensorCmd[i]) == 0) {
      if (valueLength == 0) {
        byte brightnessPercent = map(analogRead(lightSensorPin[i]), 0, 1023, 0, 100);
        itoa(brightnessPercent, tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
    strcpy(tmpBuf, lightSensorCmd[i]);
    strcat(tmpBuf, "-a");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(lightSensorAutoPush[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int >= 0 and value_int <= 6) {
          lightSensorAutoPush[i] = value_int;
          lightSensorAutoPushLastValue[i] = 0;
          EEPROM.update(eepromLightSensorAutoPushAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
    strcpy(tmpBuf, lightSensorCmd[i]);
    strcat(tmpBuf, "-b");
    if (strcmp(command, tmpBuf) == 0) {
      if (valueLength == 0) {
        itoa(lightSensorBrightnessLimit[i], tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int >= 0 and value_int <= 100) {
          lightSensorBrightnessLimit[i] = value_int;
          EEPROM.update(eepromLightSensorBrightnessLimitAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  // Cameras
  for (byte i = 0; i < cameraNum; i += 1) {
    if (strcmp(command, cameraCmd[i]) == 0) {
      if (valueLength == 0) {
        itoa(digitalRead(cameraPin[i]), tmpBuf, 10);
        busA_reply(request, tmpBuf);
        return;
      } else {
        if (value_int == 0 or value_int == 1) {
          digitalWrite(cameraPin[i], value_int);
          EEPROM.update(eepromCameraStateAddr[i], value_int);
          busA_reply(request, goodCommandReply);
          return;
        } else {
          busA_reply(request, badCommandReply);
          return;
        }
      }
    }
  }
  // Alarm
  if (strcmp(command, "A") == 0) {
    if (valueLength == 0) {
      itoa(alarmState, tmpBuf, 10);
      busA_reply(request, tmpBuf);
      return;
    } else {
      if (value_int == 0) {
        digitalWrite(alarmPin, LOW);
        alarmState = value_int;
        busA_reply(request, goodCommandReply);
        return;
      } else if (value_int >= 1 and value_int <= 120) {
        digitalWrite(alarmPin, HIGH);
        alarmState = value_int;
        alarmStateLastUpdate = millis();
        busA_reply(request, goodCommandReply);
        return;
      } else {
        busA_reply(request, badCommandReply);
        return;
      }
    }
  }
  // Power supply from battery
  if (strcmp(command, "B") == 0) {
    if (valueLength == 0) {
      itoa(batteryState, tmpBuf, 10);
      busA_reply(request, tmpBuf);
      return;
    } else {
      if (value_int == 0 or value_int == 1) {
        batteryState = value_int;
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
    for (byte i = 0; i < lightSensorNum; i += 1) {
      if (lightSensorAutoPush[i] >= 1 and lightSensorAutoPush[i] <= 6) {
        byte lightSensorAutoPushValue = lightSensorAutoPush[i];
        if (lightSensorAutoPush[i] == 6) {
          lightSensorAutoPushValue = 1;
        }
        if (curMillis - lightSensorAutoPushLU[i] >= lightSensorAutoPushValue * autoPushInterval and curMillis - lastUpdateMillisAutoPush >= minAutoPushInterval) {
          lightSensorAutoPushLU[i] = curMillis;
          lastUpdateMillisAutoPush = curMillis;
          byte brightnessPercent = map(analogRead(lightSensorPin[i]), 0, 1023, 0, 100);
          if (lightSensorAutoPush[i] == 6) {
            if (brightnessPercent != lightSensorAutoPushLastValue[i]) {
              lightSensorAutoPushLastValue[i] = brightnessPercent;
            } else {
              break;
            }
          }
          char tmpBuf[6];
          itoa(brightnessPercent, tmpBuf, 10);
          busB_send(lightSensorCmd[i], tmpBuf);
        }
      }
    }
    prevMillisAutoPush = curMillis;
  }
}

void motion_detect() {
  unsigned long curMillis = millis(); // time now in ms
  for (byte i = 0; i < motionSensorNum; i += 1) {
    if (motionSensorTime[i] != 0) {
      byte sensorStatus = digitalRead(motionSensorPin[i]);
      if (sensorStatus == 1 and motionSensorValue[i] == 1) {
        motionSensorLastUpdate[i] = curMillis;
      }
      unsigned long timeLimit = motionSensorTime[i] * 1000;
      if (batteryState == 1) {
        timeLimit = 5000;
      }
      if (timeLimit <= curMillis - motionSensorLastUpdate[i]) {
        if (sensorStatus != motionSensorValue[i]) {
          motionSensorValue[i] = sensorStatus;
          if (motionSensorValue[i] == 1) {
            motionSensorLastUpdate[i] = curMillis;
          }
          if (motionSensorAutoPush[i] == 1) {
            char tmpBuf[2];
            itoa(motionSensorValue[i], tmpBuf, 10);
            busB_send(motionSensorCmd[i], tmpBuf);
          }
        }
      }
    }
  }
}

void lamp_automode_control() {
  for (byte i = 0; i < motionSensorNum; i += 1) {
    byte lampIndex = motionSensorLampAutoMode[i] - 1;
    if (lampAutoMode[lampIndex] == 1 or lampAutoMode[lampIndex] == 2) {
      byte maxBrightnessPercent = lampAutoModeMaxBrightness[lampIndex];
      if (batteryState == 1) {
        maxBrightnessPercent = 50;
      }
      byte maxBrightness = map(maxBrightnessPercent, 0, 100, 0, 255);
      byte lampAutoModeFadeSpeedValue = lampAutoModeFadeSpeed[lampIndex];
      if (lampAutoModeFadeSpeed[lampIndex] != 0) {
        lampAutoModeFadeSpeedValue = (10 - lampAutoModeFadeSpeed[lampIndex]) * 2;
      }
      if ((motionSensorValue[i] == 1 and lampEnabled[lampIndex] == 0) or (lampAutoModeFadeValue[lampIndex] != 0)) {
        lampAutoModeMotionSensorIndex[lampIndex] = i;
        byte brightnessPercent = map(analogRead(lightSensorPin[lampLightSensorAutoMode[lampIndex] - 1]), 0, 1023, 0, 100);
        if (brightnessPercent <= lightSensorBrightnessLimit[lampLightSensorAutoMode[lampIndex] - 1] or lampAutoModeFadeValue[lampIndex] != 0) {
          if (motionSensorLampBlinkForAutoMode[i] != 0) {
            lampBlinking[lampIndex] = motionSensorLampBlinkForAutoMode[i];
            maxBrightnessPercent = 100 + lampBlinking[lampIndex];
          } else {
            if (lampAutoModeFadeSpeed[lampIndex] != 0) {
              for (int fadeValue = lampAutoModeFadeValue[lampIndex]; fadeValue < maxBrightness; fadeValue += lampAutoModeFadeSpeedValue) {
                analogWrite(lampPin[lampIndex], fadeValue);
                busA.receive(receiveTimeBusA);
                delay(10);
              }
            } else if (maxBrightnessPercent != 100) {
              analogWrite(lampPin[lampIndex], maxBrightness);
            }
            if (maxBrightnessPercent == 100) {
              digitalWrite(lampPin[lampIndex], 1);
            }
          }
          lampEnabled[lampIndex] = 1;
          lampAutoModeFadeValue[lampIndex] = 0;
          lampValue[lampIndex] = maxBrightnessPercent;
          if (lampAutoMode[lampIndex] == 2) {
            char tmpBuf[4];
            itoa(maxBrightnessPercent, tmpBuf, 10);
            busB_send(lampCmd[lampIndex], tmpBuf);
          }
        }
      } else if (lampAutoModeMotionSensorIndex[lampIndex] == i and motionSensorValue[i] == 0 and lampEnabled[lampIndex] == 1) {
        if (motionSensorLampBlinkForAutoMode[i] != 0) {
          lampBlinking[lampIndex] = 0;
        } else {
          if (lampAutoModeFadeSpeed[lampIndex] != 0) {
            for (int fadeValue = maxBrightness; fadeValue >= 0; fadeValue -= lampAutoModeFadeSpeedValue) {
              analogWrite(lampPin[lampIndex], fadeValue);
              busA.receive(receiveTimeBusA);
              if (digitalRead(motionSensorPin[i]) == 1) {
                motionSensorValue[i] = 1;
                lampAutoModeFadeValue[lampIndex] = fadeValue;
                if (motionSensorAutoPush[i] == 1) {
                  char tmpBuf[2];
                  itoa(motionSensorValue[i], tmpBuf, 10);
                  busB_send(motionSensorCmd[i], tmpBuf);
                }
                break;
              }
              delay(10);
            }
          }
        }
        if (lampAutoModeFadeValue[lampIndex] == 0) {
          digitalWrite(lampPin[lampIndex], 0);
          lampEnabled[lampIndex] = 0;
          lampValue[lampIndex] = 0;
          if (lampAutoMode[lampIndex] == 2) {
            busB_send(lampCmd[lampIndex], "0");
          }
        }
      }
    }
  }
}

void alarm() {
  if (alarmState != 0) {
    unsigned long curMillis = millis(); // time now in ms
    if (curMillis - alarmStateLastUpdate >= alarmState * 1000) {
      digitalWrite(alarmPin, LOW);
      alarmState = 0;
    }
  }
}

void lamp_blinking() {
  unsigned long curMillis = millis(); // time now in ms
  for (byte i = 0; i < lampNum; i += 1) {
    if (lampBlinking[i] == 1 or lampBlinking[i] == 2) {
      unsigned int delayBlink = 50;
      if (lampBlinking[i] == 2) {
        delayBlink = 1000;
      }
      if (digitalRead(lampPin[i]) == LOW) {
        if (curMillis - lampBlinkLastUpdate[i] >= delayBlink) {
          digitalWrite(lampPin[i], HIGH);
          lampBlinkLastUpdate[i] = curMillis;
        }
      }
      if (digitalRead(lampPin[i]) == HIGH) {
        if (curMillis - lampBlinkLastUpdate[i] >= delayBlink) {
          digitalWrite(lampPin[i], LOW);
          lampBlinkLastUpdate[i] = curMillis;
        }
      }
    }
  }
}


void loop() {
  busA.receive(receiveTimeBusA);
  autopush();
  motion_detect();
  alarm();
  lamp_automode_control();
  lamp_blinking();
}

void setup() {
  // Set lamps state
  for (byte i = 0; i < lampNum; i += 1) {
    pinMode(lampPin[i], OUTPUT);
    digitalWrite(lampPin[i], LOW);
  }
  // Cameras
  for (byte i = 0; i < cameraNum; i += 1) {
    pinMode(cameraPin[i], OUTPUT);
  }
  // Alarm
  pinMode(alarmPin, OUTPUT);
  digitalWrite(alarmPin, LOW);

  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) { //EEprom is wrong version or was not programmed, write default values to the EEprom
    for (byte i = 0; i < lampNum; i += 1) {
      EEPROM.update(eepromLampAutoModeAddr[i], lampAutoMode[i]);
      EEPROM.update(eepromLampAutoModeMaxBrightnessAddr[i], lampAutoModeMaxBrightness[i]);
      EEPROM.update(eepromMotionSensorLampAutoModeAddr[i], motionSensorLampAutoMode[i]);
      EEPROM.update(eepromLampAutoModeFadeSpeedAddr[i], lampAutoModeFadeSpeed[i]);
      EEPROM.update(eepromLampLightSensorAutoModeAddr[i], lampLightSensorAutoMode[i]);
    }
    for (byte i = 0; i < motionSensorNum; i += 1) {
      EEPROM.update(eepromMotionSensorAutoPushAddr[i], motionSensorAutoPush[i]);
      EEPROM.update(eepromMotionSensorLampBlinkForAutoModeAddr[i], motionSensorLampBlinkForAutoMode[i]);
      EEPROM.update(eepromMotionSensorTimeAddr[i], motionSensorTime[i]);
    }
    for (byte i = 0; i < lightSensorNum; i += 1) {
      EEPROM.update(eepromLightSensorAutoPushAddr[i], lightSensorAutoPush[i]);
      EEPROM.update(eepromLightSensorBrightnessLimitAddr[i], lightSensorBrightnessLimit[i]);
    }
    for (byte i = 0; i < cameraNum; i += 1) {
      digitalWrite(cameraPin[i], 0);
      EEPROM.update(eepromCameraStateAddr[i], 0);
    }
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    for (byte i = 0; i < lampNum; i += 1) {
      lampAutoMode[i] = EEPROM.read(eepromLampAutoModeAddr[i]);
      lampAutoModeMaxBrightness[i] = EEPROM.read(eepromLampAutoModeMaxBrightnessAddr[i]);
      motionSensorLampAutoMode[i] = EEPROM.read(eepromMotionSensorLampAutoModeAddr[i]);
      lampAutoModeFadeSpeed[i] = EEPROM.read(eepromLampAutoModeFadeSpeedAddr[i]);
      lampLightSensorAutoMode[i] = EEPROM.read(eepromLampLightSensorAutoModeAddr[i]);
    }
    for (byte i = 0; i < motionSensorNum; i += 1) {
      motionSensorAutoPush[i] = EEPROM.read(eepromMotionSensorAutoPushAddr[i]);
      motionSensorLampBlinkForAutoMode[i] = EEPROM.read(eepromMotionSensorLampBlinkForAutoModeAddr[i]);
      motionSensorTime[i] = EEPROM.read(eepromMotionSensorTimeAddr[i]);
    }
    for (byte i = 0; i < lightSensorNum; i += 1) {
      lightSensorAutoPush[i] = EEPROM.read(eepromLightSensorAutoPushAddr[i]);
      lightSensorBrightnessLimit[i] = EEPROM.read(eepromLightSensorBrightnessLimitAddr[i]);
    }
    for (byte i = 0; i < cameraNum; i += 1) {
      digitalWrite(cameraPin[i], EEPROM.read(eepromCameraStateAddr[i]));
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