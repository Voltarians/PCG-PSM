# PCG-PSM Architecture

## Purpose

PCG-PSM is the power supervisor for Promethean Core / PCG-1. It sits between the vehicle 12 V electrical system and PCG-Core and is responsible for reliable startup, shutdown, watchdog recovery, low-voltage protection, service mode, and power telemetry.

## Baseline architecture

```text
Vehicle 12 V
   |
   +-- source fuse
   |
   +-- reverse-polarity / transient protection
   |
   +-- supervisor supply --> Arduino Uno / ATmega328P
   |
   +-- high-side 12 V switch --> 5.1 V / ~10 A DC/DC --> Raspberry Pi 5 + peripherals
```

The high-current Pi/peripheral load is switched on the 12 V side. The full current must not be routed through the Raspberry Pi GPIO header.

## Compute target

- Raspberry Pi 5, 8 GB
- Raspberry Pi Active Cooler
- microSD initially; NVMe later
- OBD Atlas and PCG-Core services
- Multiple USB CAN/SWCAN interfaces

## Supervisor revisions

### Rev A
Existing Arduino Uno used to prove firmware, state machine, ignition handling, shutdown handshake, watchdog, voltage sensing, and service mode.

### Rev B
Custom Raspberry Pi HAT-class board using ATmega328P-derived logic and the proven Rev A firmware architecture.

### Rev C
Automotive-hardened production board integrated into PCG-1.

## Interfaces

Planned logical signals:

- ACC / ignition sense
- Main power enable
- Pi shutdown request
- Pi shutdown acknowledgement
- Pi heartbeat
- Supervisor self-hold / latch
- Service / manual wake
- 12 V voltage sense
- 5 V voltage sense
- temperature sense
- serial telemetry/configuration

Raspberry Pi GPIO is 3.3 V only. Uno/ATmega 5 V signals require safe level shifting, open-drain stages, or equivalent protection.

## Whole-vehicle current

The Chevrolet Volt already has a factory current sensor at the 12 V battery negative terminal. PCG-PSM will not duplicate whole-vehicle current measurement in Rev A. OBD Atlas / PCG-Core should obtain that information from BCM/CAN data where available.
