# IP-KVM

- [IP-KVM](#ip-kvm)
  - [Description](#description)
    - [Main functions](#main-functions)
  - [Requirements and components](#requirements-and-components)
    - [Board #1](#board-1)
    - [Board #2](#board-2)
    - [Other components](#other-components)

## Description

IP-KVM based on Raspberry Pi, almost everything taken from: [https://github.com/pikvm/pikvm](https://github.com/pikvm/pikvm) - [hardware for v2](https://github.com/pikvm/pikvm#hardware-for-v2). I just made it suitable for my cluster and added modules for access to single board computers.

### Main functions

- Access to 3 single board computers via serial console;
- Access to 3 mini PCs via IP KVM based on [PiKVM](https://github.com/pikvm/pikvm);
- Control mini PC power via power button;
- Check power status by reading power LED state.

Base main functions of deivce:

- FullHD video using advanced HDMI-to-CSI bridge;
- Bootable Virtual CD-ROM and Flash Drive;
- USB Keyboard and mouse;
- Access via Web UI or VNC.

## Requirements and components

### Board #1

- 1 x Raspberry Pi 4 (2 GB)
- 1 x 16 GB MicroSD card
- 1 x [HDMI to CSI-2 bridge based on TC358743](https://www.aliexpress.com/item/4000102166176.html)
- 1 x [USB/Power Splitter Module](https://www.pishop.us/product/usb-pwr-splitter/)
- 1 x USB-A to USB-C cable (male-male) for connecting the Raspberry Pi to the splitter
- 1 x USB-A to micro USB-B cable (male-male) for connecting the server to the splitter

### Board #2

- 6 x [MOSFET relays OMRON G3VM-61A1](https://www.aliexpress.com/item/1005001989679623.html?spm=a2g0s.9042311.0.0.27424c4dnmIQ2j)
- 6 x 390 Ohm resistors
- 3 x 4.7k Ohm resistors
- 1 x 4 port USB HUB module

### Other components

- 3 x USB to PL2303 UART TTL Cables
- [4 Ports KVM Switcher 4K HDMI-compatible Video Display 4 IN 1 Out Type USB-C with EDID/HDCP decryption](https://www.aliexpress.com/item/1005001873550202.html?spm=a2g0s.9042311.0.0.49584c4dNbLu7M)
