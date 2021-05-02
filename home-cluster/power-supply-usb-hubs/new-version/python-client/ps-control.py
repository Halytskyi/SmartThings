#!/usr/bin/env python3
#
# Copyright (C) 2021 Oleh Halytskyi
#
# This software may be modified and distributed under the terms
# of the Apache license. See the LICENSE file for details.

import sys
import struct
import time
from smbus import SMBus
from ina219 import INA219
from ina219 import DeviceRangeError

i2cBusAddr = 1             # indicates /dev/i2c-1
arduinoSlaveAddress = 0x03 # Arduino bus address

# Control outputs
controlOutputs = [
  "IP-KVM (Raspberry Pi)",
  "Master01 (SBC) + USB module",
  "Master02 (SBC) + USB module",
  "Master03 (SBC)",
  "Worker01 (mini PC)",
  "Worker02 (mini PC)",
  "Worker03 (mini PC)",
  "Ethernet switch",
  "External device #1",
  "External device #2",
  "External device #3"
]

# USB switch selectors
usbSwitchSelectors = [
  "USB switch selector #1",
  "USB switch selector #2"
]

# Params for INA219 sensors
shunt_ohms = 0.1
ina219Sensors = [
  {"IP-KVM (Raspberry Pi)": 0x40},
  {"Master01 (SBC) + USB module": 0x41},
  {"USB switch selector #1": 0x42},
  {"Master02 (SBC) + USB module": 0x43},
  {"Master03 (SBC)": 0x44},
  {"Worker01 (mini PC)": 0x45},
  {"USB switch selector #2": 0x46},
  {"Worker02 (mini PC)": 0x47},
  {"Worker03 (mini PC)": 0x48},
  {"Ethernet switch": 0x49},
  {"External device #1": 0x4A},
  {"External device #2": 0x4B},
  {"External device #3": 0x4C}
]

# Output statuses
statuses = {
  "0": "disabled",
  "1": "enabled",
  "2": "rebooting"
}


# This function converts a string to an array of bytes
def ConvertStringToBytes(src):
  converted = []
  for b in src:
    converted.append(ord(b))
  return converted

def getStatus(command, value):
  status = ""
  bus = SMBus(i2cBusAddr)
  BytesToSend = ConvertStringToBytes(value)
  bus.write_i2c_block_data(arduinoSlaveAddress, ord(command), BytesToSend)
  time.sleep(0.001)
  for i in range (0, 11):
    status_chr = chr(bus.read_byte(arduinoSlaveAddress))
    if status_chr != ";":
      status += status_chr
    else:
      break
  bus.close()
  return status

def psControl(command, value):
  response = getStatus(command, value)
  if response == "err":
    print("Wrong command!\n")
    help()
    sys.exit(1)
  if value[0] == "s":
    answer_mid = "-"
  else:
    answer_mid = "has been"
  answer_end = statuses[response[0]]

  if command == "c":
    if value[1].isdigit():
      value_int = int(value[1])
      if len(value) == 3:
        if value[2].isdigit():
          value_int = int(value[1]+value[2])
        else:
          print("Wrong command!\n")
          help()
          sys.exit(1)
    else:
      print("Wrong command!\n")
      help()
      sys.exit(1)
    if value == "s0":
      for i in range(0, len(controlOutputs)):
        print(controlOutputs[i], answer_mid, statuses[response[i]])
    else:
      output = controlOutputs[value_int - 1]
      if value[0] == "r":
        if response[0] != "1":
          answer_mid = "is"
        else:
          answer_mid = "- started"
          answer_end = "reboot process"
      if response[0] == "2":
        answer_mid = "- some output still in reboot process"
      print(output, answer_mid, answer_end)
  elif command == "u":
    if value[1].isdigit():
      value_int = int(value[1])
      if len(value) == 3:
        if value[2].isdigit():
          value_int2 = int(value[2])
        else:
          print("Wrong command!\n")
          help()
          sys.exit(1)
    else:
      print("Wrong command!\n")
      help()
      sys.exit(1)
    if value == "s0":
      for i in range(0, len(usbSwitchSelectors)):
        print(usbSwitchSelectors[i], "output #1", answer_mid, statuses[response[i]])
    else:
      usbSwitchSelector = usbSwitchSelectors[value_int - 1]
      if value[0] == "e":
        answer_end = "enabled"
        if response[0] == "0":
          answer_mid = "already"
        time.sleep(0.3)
        response = getStatus(command, "s" + value[1])
        if response[0] == "2":
          answer_mid = "-"
          answer_end = "something wrong, not enabled"
      else:
        value_int2 = 1
      print(usbSwitchSelector, "output #" + str(value_int2), answer_mid, answer_end)

def readSensor(ina219_sensor, ina219_address):
    ina = INA219(shunt_ohms, address=ina219_address)
    ina.configure()

    print("%s:" % ina219_sensor)
    print("Voltage: %.3f V" % ina.voltage())
    try:
        print("Current: %.3f mA" % ina.current())
        print("Power: %.3f mW\n" % ina.power())
        #print("Shunt voltage: %.3f mV" % ina.shunt_voltage())
    except DeviceRangeError as e:
        # Current out of device range with specified shunt resister
        print(e)

def readSensors(command):
  try:
    cmd_int = int(command)
  except:
    print("Wrong command!")
    help()
    sys.exit(1)

  if cmd_int >= 1 and cmd_int <= len(ina219Sensors):
    for ina219_sensor, ina219_address in ina219Sensors[cmd_int - 1].items():
      readSensor(ina219_sensor, ina219_address)
  elif cmd_int == 0:
    for i in range(0, len(ina219Sensors)):
      for ina219_sensor, ina219_address in ina219Sensors[i].items():
        readSensor(ina219_sensor, ina219_address)
  else:
    print("Wrong command!")
    help()

def help():
  print("Usage:", sys.argv[0], "[command]\n")
  print("Commands:")
  print("  [Control outputs]")
  print("    cs0 - get status of all controlled outputs")
  for i in range(0, len(controlOutputs)):
    print("    c[e|d|r|s]%s - %s" % (i + 1, controlOutputs[i]))
  print("  [USB switch selectors]")
  print("    us0 - get status of all USB switch selectors for output #1")
  for i in range(0, len(usbSwitchSelectors)):
    print("    us%s - get status of %s for output #1" % (i + 1, usbSwitchSelectors[i]))
  for i in range(0, len(usbSwitchSelectors)):
    for j in range(1, 3):
      print("    ue%s%s - enable output #%s for %s" % (i + 1, j, j, usbSwitchSelectors[i]))
  print("  [Voltage/current/power sensors]")
  print("    s0 - read all sensors")
  for i in range(0, len(ina219Sensors)):
    for ina219_sensor, _ in ina219Sensors[i].items():
      print("    s%s - %s" % (i + 1, ina219_sensor))
  print("")
  print("Description:")
  print("  e - enable")
  print("  d - disable")
  print("  r - reboot")
  print("  s - get status")
  print("")

def main():
  if len(sys.argv) == 1 or len(sys.argv[1]) > 4:
    help()
    sys.exit(1)
  else:
    if sys.argv[1][0] == "s":
      if len(sys.argv[1]) == 2:
        readSensors(sys.argv[1][1])
      else:
        print("Wrong command!")
        help()
        sys.exit(1)
    elif sys.argv[1][0] == "c" or sys.argv[1][0] == "u":
      psControl(sys.argv[1][:1], sys.argv[1][1:])
    else:
      print("Wrong command!")
      help()
      sys.exit(1)

if __name__== "__main__":
  main()
