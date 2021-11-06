# Command-line tool for devices remote control

## Install dependencies

```bash
mkdir /opt/scripts && cd /opt/scripts
python3 -m venv .venv
source .venv/bin/activate
pip install smbus pi-ina219
```

## Examples of use

- Show all available commands

  ```bash
  $ ./ps-control.py
  Usage: ./ps-control.py [command]

  Commands:
    [Control outputs]
      cs0 - get status of all controlled outputs
      c[e|d|r|s]1 - IP-KVM (Raspberry Pi)
      c[e|d|r|s]2 - Master01 (SBC) + USB module
      c[e|d|r|s]3 - Master02 (SBC) + USB module
      c[e|d|r|s]4 - Master03 (SBC)
      c[e|d|r|s]5 - Worker01 (mini PC)
      c[e|d|r|s]6 - Worker02 (mini PC)
      c[e|d|r|s]7 - Worker03 (mini PC)
      c[e|d|r|s]8 - Ethernet switch
      c[e|d|r|s]9 - External device #1
      c[e|d|r|s]10 - External device #2
      c[e|d|r|s]11 - External device #3
    [USB switch selectors]
      us0 - get status of all USB switch selectors for output #1
      us1 - get status of USB switch selector #1 for output #1
      us2 - get status of USB switch selector #2 for output #1
      ue11 - enable output #1 for USB switch selector #1
      ue12 - enable output #2 for USB switch selector #1
      ue21 - enable output #1 for USB switch selector #2
      ue22 - enable output #2 for USB switch selector #2
    [Voltage/current/power sensors]
      s0 - read all sensors
      s1 - IP-KVM (Raspberry Pi)
      s2 - Master01 (SBC) + USB module
      s3 - USB switch selector #1
      s4 - Master02 (SBC) + USB module
      s5 - Master03 (SBC)
      s6 - Worker01 (mini PC)
      s7 - USB switch selector #2
      s8 - Worker02 (mini PC)
      s9 - Worker03 (mini PC)
      s10 - Ethernet switch
      s11 - External device #1
      s12 - External device #2
      s13 - External device #3

  Description:
    e - enable
    d - disable
    r - reboot
    s - get status
  ```

- Enable device

  ```bash
  $ ./ps-control.py ce4
  Master03 (SBC) has been enabled
  ```

- Reboot device

  ```bash
  $ ./ps-control.py cr4
  Master03 (SBC) - started reboot process
  ```

- Disable device

  ```bash
  $ ./ps-control.py cd4
  Master03 (SBC) has been disabled
  ```

- Show device status

  ```bash
  $ ./ps-control.py cs4
  Master03 (SBC) - disabled
  ```

- Read device Voltage/current/power info

  ```bash
  $ ./ps-control.py s1
  IP-KVM (Raspberry Pi):
  Voltage: 5.304 V
  Current: 556.537 mA
  Power: 3659.024 mW
  ```
