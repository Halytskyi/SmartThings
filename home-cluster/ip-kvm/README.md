# IP-KVM

- [IP-KVM](#ip-kvm)
  - [Description](#description)
    - [Main functions](#main-functions)
  - [Requirements and components](#requirements-and-components)
    - [Raspberry Pi](#raspberry-pi)
    - [Board](#board)
    - [Other components](#other-components)
    - [Pinout](#pinout)
  - [4 Ports HDMI KVM Switcher](#4-ports-hdmi-kvm-switcher)
    - [Switching channels](#switching-channels)
    - [Power supply](#power-supply)
  - [Hardware Arduino HID instead of the OTG](#hardware-arduino-hid-instead-of-the-otg)
    - [Switch IP-KVM to Hardware Arduino HID](#switch-ip-kvm-to-hardware-arduino-hid)
    - [Flash the Arduino HID (TTL Firmware)](#flash-the-arduino-hid-ttl-firmware)
  - [Photos](#photos)
    - [Complete device photos](#complete-device-photos)
    - [Board photos](#board-photos)

## Description

IP-KVM based on Raspberry Pi, for x64 architecture uses PI-KVM: [https://github.com/pikvm/pikvm](https://github.com/pikvm/pikvm) - [hardware for v2](https://github.com/pikvm/pikvm#hardware-for-v2), for arm architecture (single board computers) uses serial console via PL2303 UART TTL Cables.

### Main functions

- Access to 3 single board computers via serial console;
- Access to 3 mini PCs via IP KVM based on [PiKVM](https://github.com/pikvm/pikvm) with other features:
  - FullHD video using advanced HDMI-to-CSI bridge;
  - Bootable Virtual CD-ROM and Flash Drive;
  - USB Keyboard and mouse;
  - Access via Web UI or VNC
  - Control power (on mini PCs) by power buttons via Web UI;
  - Check power status (on mini PCs) by reading power LED state via Web UI.

## Requirements and components

### Raspberry Pi

- 1 x Raspberry Pi 4 (2 GB)
- 1 x 32 GB MicroSD card

### Board

- 1 x [HDMI to CSI-2 bridge based on TC358743](https://www.aliexpress.com/item/4000102166176.html)
- 1 x ATMEGA32U4 Pro Micro 5V/16MHZ
- 12 x [MOSFET relays OMRON G3VM-61A1](https://www.aliexpress.com/item/1005001989679623.html?spm=a2g0s.9042311.0.0.27424c4dnmIQ2j)
- 13 x 390 Ohm resistors
- 6 x 4.7k Ohm resistors
- 1 x 2N2222A
- 1 x Bidirectional Logic Level Converter

### Other components

- ?1 x [USB/Power Splitter Module](https://www.pishop.us/product/usb-pwr-splitter/)
- ?1 x USB-A to USB-C cable (male-male) for connecting the Raspberry Pi to the splitter
- ?1 x USB-A to micro USB-B cable (male-male) for connecting the server to the splitter
- 3 x USB to PL2303 UART TTL Cables
- [4 Ports KVM Switcher 4K HDMI-compatible Video Display 4 IN 1 Out Type USB-C with EDID/HDCP decryption](https://www.aliexpress.com/item/1005001873550202.html?spm=a2g0s.9042311.0.0.49584c4dNbLu7M)

### Pinout

[<img src="images/RPi4-Pinout.jpeg" width="500"/>](images/RPi4-Pinout.jpeg)

| RPi-4 PIN | Component on Board | Notes |
| --- | --- | --- |
| 3.3V (1) | Power for OMRONs G3VM-61A1 ||
| GND (6) | Bidirectional LLC GND -> Arduino GND ||
| GPIO 14 (UART TX) | Bidirectional LLC -> Arduino RX ||
| GPIO 15 (UART RX) | Bidirectional LLC -> Arduino TX ||
| GPIO 4 (GPCLK0) | 2N2222A | For reset the HID |
| GPIO 17 | OMRON 1 (input) | KVM Switch, channel #1: LED |
| GPIO 27 | OMRON 2 (output) | KVM Switch, channel #1: Button |
| GPIO 22 | OMRON 3 (input) | KVM Switch, channel #2: LED |
| GPIO 23 | OMRON 4 (output) | KVM Switch, channel #2: Button |
| GPIO 24 | OMRON 5 (input) | KVM Switch, channel #3: LED |
| GPIO 25 | OMRON 6 (output) | KVM Switch, channel #3: Button |
| GPIO 5 | OMRON 7 (output) | Worker01: Power button |
| GPIO 6 | OMRON 8 (input) | Worker01: LED |
| GPIO 13 | OMRON 9 (output) | Worker02: Power button |
| GPIO 26 | OMRON 10 (input) | Worker02: LED |
| GPIO 12 | OMRON 11 (output) | Worker03: Power button |
| GPIO 16 | OMRON 12 (input) | Worker03: LED |

## 4 Ports HDMI KVM Switcher

### Switching channels

[4 Ports HDMI KVM Switcher](https://www.aliexpress.com/item/1005001873550202.html?spm=a2g0s.9042311.0.0.49584c4dNbLu7M) support using real keyboard for selecting ports, but nor Raspberry Pi4 emulated HID netheir Arduino emulated HID dostn't support it, therefore I used "Omron G3VM-61A1" for switching ports and read their status (via Web UI).

### Power supply

This KVM switch doesn't have separate input for power supply, therefore, it powering via usb-c ports from PCs. Also, it has "back powering issue" as power also coming to HDMI output port and USB HID ports (from Raspberry Pi) - see details on [Multiport KVM over IP](https://github.com/pikvm/pikvm/blob/master/pages/multiport.md), as result, after shutdown Raspberry Pi it wont't be started and will be needed push power button on KVM switch.
To solve this issue I desided make own USB cables without power (+5V) wire. As I need only 3 KVM ports I use 4rd port for powering KVM switch from board #2. By this modification KVM switch powering only when Raspberry Pi started and it doesn't have "back powering issue".

## Hardware Arduino HID instead of the OTG

As some motherboards contain a buggy BIOS that does not understand the keyboard of the v2 platform I implemented "Arduino HID".

Hightlights from [official doc](https://github.com/pikvm/pikvm/blob/master/pages/arduino_hid.md):

### Switch IP-KVM to Hardware Arduino HID

- Switch to RW-mode using command `rw`.
- Add these lines to `/etc/kvmd/override.yaml` (remove `{}` in the file before):

  ```bash
  kvmd:
    hid:
        type: serial
        reset_pin: 4
        device: /dev/kvmd-hid
  ```

- Create file `/etc/udev/rules.d/99-kvmd-extra.rules`:
  
  ```bash
  KERNEL=="ttyAMA0", SYMLINK+="kvmd-hid"
  ```

- Run `systemctl disable getty@ttyAMA0.service`.
- Remove `console=ttyAMA0,115200` or `console=serial0,115200` and `kgdboc=ttyAMA0,115200` or `kgdboc=serial0,115200` from `/boot/cmdline.txt`.

### Flash the Arduino HID (TTL Firmware)

- Disconnect the RESET wire from the Arduino board.
- Connect the Arduino and RPi with a suitable USB cable.
- Execute `rw`, add line `dtoverlay=spi0-1cs` to `/boot/config.txt` and perform `reboot`.
- Upload the firmware (USB keyboard & mouse is used by default)
  
  ```bash
  [root@pikvm ~]# rw
  [root@pikvm ~]# systemctl stop kvmd
  [root@pikvm ~]# cp -r /usr/share/kvmd/hid ~
  [root@pikvm ~]# cd ~/hid
  [root@pikvm hid]# make
  [root@pikvm hid]# make install
  ```

- Connect the RESET wire, disconnect the USB cable, and `reboot` the RPi.

## Photos

### Complete device photos

### Board photos

[<img src="images/board1_1.jpeg" width="300"/>](images/board1_1.jpeg)
