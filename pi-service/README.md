# PCG-PSM Raspberry Pi Service — Rev A.1

This directory contains the Raspberry Pi-side companion design for the PCG-PSM supervisor.

## Frozen Rev A.1 GPIO contract

| Pi GPIO | Physical pin | Function |
| --- | ---: | --- |
| GPIO17 | 11 | active-low shutdown request from Uno through U4B |
| GPIO27 | 13 | 1 Hz heartbeat output through U4C |
| GPIO22 | 15 | safe-to-cut-power output through U4D |
| 3.3 V | 1 | Pi-side pull-up reference only |
| GND | 6 | Pi-side optocoupler return |

No Pi 5 V power is carried on the PCG-PSM signal header.

## Boot configuration

Add the following overlays to the Pi boot configuration:

```text
dtoverlay=gpio-shutdown,gpio_pin=17,active_low=1,gpio_pull=up,debounce=100
dtoverlay=gpio-poweroff,gpiopin=22
```

### Why GPIO22 is the cut-power acknowledgement

Rev A.1 no longer treats an early userspace flag as permission to remove power. The Uno waits for the `gpio-poweroff` hardware indication on GPIO22, isolated through U4D, before normal power removal. A supervisor hard timeout remains the fallback if Linux never reaches that state.

## Heartbeat

A small systemd service will toggle GPIO27 every 500 ms, producing a 1 Hz full cycle while PCG-Core is healthy.

Supervisor policy:

- BOOT requires at least one real heartbeat edge.
- RUN faults if no edge is seen for approximately 15 seconds.
- A watchdog fault requests orderly shutdown first and falls back to the hard power timeout if required.

## Shutdown flow

```text
ACC off
  -> Uno programmable delay
  -> Uno D3 asserts shutdown request
  -> U4B pulls GPIO17 low
  -> gpio-shutdown initiates normal Linux shutdown
  -> OBD Atlas / PCG services stop and storage is flushed
  -> Linux reaches poweroff
  -> gpio-poweroff drives GPIO22
  -> U4D makes Uno D4 active LOW
  -> Uno disables U2 / PS1
  -> Uno releases D7 self-hold
  -> supervisor removes its own power
```

## Service responsibilities

The PCG-PSM userspace component will:

- generate GPIO27 heartbeat
- expose supervisor telemetry/configuration once serial protocol is added
- coordinate PCG-Core application shutdown where needed
- log boot/shutdown/watchdog events

It does **not** have final authority to keep the hardware powered. The Uno/ATmega supervisor remains independent and retains the hard timeout.

## Next implementation

The next software deliverable is a minimal `pcg-psm-heartbeat.service` plus heartbeat executable/script, followed by a serial telemetry protocol after Rev A.1 electrical bench validation.