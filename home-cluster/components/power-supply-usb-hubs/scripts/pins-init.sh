#!/bin/bash
# Pin num found here: https://github.com/adafruit/Adafruit_Blinka/issues/666#issuecomment-1519068609
#
# SPI relay control, GPIO2_D4 (Pin 11, right side)
# Check if gpio is already exported
if [ ! -d /sys/class/gpio/gpio92 ]
then
  echo 92 > /sys/class/gpio/export
  sleep 1 # Short delay while GPIO permissions are set up
fi
echo "out" > /sys/class/gpio/gpio92/direction
echo 0 > /sys/class/gpio/gpio92/value

# SPI relay control, GPIO1_A3 (Pin 13, right side)
# Check if gpio is already exported
if [ ! -d /sys/class/gpio/gpio35 ]
then
  echo 35 > /sys/class/gpio/export
  sleep 1 # Short delay while GPIO permissions are set up
fi
echo "out" > /sys/class/gpio/gpio35/direction
echo 0 > /sys/class/gpio/gpio35/value
