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
ina219Sensors = [
  {"name": "IP-KVM (Raspberry Pi)", "addr": 0x40, "shuntOhm": 0.1},
  {"name": "Master01 (SBC) + USB module", "addr": 0x41, "shuntOhm": 0.1},
  {"name": "USB switch selector #1", "addr": 0x42, "shuntOhm": 0.1},
  {"name": "Master02 (SBC) + USB module", "addr": 0x43, "shuntOhm": 0.1},
  {"name": "Master03 (SBC)", "addr": 0x44, "shuntOhm": 0.1},
  {"name": "Worker01 (mini PC)", "addr": 0x45, "shuntOhm": 0.05},
  {"name": "USB switch selector #2", "addr": 0x46, "shuntOhm": 0.1},
  {"name": "Worker02 (mini PC)", "addr": 0x47, "shuntOhm": 0.05},
  {"name": "Worker03 (mini PC)", "addr": 0x48, "shuntOhm": 0.05},
  {"name": "Ethernet switch", "addr": 0x49, "shuntOhm": 0.1},
  {"name": "External device #1", "addr": 0x4A, "shuntOhm": 0.1},
  {"name": "External device #2", "addr": 0x4B, "shuntOhm": 0.1},
  {"name": "External device #3", "addr": 0x4C, "shuntOhm": 0.1}
]

# Output statuses
statuses = {
  "0": "disabled",
  "1": "enabled",
  "2": "rebooting"
}


# This function converts a string to an array of bytes
def convert_string_to_bytes(src):
  converted = []
  for b in src:
    converted.append(ord(b))
  return converted

def get_status(command, value):
  status = ""
  bus = SMBus(i2cBusAddr)
  BytesToSend = convert_string_to_bytes(value)
  bus.write_i2c_block_data(arduinoSlaveAddress, ord(command), BytesToSend)
  time.sleep(0.001)
  for i in range (0, 11):
    statusChr = chr(bus.read_byte(arduinoSlaveAddress))
    if statusChr != ";":
      status += statusChr
    else:
      break
  bus.close()
  return status

def ps_control(command, value):
  response = get_status(command, value)
  if response == "err":
    print("Wrong command!\n")
    help()
    sys.exit(1)
  if value[0] == "s":
    answerMid = "-"
  else:
    answerMid = "has been"
  answer_end = statuses[response[0]]

  if command == "c":
    if value[1].isdigit():
      valueInt = int(value[1])
      if len(value) == 3:
        if value[2].isdigit():
          valueInt = int(value[1]+value[2])
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
        print(controlOutputs[i], answerMid, statuses[response[i]])
    else:
      output = controlOutputs[valueInt - 1]
      if value[0] == "r":
        if response[0] != "1":
          answerMid = "is"
        else:
          answerMid = "- started"
          answer_end = "reboot process"
      if response[0] == "2":
        answerMid = "- some output still in reboot process"
      print(output, answerMid, answer_end)
  elif command == "u":
    if value[1].isdigit():
      valueInt = int(value[1])
      if len(value) == 3:
        if value[2].isdigit():
          valueInt2 = int(value[2])
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
        print(usbSwitchSelectors[i], "output #1", answerMid, statuses[response[i]])
    else:
      usbSwitchSelector = usbSwitchSelectors[valueInt - 1]
      if value[0] == "e":
        answer_end = "enabled"
        if response[0] == "0":
          answerMid = "already"
        time.sleep(0.3)
        response = get_status(command, "s" + value[1])
        if response[0] == "2":
          answerMid = "-"
          answer_end = "something wrong, not enabled"
      else:
        valueInt2 = 1
      print(usbSwitchSelector, "output #" + str(valueInt2), answerMid, answer_end)

def read_sensor(ina219Sensor, ina219Address, shuntOhm):
    ina = INA219(shuntOhm, busnum=i2cBusAddr, address=ina219Address)
    try:
      ina.configure()
    except:
      print("%s:" % ina219Sensor)
      print("Can't get data from sensor!\n")
      return

    print("%s:" % ina219Sensor)
    print("Voltage: %.3f V" % ina.voltage())
    try:
      print("Current: %.3f mA" % ina.current())
      print("Power: %.3f mW\n" % ina.power())
      #print("Shunt voltage: %.3f mV" % ina.shunt_voltage())
    except DeviceRangeError as e:
      # Current out of device range with specified shunt resister
      print(e)

def read_sensors(command):
  try:
    cmdInt = int(command)
  except:
    print("Wrong command!")
    help()
    sys.exit(1)

  if cmdInt >= 1 and cmdInt <= len(ina219Sensors):
    read_sensor(ina219Sensors[cmdInt - 1]["name"], ina219Sensors[cmdInt - 1]["addr"], ina219Sensors[cmdInt - 1]["shuntOhm"])
  elif cmdInt == 0:
    for i in range(0, len(ina219Sensors)):
      read_sensor(ina219Sensors[i]["name"], ina219Sensors[i]["addr"], ina219Sensors[i]["shuntOhm"])
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
    print("    s%s - %s" % (i + 1, ina219Sensors[i]["name"]))
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
      if len(sys.argv[1]) <= 3:
        read_sensors(sys.argv[1][1:])
      else:
        print("Wrong command!")
        help()
        sys.exit(1)
    elif sys.argv[1][0] == "c" or sys.argv[1][0] == "u":
      ps_control(sys.argv[1][:1], sys.argv[1][1:])
    else:
      print("Wrong command!")
      help()
      sys.exit(1)

if __name__== "__main__":
  main()
