# PCG-PSM Arduino Uno Firmware — Rev A.2

`pcg_psm_rev_a.ino` is the permanent-supervisor firmware development target for the Rev A Arduino Uno. The same state-machine design is intended to migrate to the Rev B ATmega328P HAT.

## Implemented

- ACC and service wake handling
- hardware self-latch ownership
- main 12 V power-switch control
- Pi boot heartbeat qualification
- graceful shutdown request / gpio-poweroff acknowledgement
- bounded automatic restart after Pi heartbeat or boot failure
- restart-loop suppression and cooldown
- 12 V / 5 V / temperature telemetry
- calibratable BTS50060 current telemetry
- EEPROM configuration with CRC
- 32-record EEPROM event ring
- line-oriented JSON status/event output
- serial configuration commands
- low-voltage and thermal protection framework

## Intentionally disabled by default

Low-voltage shutdown and thermal shutdown are **not enabled in the default configuration**. Their thresholds must be measured and validated on the bench and in the Chevrolet Volt before `low_voltage_enable` or `thermal_enable` is set to `1` and saved.

BTS50060 current conversion also remains uncalibrated by default; `current_raw` is always available, while `current_ma` remains `null` until the calibration slope is entered.

## Serial commands

At 115200 baud:

```text
STATUS
CONFIG
EVENTS
CLEAR_EVENTS
SET <key> <value>
SAVE
DEFAULTS
HELP
```

`SET` changes RAM immediately. Use `SAVE` to make settings persistent.

Examples:

```text
SET ignition_off_delay_ms 45000
SAVE

SET adc_ref_mv 4976
SAVE
```

## EEPROM layout

- address `0`: CRC-protected `PsmConfig`
- address `128`: 32-entry circular event log

The event log is written only on significant state/fault events rather than continuously.

## Restart behavior

A missing Pi heartbeat or boot timeout initiates the normal GPIO17 shutdown path first. After GPIO22 acknowledgement (or the hard timeout), main power is removed. If ACC remains active, the Uno waits the configured restart delay and tries again.

After the configured number of restart attempts, PCG-PSM holds the Pi off for the cooldown period rather than entering an endless reboot loop. A stable run clears the accumulated restart count.
