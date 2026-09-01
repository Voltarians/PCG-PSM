# PCG-PSM Validation Plan

PCG-PSM is not considered permanent vehicle hardware until the relevant tests pass with documented results.

## Bench validation

- Repeated ACC on/off cycling
- Boot only after a real Pi heartbeat is observed
- Graceful shutdown while OBD Atlas logging is active
- Storage flush before power removal
- Hard shutdown timeout when Linux does not acknowledge
- Watchdog recovery from missing/frozen heartbeat
- Service-mode timeout
- Low-voltage warning/shutdown with hysteresis
- Power interruption during each supervisor state
- Main 5 V rail stability under sustained Pi + USB load
- Thermal soak of DC/DC converter and switching hardware
- Supervisor latch release and standby-current measurement

## Vehicle validation

- Normal startup through Volt wake/ACC transitions
- No nuisance resets during vehicle state changes
- Stable 12 V and 5 V rails
- Negligible PCG standby draw after shutdown
- Clean OBD Atlas log/database closure
- No interference with factory BCM or negative-terminal battery-current sensing
- Recovery after abnormal Pi shutdown
- Service mode with ACC off

## Evidence

For each test record:

- date
- hardware revision
- firmware commit
- Pi-service commit
- supply/input conditions
- measured voltage/current/temperature where relevant
- expected behavior
- actual behavior
- pass/fail
- notes and corrective action
