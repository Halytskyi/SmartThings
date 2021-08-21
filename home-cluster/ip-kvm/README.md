# IP-KVM

- [IP-KVM](#ip-kvm)
  - [Description](#description)
    - [Main functions](#main-functions)
  - [Requirements and components](#requirements-and-components)
    - [Board #1](#board-1)
    - [Board #2](#board-2)
    - [Other components](#other-components)
  - [4 Ports HDMI KVM Switcher](#4-ports-hdmi-kvm-switcher)
  - [Hardware Arduino HID instead of the OTG](#hardware-arduino-hid-instead-of-the-otg)
    - [Switch IP-KVM to Hardware Arduino HID](#switch-ip-kvm-to-hardware-arduino-hid)
    - [Flash the Arduino HID (TTL Firmware)](#flash-the-arduino-hid-ttl-firmware)

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

### Board #1

- 1 x Raspberry Pi 4 (2 GB)
- 1 x 32 GB MicroSD card
- 1 x [HDMI to CSI-2 bridge based on TC358743](https://www.aliexpress.com/item/4000102166176.html)
- 1 x [USB/Power Splitter Module](https://www.pishop.us/product/usb-pwr-splitter/)
- 1 x USB-A to USB-C cable (male-male) for connecting the Raspberry Pi to the splitter
- 1 x USB-A to micro USB-B cable (male-male) for connecting the server to the splitter

### Board #2

- 12 x [MOSFET relays OMRON G3VM-61A1](https://www.aliexpress.com/item/1005001989679623.html?spm=a2g0s.9042311.0.0.27424c4dnmIQ2j)
- 13 x 390 Ohm resistors
- 6 x 4.7k Ohm resistors
- 1 x ATMEGA32U4 Pro Micro 5V/16MHZ
- 1 x 2N2222A

### Other components

- 3 x USB to PL2303 UART TTL Cables
- [4 Ports KVM Switcher 4K HDMI-compatible Video Display 4 IN 1 Out Type USB-C with EDID/HDCP decryption](https://www.aliexpress.com/item/1005001873550202.html?spm=a2g0s.9042311.0.0.49584c4dNbLu7M)

## 4 Ports HDMI KVM Switcher

[4 Ports HDMI KVM Switcher](https://www.aliexpress.com/item/1005001873550202.html?spm=a2g0s.9042311.0.0.49584c4dNbLu7M) support using real keyboard for selecting ports, but nor Raspberry Pi4 emulated HID netheir Arduino emulated HID dostn't support it, therefore I used "Omron G3VM-61A1" for switching ports and read their status (via Web UI).

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
