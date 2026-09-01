# Supervisor State Machine

PCG-PSM uses an explicit state machine so power behavior remains deterministic and testable.

## States

```text
OFF
WAKE
PRECHECK
BOOT
RUN
SHUTDOWN_REQUEST
SHUTDOWN_WAIT
POWER_OFF
SERVICE
FAULT
```

## Normal startup

```text
OFF -> WAKE -> PRECHECK -> BOOT -> RUN
```

- Wake source becomes active.
- Supervisor powers/latches itself.
- Vehicle input voltage is checked.
- Main 12 V power switch and DC/DC converter are enabled.
- Raspberry Pi boots.
- Pi heartbeat is detected.
- Supervisor enters RUN.

## Normal shutdown

```text
RUN -> SHUTDOWN_REQUEST -> SHUTDOWN_WAIT -> POWER_OFF -> OFF
```

- ACC/wake input goes inactive.
- Configurable delay starts.
- Supervisor requests Linux shutdown.
- Pi stops OBD Atlas / PCG services and flushes storage.
- Pi asserts shutdown acknowledgement.
- Supervisor waits a short safety delay.
- Main converter is disabled.
- Shutdown reason is stored.
- Supervisor releases its own latch.

## Watchdog recovery

If Pi heartbeat disappears while the system should be running:

1. Start watchdog grace period.
2. Request orderly recovery/shutdown if possible.
3. If the Pi remains unresponsive, remove main power after a hard timeout.
4. Wait before restart.
5. Re-enable main power if the wake condition still requires operation.
6. Record `WATCHDOG_RESET`.

## Low-voltage behavior

Thresholds are configurable and use hysteresis. Planned logical bands are:

```text
NORMAL
LOW
CRITICAL
EMERGENCY
ABSOLUTE_MINIMUM
```

Rev A thresholds are not considered final until bench and in-vehicle measurements are complete.

## Service mode

SERVICE allows PCG-Core to remain powered with ACC off for diagnostics and logging. It must have a configurable timeout and must still respect an absolute low-voltage cutoff.

## Fault behavior

FAULT records the reason and moves the system to the safest available state. Faults may include invalid voltage, failed boot, failed shutdown, watchdog timeout, sensor failure, or supervisor brownout.
