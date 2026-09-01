# PCG-PSM — Promethean Core Power Supervisor Module

PCG-PSM is the dedicated automotive power-management subsystem for **Promethean Core / PCG-1**.

Its job is to make a Raspberry Pi 5 operate reliably as a permanent in-vehicle computer by controlling startup, shutdown, watchdog recovery, low-voltage protection, service mode, and power-system telemetry.

The first development version uses an **Arduino Uno**. The proven logic will later migrate to a custom Raspberry Pi HAT based on the same ATmega328P architecture.

## Mission

PCG-PSM provides intelligent power supervision between the vehicle's 12-V electrical system and PCG-Core.

Primary goals:

* Ignition/ACC-aware startup
* Graceful Raspberry Pi Linux shutdown
* Programmable shutdown delay
* Independent Pi heartbeat watchdog
* Automatic recovery from a hung Pi
* Low-12-V battery protection with hysteresis
* Service/maintenance mode
* 12-V and 5-V rail monitoring
* Power-system temperature monitoring
* Persistent boot, shutdown, fault, and watchdog logging
* Self-latching power control for minimal standby draw
* Future CAN/network-triggered wake support

## Hardware Target

### PCG-Core

* Raspberry Pi 5
* 8 GB RAM
* Raspberry Pi Active Cooler
* microSD storage initially
* NVMe storage planned
* Multiple USB CAN interfaces
* OBD Atlas / PCG-Core services

### Main Power

The Raspberry Pi and attached interfaces will be powered from an external automotive-grade DC/DC converter.

Target specification:

* Vehicle input: nominal 12 V
* Wide automotive input range preferred
* Output: approximately 5.1 V
* Capacity: approximately 10 A / 50 W
* Main high-current switching performed on the 12-V side

The full Pi/peripheral current must **not** be routed through the Raspberry Pi GPIO header.

## Development Revisions

### Rev A — Arduino Uno Prototype

The existing Arduino Uno is used as the permanent supervisor firmware development platform.

Rev A proves:

* ignition detection
* power enable
* graceful shutdown
* Pi heartbeat watchdog
* low-voltage behavior
* service mode
* self-latching shutdown
* telemetry
* fault handling

### Rev B — PCG Power HAT

Rev B moves the proven Rev A design onto a dedicated Raspberry Pi HAT-class PCB.

Planned controller:

* ATmega328P or compatible architecture

The board must remain mechanically compatible with:

* Raspberry Pi 5
* Raspberry Pi Active Cooler
* Standard Raspberry Pi M.2 HAT+ stack

### Rev C — Automotive Production Hardware

Rev C hardens the design for permanent PCG-1 installation.

Planned improvements include:

* automotive transient protection
* optimized standby current
* production connectors
* environmental protection
* improved thermal design
* integrated manufacturing test points
* production-ready PCB and enclosure support

## Supervisor State Machine

The planned operating states are:

```text
OFF
 ↓
WAKE
 ↓
PRECHECK
 ↓
BOOT
 ↓
RUN
 ↓
SHUTDOWN_REQUEST
 ↓
SHUTDOWN_WAIT
 ↓
POWER_OFF
```

Additional states:

```text
SERVICE
FAULT
```

## Startup Sequence

Typical startup:

```text
ACC / wake signal active
        ↓
Supervisor wakes
        ↓
Check vehicle voltage
        ↓
Enable main DC/DC converter
        ↓
Power Raspberry Pi
        ↓
Wait for Pi heartbeat
        ↓
Enter RUN state
```

## Shutdown Sequence

Typical shutdown:

```text
ACC OFF
   ↓
Programmable delay
   ↓
Shutdown request sent to Raspberry Pi
   ↓
OBD Atlas and PCG services stop cleanly
   ↓
Storage buffers flushed
   ↓
Pi confirms shutdown
   ↓
Supervisor disables main power
   ↓
Shutdown reason stored
   ↓
Supervisor releases its own power latch
```

The supervisor also has a hard timeout so a failed or frozen operating system cannot leave PCG-Core powered indefinitely.

## Watchdog

The Raspberry Pi sends a periodic heartbeat to PCG-PSM.

If the heartbeat disappears while the system should be running, the supervisor can:

1. Detect the missing heartbeat.
2. Allow a configurable recovery interval.
3. Request a controlled shutdown.
4. Remove power if the Pi does not respond.
5. Wait before restart.
6. Re-enable PCG-Core.
7. Record the event as a watchdog reset.

This watchdog operates independently of Linux.

## Low-Voltage Protection

PCG-PSM will monitor vehicle 12-V voltage.

Planned levels include:

```text
NORMAL
LOW
CRITICAL
EMERGENCY
ABSOLUTE MINIMUM
```

Thresholds will use hysteresis to prevent repeated power cycling near a cutoff voltage.

## Service Mode

Service mode allows PCG-Core to remain powered with ACC off for:

* diagnostics
* CAN logging
* OBD Atlas development
* vehicle programming support
* maintenance
* bench testing

Service mode will include a configurable timeout to prevent accidental battery discharge.

## Telemetry

PCG-PSM should provide PCG-Core with data such as:

```text
Vehicle voltage
5-V rail voltage
Supply temperature
Ignition state
Supervisor state
Pi heartbeat state
Uptime
Boot count
Watchdog reset count
Last shutdown reason
Last fault reason
Service-mode state
```

Telemetry will be available to PCG-Core through a simple serial and/or GPIO interface.

## Chevrolet Volt 12-V Current Measurement

The Chevrolet Volt already contains a factory battery-current sensor on the negative terminal of the 12-V battery.

PCG-PSM does not need to duplicate that whole-vehicle current measurement.

Where possible, PCG-Core / OBD Atlas will obtain factory battery-current information through the BCM/CAN network.

A separate PCG current sensor may be added later only if measuring PCG-Core's own consumption proves useful.

## Raspberry Pi Interface

The Raspberry Pi GPIO operates at **3.3 V** and is not 5-V tolerant.

The Arduino Uno / ATmega supervisor therefore must use safe interfaces such as:

* open-drain transistor stages
* level shifters
* optocouplers where appropriate

Direct 5-V Uno outputs must never be connected directly to Pi GPIO pins.

## Initial Signal Plan

Provisional Rev A signals:

```text
D2   ACC / ignition sense
D3   Pi shutdown request
D4   Pi shutdown acknowledgement
D5   Pi heartbeat
D6   Main power enable
D7   Supervisor self-hold
D8   Service / manual wake

A0   12-V system voltage
A1   5-V rail voltage
A2   Power-system temperature
```

The final assignments may change during schematic development.

## Automotive Input Protection

The permanent design must include appropriate protection against automotive electrical conditions.

Planned protection includes:

* source fuse
* reverse-polarity protection
* TVS transient suppression
* filtered ACC input
* protected voltage measurement
* robust grounding
* appropriate connector and wire sizing

The Arduino Uno VIN input is not considered sufficient automotive protection by itself.

## Repository Structure

Planned repository layout:

```text
docs/
    architecture.md
    state-machine.md
    power-interface.md
    validation.md

hardware/
    rev-a/
    rev-b-hat/
    bom/

firmware/
    arduino-uno/

pi-service/

tests/
```

## Rev A Acceptance Criteria

Rev A is considered successful when:

1. ACC-on reliably starts PCG-Core.
2. ACC-off triggers a configurable graceful shutdown.
3. Linux finishes shutdown before main power is removed.
4. A hard-timeout handles failed shutdowns safely.
5. Missing Pi heartbeat triggers controlled recovery.
6. Watchdog events are recorded.
7. Low-voltage shutdown operates with proper hysteresis.
8. Service mode works with a configurable timeout.
9. Supervisor standby current becomes negligible after shutdown.
10. The design operates successfully from a 12-V bench supply.
11. The same architecture passes in-vehicle testing in the Chevrolet Volt.
12. OBD Atlas logging and storage shut down without corruption.

## Relationship to Promethean Core

PCG-PSM is one subsystem of the larger Promethean Core architecture.

```text
Vehicle 12-V System
        ↓
     PCG-PSM
        ↓
     PCG-Core
        ↓
 Raspberry Pi 5
        ↓
 CAN / SWCAN / vehicle interfaces
        ↓
    OBD Atlas
```

PCG-PSM owns power reliability.

PCG-Core owns vehicle-network processing, diagnostics, logging, gateway services, and integration with the rest of the Promethean Core platform.

## Current Status

Current phase:

**Rev A architecture and component selection**

Available development hardware:

* Arduino Uno
* 12-V bench power source

Immediate next tasks:

1. Select permanent automotive 12-V to approximately 5.1-V / 10-A converter.
2. Finalize input protection.
3. Finalize main power-switch circuit.
4. Finalize Pi/Uno signal interface.
5. Build the Rev A schematic and BOM.
6. Implement the initial Arduino state machine.
7. Bring up the Raspberry Pi-side shutdown and heartbeat service.
8. Begin bench validation.

---

**PCG-PSM**
Promethean Core Power Supervisor Module
