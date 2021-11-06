# Command-line tool for devices remote control

## Install dependencies

```bash
$ mkdir /opt/scripts && cd /opt/scripts
$ python3 -m venv .venv
$ source .venv/bin/activate
$ pip install smbus pi-ina219
```

## Examples of use

- Show all available commands

  ```bash
  $ ./ps-control.py
  Usage: ./ps-control.py [command]

  Commands:
    [Cluster devices]
      cv0 - get status of all cluster devices
      c[e|d|r|v]1 - Raspberry Pi
      c[e|d|r|v]2 - USB module #1
      c[e|d|r|v]3 - USB module #2, for serial console's (IP KVM)
      c[e|d|r|v]4 - Master01 (mini PC)
      c[e|d|r|v]5 - Master02 (Rock64)
      c[e|d|r|v]6 - Master03 (Rock64)
      c[e|d|r|v]7 - Worker01 (mini PC)
      c[e|d|r|v]8 - Worker02 (Rock64)
      c[e|d|r|v]9 - Worker03 (Rock64)
    [IP KVM devices]
      iv0 - get status of all IP KVM devices
      i[e|d|v]1 - IP KVM for Master01
      i[e|d|v]2 - IP KVM for Worker01
      i[e|d|v]3 - D13
      i[e|d|v]4 - A0
      i[e|d|v]5 - A1
    [Voltage/current/power sensors]
      s0 - read all sensors
      s1 - Raspberry Pi
      s2 - Master01 (mini PC)
      s3 - Master02 (Rock64)
      s4 - Master03 (Rock64)
      s5 - Worker01 (mini PC)
      s6 - Worker02 (Rock64)
      s7 - Worker03 (Rock64)

  Description:
    e - enable
    d - disable
    r - reboot
    v - get status
  ```

- Enable device

  ```bash
  $ ./ps-control.py ce4
  Master01 (mini PC) has been enabled
  ```

- Reboot device

  ```bash
  $ ./ps-control.py cr4
  Master01 (mini PC) has been rebooted
  ```

- Disable device

  ```bash
  $ ./ps-control.py cd4
  Master01 (mini PC) has been disabled
  ```

- Show device status

  ```bash
  $ ./ps-control.py cv4
  Master01 (mini PC) is disabled
  ```

- Read device Voltage/current/power info

  ```bash
  $ ./ps-control.py s1
  Raspberry Pi:
  Voltage: 5.304 V
  Current: 556.537 mA
  Power: 3659.024 mW
  ```
