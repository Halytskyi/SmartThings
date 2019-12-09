#!/usr/bin/env python3

import sys
import struct
import time
from smbus import SMBus
from ina219 import INA219
from ina219 import DeviceRangeError

bus = SMBus(1)             # indicates /dev/ic2-1
arduinoSlaveAddress = 0x03 # Arduino bus address

# Cluster devices
cluster_devices = [
  "Raspberry Pi",
  "USB module #1",
  "USB module #2, for serial console's (IP KVM)",
  "Master01 (mini PC)",
  "Master02 (Rock64)",
  "Master03 (Rock64)",
  "Worker01 (mini PC)",
  "Worker02 (Rock64)",
  "Worker03 (Rock64)"
]

# IP KVM devices
ipkvm_devices = [
  "IP KVM for Master01",
  "IP KVM for Worker01",
  "D13",
  "A0",
  "A1"
]

# Params for INA219 sensors
shunt_ohms = 0.1
ina219_sensors = [
  {"Raspberry Pi": 0x40},
  {"Master01 (mini PC)": 0x46},
  {"Master02 (Rock64)": 0x44},
  {"Master03 (Rock64)": 0x43},
  {"Worker01 (mini PC)": 0x45},
  {"Worker02 (Rock64)": 0x42},
  {"Worker03 (Rock64)": 0x41}
]

# Device statuses
statuses = {
  "0": "disabled",
  "1": "enabled",
  "2": "reboots"
}


# This function converts a string to an array of bytes
def ConvertStringToBytes(src):
  converted = []
  for b in src:
    converted.append(ord(b))
  return converted

def getStatus():
  status = ""
  for i in range (0, 9):
    status += chr(bus.read_byte(arduinoSlaveAddress))
  return status

def psControl(command, value):
  BytesToSend = ConvertStringToBytes(value)
  bus.write_i2c_block_data(arduinoSlaveAddress, ord(command), BytesToSend)
  time.sleep(0.001)
  response = getStatus()
  if response[0] == "W":
    print("Wrong command!")
    help()
    sys.exit(1)
  if value[0] == "v":
    answer_mid = "is"
  else:
    answer_mid = "has been"
  answer_end = statuses[response[0]]

  if command == "c":
    if value == "v0":
      for i in range(0, len(cluster_devices)):
        print(cluster_devices[i], answer_mid, statuses[response[i]])
    else:
      device = cluster_devices[int(value[1]) - 1]
      if value[0] == "r":
        if response[0] != "1":
          answer_mid = "is"
        else:
          answer_end = "rebooted"
      if response[0] == "2":
        answer_mid = "still"
      print(device, answer_mid, answer_end)
  elif command == "i":
    if value == "v0":
      for i in range(0, len(ipkvm_devices)):
        print(ipkvm_devices[i], answer_mid, statuses[response[i]])
    else:
      device = ipkvm_devices[int(value[1]) - 1]
      print(device, answer_mid, answer_end)

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

  if cmd_int >= 1 and cmd_int <= len(ina219_sensors):
    for ina219_sensor, ina219_address in ina219_sensors[cmd_int - 1].items():
      readSensor(ina219_sensor, ina219_address)
  elif cmd_int == 0:
    for i in range(0, len(ina219_sensors)):
      for ina219_sensor, ina219_address in ina219_sensors[i].items():
        readSensor(ina219_sensor, ina219_address)
  else:
    print("Wrong command!")
    help()

def help():
  print("Usage:", sys.argv[0], "[command]\n")
  print("Commands:")
  print("  [Cluster devices]")
  print("    cv0 - get status of all cluster devices")
  for i in range(0, len(cluster_devices)):
    print("    c[e|d|r|v]%s - %s" % (i + 1, cluster_devices[i]))
  print("  [IP KVM devices]")
  print("    iv0 - get status of all IP KVM devices")
  for i in range(0, len(ipkvm_devices)):
    print("    i[e|d|v]%s - %s" % (i + 1, ipkvm_devices[i]))
  print("  [Voltage/current/power sensors]")
  print("    s0 - read all sensors")
  for i in range(0, len(ina219_sensors)):
    for ina219_sensor, _ in ina219_sensors[i].items():
      print("    s%s - %s" % (i + 1, ina219_sensor))
  print("")
  print("Description:")
  print("  e - enable")
  print("  d - disable")
  print("  r - reboot")
  print("  v - get status")
  print("")

def main():
  if len(sys.argv) == 1 or len(sys.argv[1]) > 3:
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
    elif sys.argv[1][0] == "c" or sys.argv[1][0] == "i":
      psControl(sys.argv[1][:1], sys.argv[1][1:])
    else:
      print("Wrong command!")
      help()
      sys.exit(1)

if __name__== "__main__":
  main()
