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
arduinoSlaveAddress = 0x14
ps = ["24V 15A", "18V 20A", "Solar Battaries", "12V DC-DC"]
lineParams = ["Voltage", "Current", "Power", "Energy", "Frequency", "Power Factor"]

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
  time.sleep(0.3)
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
  if data == "v":
    for i in range(0, len(values)):
      print(f"Voltage {ps[i]}: {values[i]}V")
  elif data == "i":
    for i in range(0, len(values)):
      print(f"Current {ps[i]}: {values[i]}A")
  elif data == "p":
    for i in range(0, len(values)):
      print(f"Power {ps[i]}: {values[i]}W")
  elif data == "t":
    print(f"Temperature 24V 15A: {values[0]}°C")
    print(f"Temperature 18V 20A: {values[1]}°C")
    print(f"Temperature 12V DC-DC2, from 24V 15A PS: {values[2]}°C")
    print(f"Temperature 12V DC-DC1, from 18V 20A PS: {values[3]}°C")
  elif data == "l":
    for i in range(0, len(values)):
      print(f"Line {lineParams[i]}: {values[i]}")
  else:
    print(response)

def help():
  print("Usage:", sys.argv[0], "<cluster|rack> <command>\n")
  print("Commands:")
  print("  sd - get value of source: Solar Battaries or DC-DC converter")
  print("    sd[0-1] - set value of source: Solar Battaries or DC-DC converter. 0 - Solar Battaries, 1 - DC-DC converter\n")
  print("  v - get voltage from all sensors")
  print("    v[1-4] - get value of voltage for 1-4 outputs\n")
  print("  i - get current from all sensors")
  print("    i[1-4] - get value of current for 1-4 outputs\n")
  print("  p - get power from all sensors")
  print("    p[1-4] - get value of power for 1-4 outputs\n")
  print("  t - get temperature from all sensors")
  print("    t[1-4] - get value of temperature for 1-4 outputs")
  print("  ta - get value of auto-mode for get temperatures")
  print("    ta[0-1] - set value of auto-mode for get temperatures. 0 - disabled, 10-120 seconds\n")
  print("  l - get line parameters")
  print("    l[v,c,p,e,f,pf] - get value of AC line parameters: voltage, current, power, energy, frequency, power factor")
  print("  la - get value of auto-mode for get AC line parameters")
  print("    la[0-1] - set value of auto-mode for get AC line parameters. 0 - disabled, 10-120 seconds\n")

def main():
  if len(sys.argv) < 2 or len(sys.argv[1]) > 6:
    help()
    sys.exit(1)

  command(sys.argv[1])

if __name__== "__main__":
  main()
