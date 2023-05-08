#!/bin/bash

node=$1

if [ -z "$node" ]; then
    echo "Usage: $0 <node>. Valid nodes are master01 and master02"
    exit 1
fi

echo "Switching I2C bus to node $node"
if [ "$node" == "master01" ]; then
    echo 0 > /sys/class/gpio/gpio92/value
    if [ $? -ne 0 ]; then
        echo "Failed to switch I2C bus to node $node"
        exit 1
    fi
    echo 0 > /sys/class/gpio/gpio35/value
    if [ $? -ne 0 ]; then
        echo "Failed to switch I2C bus to node $node"
        exit 1
    fi
elif [ "$node" == "master02" ]; then
    echo 1 > /sys/class/gpio/gpio92/value
    if [ $? -ne 0 ]; then
        echo "Failed to switch I2C bus to node $node"
        exit 1
    fi
    echo 1 > /sys/class/gpio/gpio35/value
    if [ $? -ne 0 ]; then
        echo "Failed to switch I2C bus to node $node"
        exit 1
    fi
else
    echo "Invalid node $node"
    exit 1
fi
echo "Done"
