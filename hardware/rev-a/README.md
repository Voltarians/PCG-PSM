# Rev A Hardware Prototype

Rev A uses the existing Arduino Uno as the development supervisor for PCG-PSM. The power architecture and primary part selections are now frozen for the first bench build; see `hardware/bom/README.md` and `hardware/rev-a/schematic.md`.

## Goals

- Prove ignition/ACC sensing
- Prove self-latching supervisor power
- Control the main 12 V-side switch feeding the 5 V DC/DC converter
- Establish isolated 5 V/3.3 V signaling with Raspberry Pi 5
- Measure vehicle 12 V and main 5 V rails
- Read diagnostic PCG current from the smart high-side switch
- Add power-system temperature sensing
- Implement service/manual wake
- Validate graceful shutdown and watchdog recovery

## Frozen Uno I/O for Rev A

| Uno pin | Function | Electrical convention |
| --- | --- | --- |
| D2 | ACC / ignition sense | Active LOW through optocoupler |
| D3 | Pi shutdown request | HIGH lights optocoupler; Pi receives active LOW |
| D4 | Pi shutdown acknowledgement | Active LOW through optocoupler |
| D5 | Pi heartbeat | Edge-based through optocoupler |
| D6 | Main power enable | HIGH enables BTS50060 |
| D7 | Supervisor self-hold / latch | HIGH holds supervisor power |
| D8 | Service-mode input | Active LOW pushbutton/input |
| A0 | Protected 12 V system voltage | 180k/22k divider |
| A1 | Main 5 V rail voltage | 47k/47k divider |
| A2 | Power-system temperature | 10k NTC divider |
| A3 | Main switch diagnostic current sense | BTS50060 IS through 470 ohm sense resistor |

## Main power architecture

```text
12 V source
  -> 10 A fuse
  -> LM74610-Q1 + 60 V N-MOSFET reverse-polarity stage
  -> SLD8S18A automotive load-dump TVS
  -> BTS50060 automotive smart high-side switch
  -> Mean Well DDR-60G-5
  -> regulated main 5 V distribution
```

The DDR-60G-5 is rated 9–36 V input and 10.8 A / 54 W output, with output adjustment from 4.5 to 5.5 V. The Rev A target is 5.10 V measured at the Pi under representative load.

## Supervisor architecture

A P-channel MOSFET latch feeds an automotive 5 V regulator and the Uno. ACC or the service switch wakes the latch before the Uno is powered. The Uno then asserts SELF_HOLD. After graceful Pi shutdown the Uno releases the latch, removing its own supply and minimizing standby draw.

## Electrical rules

- Never apply vehicle 12 V directly to an Arduino digital/analog pin.
- Never connect a 5 V Uno output directly to Raspberry Pi GPIO.
- ACC wake must work without supervisor power already present.
- Use the optocoupler interface for Pi/Uno handshake signals.
- Switch the high-current load on the 12 V side.
- Do not route the full Pi/peripheral load through the Raspberry Pi GPIO header.
- Bond the isolated main 5 V negative to vehicle/PCG ground at one intentional star point.
- Place the source fuse close to the 12 V source.

## Rev A release gate

Rev A is not ready for permanent vehicle installation until reverse-polarity, transient, startup/shutdown, watchdog, low-voltage, thermal, high-load, and standby-current tests pass. Exact low-voltage thresholds remain intentionally unfrozen until Volt 12 V behavior is measured.