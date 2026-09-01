# Rev A Hardware Prototype

Rev A uses the existing Arduino Uno as the development supervisor for PCG-PSM.

## Goals

- Prove ignition/ACC sensing
- Prove self-latching supervisor power
- Control the main 12 V-side switch feeding the 5.1 V DC/DC converter
- Establish safe 5 V/3.3 V signaling with Raspberry Pi 5
- Measure vehicle 12 V and main 5 V rails
- Add power-system temperature sensing
- Implement service/manual wake
- Validate graceful shutdown and watchdog recovery

## Provisional Uno I/O

| Uno pin | Function |
| --- | --- |
| D2 | ACC / ignition sense |
| D3 | Pi shutdown request |
| D4 | Pi shutdown acknowledgement |
| D5 | Pi heartbeat |
| D6 | Main power enable |
| D7 | Supervisor self-hold / latch |
| D8 | Service / manual wake |
| A0 | 12 V system voltage |
| A1 | 5 V rail voltage |
| A2 | Power-system temperature |

These assignments are provisional until the Rev A schematic is frozen.

## Electrical rules

- Never apply vehicle 12 V directly to an Arduino digital/analog pin.
- Never connect a 5 V Uno output directly to Raspberry Pi GPIO.
- Use protected/conditioned ACC sensing.
- Use appropriate divider/filter/protection networks for voltage measurement.
- Use a dedicated automotive-protected supervisor supply.
- Switch the high-current load on the 12 V side.
- Do not route the full Pi/peripheral load through the GPIO header.

## Main DC/DC target

The external converter target is approximately:

- nominal input: vehicle 12 V
- wide automotive input range preferred
- output: about 5.1 V
- continuous capacity: at least 5 A; approximately 10 A preferred for margin
- thermal and transient behavior suitable for permanent automotive installation

## Rev A release gate

Rev A is not ready for vehicle permanence until repeated startup/shutdown, watchdog, low-voltage, thermal, and standby-current tests pass.
