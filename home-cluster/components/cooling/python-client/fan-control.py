#!/usr/bin/env python3
#
# Copyright (C) 2023 Oleh Halytskyi
#
# This software may be modified and distributed under the terms
# of the Apache license. See the LICENSE file for details.

import sys
import time
from smbus import SMBus

i2cBusAddr = 5             # indicates /dev/i2c-5
arduinoSlaveAddress = 0x12
numTempSensors = 8

# This function converts a string to an array of bytes
def convert_string_to_bytes(src):
  converted = []
  for b in src:
    converted.append(ord(b))
  return converted

def send_data(data):
  bus = SMBus(i2cBusAddr)
  BytesToSend = convert_string_to_bytes(data[1:])
  bus.write_i2c_block_data(arduinoSlaveAddress, ord(data[:1]), BytesToSend)
  time.sleep(0.1)
  response = ""
  while True:
    responseChr = chr(bus.read_byte(arduinoSlaveAddress))
    if responseChr != ";":
      response += responseChr
    else:
      break
  bus.close()
  return response

def command(data):
  response = send_data(data)
  if response == "err":
    print("Wrong command!\n")
    help()
    sys.exit(1)
  values = response.split(",")[:-1]
  if data == "f":
    for i in range(0, len(values)):
      print(f"Fan {i+1} speed: {values[i]}%")
  elif data == "f-ac":
    for i in range(0, len(values)):
      print(f"Fan {i+1} auto control mode: {values[i]}")
    print("Note: 0 - disabled, 1 - enabled without speed control, 2 - enabled with speed control")
  elif data == "f-tl":
    for i in range(0, len(values)):
      print(f"Fan {i+1} temperature low limit: {values[i]}°C")
    print("Note: if temperature is less than defined value - fan is stopped")
  elif data == "f-th":
    for i in range(0, len(values)):
      print(f"Fan {i+1} temperature high limit: {values[i]}°C")
    print("Note: if temperature is greater than defined value - fan speed is 100%")
  elif data == "t":
    for i in range(0, len(values)):
      print(f"Temperature {i+1}: {values[i]}°C")
    if numTempSensors == 8:
      print("Note: '1-2' temperature sensors for 'Fan #1', '3-4' - 'Fan #2', '5-6' - 'Fan #3', '7-8' - 'Fan #4'")
      print("In automatic fan control mode '2', fan speed calculation by temperature from sensor in group with highest temperature.")
  else:
    print(response)

def help():
  print("Usage:", sys.argv[0], "<cluster|rack> <command>\n")
  print("Commands:")
  print("  f - get fan speed in percents")
  print("    f[1-4] get fan speed in percents for fan #1-#4")
  print("    f[1-4]=[0-100] set fan speed in percents for fan #1-#4\n")
  print("  f-ac - get fan auto control mode")
  print("    f-ac[1-4] get fan auto control mode for fan #1-#4")
  print("    f-ac[1-4]=[0-2] set fan auto control mode for fan #1-#4. 0 - disabled, 1 - enabled without speed control, 2 - enabled with speed control\n")
  print("  f-tl - get fan temperature low limit. If temperature is less than defined value - fan is stopped")
  print("    f-tl[1-4] get fan temperature low limit for fan #1-#4")
  print("    f-tl[1-4]=[20-25] set fan temperature low limit for fan #1-#4. Allowed values: 20-25\n")
  print("  f-th - get fan temperature high limit. If temperature is greater than defined value - fan speed is 100%")
  print("    f-th[1-4] get fan temperature high limit for fan #1-#4")
  print("    f-th[1-4]=[26-39] set fan temperature high limit for fan #1-#4. Allowed values: 26-39\n")
  print("  t - get temperature in Celsius degrees")
  print("    t[1-4,8] get temperature in Celsius degrees for temperature sensor #1-#4,8")
  print("  ta - get value of auto-mode for get temperatures")
  print("    ta=[10-120] set value of auto-mode for get temperatures. 0 - disabled, 10-120 - seconds\n")
  print("  a - external alarm status")
  print("    a=[0,1] set external alarm status. 0 - disabled, 1 - enabled\n")
  print("For rack: f1/t1 - back wall, f2/t2 - left, f3/t3 - top, f4/t4 - right\n")

def main():
  global arduinoSlaveAddress, numTempSensors

  if len(sys.argv) < 3 or (sys.argv[1] != "cluster" and sys.argv[1] != "rack") or len(sys.argv[2]) > 8:
    help()
    sys.exit(1)

  if sys.argv[1] == "rack":
    arduinoSlaveAddress = 0x13
    numTempSensors = 4

  command(sys.argv[2])

if __name__== "__main__":
  main()
