/*
* Copyright (C) 2019 Oleh Halytskyi
*
* This software may be modified and distributed under the terms
* of the Apache license. See the LICENSE file for details.
*
*/

#include <Wire.h>
#include <EEPROM.h>

#define SLAVE_ADDRESS 0x03
#define ADDR_VERSION 255         // Location of the software version in EEPROM
#define CURRENT_EEPROM_VERSION 2 // We are on revision 1 of the EEPROM storage structure (0xFF or 255 is never a valid value for version)

// Cluster outputs
#define num_cluster_outputs 9
// {<output pin>, <status>, <status_eeprom_addr>}
int cluster_outputs[num_cluster_outputs][3] = {
  {6, 0, 1},
  {2, 0, 2},
  {3, 0, 3},
  {10, 0, 4},
  {9, 0, 5},
  {8, 0, 6},
  {5, 0, 7},
  {4, 0, 8},
  {7, 0, 9}
};

// IP KVM outputs
#define num_ipkvm_outputs 5
// {<output pin>, <status>, <status_eeprom_addr>}
int ipkvm_outputs[num_ipkvm_outputs][3] = {
  {11, 0, 10},
  {12, 0, 11},
  {13, 0, 12},
  {14, 0, 13},
  {15, 0, 14}
};

const int buttonClusterPin = 16;
const int buttonIPKVMPin = 17;
int buttonTime = 2000; // 2 seconds
char sendStatus[9] = "";
String errorMsg = "WrongCMD";
int index = 0;
int rebootPin = -1;
int rebootTime = 5000; // 5 seconds
int rebootStatus = 0;


void reboot() {
  if (rebootPin != -1) {
    rebootStatus = 1;
    digitalWrite(rebootPin, LOW);
    delay(rebootTime);
    digitalWrite(rebootPin, HIGH);
    rebootPin = -1;
    rebootStatus = 0;
  }
}

void clearArray() {
  for (unsigned int i=0; i < sizeof(sendStatus);  ++i)
    sendStatus[i] = (char)0;
}

void receiveData(int byteCount) {
  clearArray();
  String response;
  int receiveByte = 0;
  char command[3] = {'\0', '\0', '\0'};
  while (Wire.available()) {
    command[receiveByte] = Wire.read();
    receiveByte++;
  }
  String type = String(command[0]);
  String cmd = String(command[1]);
  int value = command[2] - '0';

  if (type == "c") {
    if (cmd == "e") {
      if (value >= 1 && value <= num_cluster_outputs) {
        if (rebootPin != cluster_outputs[value - 1][0]) {
          digitalWrite(cluster_outputs[value - 1][0], HIGH);
          cluster_outputs[value - 1][1] = 1;
          EEPROM.update(cluster_outputs[value - 1][2], 1);
          response = String(digitalRead(cluster_outputs[value - 1][0]));
        } else {
          response = "2";
        }
        response.toCharArray(sendStatus, 2);
      } else {
        response = errorMsg;
        response.toCharArray(sendStatus, 9);
      }
    } else if (cmd == "d") {
      if (value >= 1 && value <= num_cluster_outputs) {
        if (rebootPin != cluster_outputs[value - 1][0]) {
          digitalWrite(cluster_outputs[value - 1][0], LOW);
          cluster_outputs[value - 1][1] = 0;
          EEPROM.update(cluster_outputs[value - 1][2], 0);
          response = String(digitalRead(cluster_outputs[value - 1][0]));
        } else {
          response = "2";
        }
        response.toCharArray(sendStatus, 2);
      } else {
        response = errorMsg;
        response.toCharArray(sendStatus, 9);
      }
    } else if (cmd == "v") {
      if (value >= 1 && value <= num_cluster_outputs) {
        response = String(digitalRead(cluster_outputs[value - 1][0]));
        response.toCharArray(sendStatus, 2);
      } else if (value == 0) {
        for (int i = 0; i < num_cluster_outputs; i += 1) {
          response += cluster_outputs[i][1];
        }
        response.toCharArray(sendStatus, num_cluster_outputs + 1);
      } else {
        response = errorMsg;
        response.toCharArray(sendStatus, 9);
      }
    } else if (cmd == "r") {
      if (value >= 1 && value <= num_cluster_outputs) {
        response = String(digitalRead(cluster_outputs[value - 1][0]));
        if (response == "1") {
          if (rebootStatus == 0) {
            rebootPin = cluster_outputs[value - 1][0];
          } else {
            response = "2";
          }
        }
        response.toCharArray(sendStatus, 2);
      } else {
        response = errorMsg;
        response.toCharArray(sendStatus, 9);
      }
    } else {
      response = errorMsg;
      response.toCharArray(sendStatus, 9);
    }
  } else if (type == "i") {
    if (cmd == "e") {
      if (value >= 1 && value <= num_ipkvm_outputs) {
        digitalWrite(ipkvm_outputs[value - 1][0], HIGH);
        ipkvm_outputs[value - 1][1] = 1;
        EEPROM.update(ipkvm_outputs[value - 1][2], 1);
        response = String(digitalRead(ipkvm_outputs[value - 1][0]));
        response.toCharArray(sendStatus, 2);
      } else {
        response = errorMsg;
        response.toCharArray(sendStatus, 9);
      }
    } else if (cmd == "d") {
      if (value >= 1 && value <= num_ipkvm_outputs) {
        digitalWrite(ipkvm_outputs[value - 1][0], LOW);
        ipkvm_outputs[value - 1][1] = 0;
        EEPROM.update(ipkvm_outputs[value - 1][2], 0);
        response = String(digitalRead(ipkvm_outputs[value - 1][0]));
        response.toCharArray(sendStatus, 2);
      } else {
        response = errorMsg;
        response.toCharArray(sendStatus, 9);
      }
    } else if (cmd == "v") {
      if (value >= 1 && value <= num_ipkvm_outputs) {
        response = String(digitalRead(ipkvm_outputs[value - 1][0]));
        response.toCharArray(sendStatus, 2);
      } else if (value == 0) {
        for (int i = 0; i < num_ipkvm_outputs; i += 1) {
          response += ipkvm_outputs[i][1];
        }
        response.toCharArray(sendStatus, num_ipkvm_outputs + 1);
      } else {
        response = errorMsg;
        response.toCharArray(sendStatus, 9);
      }
    } else {
      response = errorMsg;
      response.toCharArray(sendStatus, 9);
    }
  } else {
    response = errorMsg;
    response.toCharArray(sendStatus, 9);
  }
}

void sendData() {
  Wire.write(sendStatus[index]);
  ++index;
  if (index >= 9) {
    index = 0;
  }
}

void buttonsControl() {
  // Button for cluster control
  int buttonClusterStatus = digitalRead(buttonClusterPin);
  if (buttonClusterStatus == LOW) {
    for (int i = 0; i < num_cluster_outputs; i += 1) {
      if (digitalRead(cluster_outputs[i][0]) == 0) {
        digitalWrite(cluster_outputs[i][0], HIGH);
        cluster_outputs[i][1] = 1;
        EEPROM.update(cluster_outputs[i][2], 1);
        break;
      }
    }
    delay(buttonTime);
  }

  // Button for IP KVM control
  int buttonIPKVMStatus = digitalRead(buttonIPKVMPin);
  if (buttonIPKVMStatus == LOW) {
    for (int i = 0; i < num_ipkvm_outputs; i += 1) {
      if (digitalRead(ipkvm_outputs[i][0]) == 0) {
        digitalWrite(ipkvm_outputs[i][0], HIGH);
        ipkvm_outputs[i][1] = 1;
        EEPROM.update(ipkvm_outputs[i][2], 1);
        break;
      }
    }
    delay(buttonTime);
  }
}

void loop() {
  reboot();
  buttonsControl();
  delay(100);
}

void setup() {
  // Join I2C bus as slave
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  // Initialize input PINs
  pinMode(buttonClusterPin, INPUT);
  pinMode(buttonIPKVMPin, INPUT);

  // Initialize output PINs
  for (int i = 0; i < num_cluster_outputs; i += 1) {
    pinMode(cluster_outputs[i][0], OUTPUT);
  }
  for (int i = 0; i < num_ipkvm_outputs; i += 1) {
    pinMode(ipkvm_outputs[i][0], OUTPUT);
  }

  // Read status of outputs
  if (EEPROM.read(ADDR_VERSION) != CURRENT_EEPROM_VERSION) {
    // EEprom is wrong version or was not programmed, write default values to the EEprom
    for (int i = 0; i < num_cluster_outputs; i += 1) {
      EEPROM.update(cluster_outputs[i][2], cluster_outputs[i][1]);
    }
    for (int i = 0; i < num_ipkvm_outputs; i += 1) {
      EEPROM.update(ipkvm_outputs[i][2], ipkvm_outputs[i][1]);
    }
    EEPROM.update(ADDR_VERSION, CURRENT_EEPROM_VERSION); // update software version
  } else {
    for (int i = 0; i < num_cluster_outputs; i += 1) {
      cluster_outputs[i][1] = EEPROM.read(cluster_outputs[i][2]);
      digitalWrite(cluster_outputs[i][0], cluster_outputs[i][1]);
    }
    for (int i = 0; i < num_ipkvm_outputs; i += 1) {
      ipkvm_outputs[i][1] = EEPROM.read(ipkvm_outputs[i][2]);
      digitalWrite(ipkvm_outputs[i][0], ipkvm_outputs[i][1]);
    }
  }
}
