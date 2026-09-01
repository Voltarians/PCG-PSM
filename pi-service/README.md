# PCG-PSM Raspberry Pi Service

This directory will contain the Raspberry Pi-side companion service for the PCG-PSM supervisor.

## Responsibilities

- Generate a periodic heartbeat for the Arduino/ATmega supervisor
- Receive supervisor shutdown requests
- Stop OBD Atlas and other PCG-Core services cleanly
- Flush filesystems and databases
- Assert shutdown acknowledgement when the system is ready for power removal
- Expose supervisor telemetry to PCG-Core
- Provide local configuration for delays, thresholds, and service behavior

## Planned deployment

The service will run under `systemd` and start automatically during boot.

## Safety behavior

The Pi service is not the final authority over power. The supervisor remains independent and retains a hard shutdown timeout if Linux is unresponsive.

## Initial interface

The first Rev A implementation will use simple GPIO heartbeat/shutdown lines. A serial protocol will be added for telemetry and configuration once the electrical interface is bench-validated.
