# Rev A.1 Hardware Prototype

Rev A.1 uses the existing Arduino Uno as the development supervisor for PCG-PSM. The first bench-build electrical architecture is now frozen.

- Graphical schematic set: [`schematic/README.md`](schematic/README.md)
- Exact pin/net contract: [`schematic.md`](schematic.md)
- BOM: [`../bom/README.md`](../bom/README.md)
- Procurement: [`../bom/PROCUREMENT.md`](../bom/PROCUREMENT.md)

## Goals

- prove ignition/ACC sensing
- prove self-latching supervisor power
- control the 12 V-side smart switch feeding the 5 V converter
- establish isolated Uno / Raspberry Pi signaling
- measure protected 12 V and main 5 V rails
- read diagnostic local PCG current from the BTS50060
- monitor power-area temperature
- implement deterministic service/manual wake
- validate graceful shutdown, hardware safe-poweroff acknowledgement, and watchdog recovery

## Frozen Uno I/O for Rev A.1

| Uno pin | Function | Electrical convention |
| --- | --- | --- |
| D2 | ACC / ignition sense | active LOW through U4A |
| D3 | Pi shutdown request | HIGH lights U4B; GPIO17 receives LOW |
| D4 | Pi safe-poweroff acknowledgement | active LOW from GPIO22 through U4D |
| D5 | Pi heartbeat | edge-based from GPIO27 through U4C |
| D6 | Main power enable | HIGH enables BTS50060 |
| D7 | Supervisor self-hold | HIGH holds Q2 latch |
| D8 | Service-mode input | active LOW from SW1 pole B; `INPUT_PULLUP` |
| A0 | Protected 12 V system voltage | 180 kΩ / 22 kΩ divider + 10 kΩ series + SBAT54SLT1G clamp |
| A1 | Main 5 V rail voltage | 47 kΩ / 47 kΩ divider |
| A2 | Power-system temperature | 10 kΩ / 10 kΩ NTC divider |
| A3 | Main-switch diagnostic current | BTS50060 IS through 470 Ω + 4.7 nF |

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

The DDR-60G-5 accepts 9–36 V and supplies up to 10.8 A / 54 W at nominal 5 V. Rev A.1 targets about **5.10 V measured at the Pi under representative load**.

## Supervisor architecture

Q2 is a self-latching P-channel MOSFET. ACC can wake the latch through Q3 even while the Uno is off. SW1 is DPST momentary N.O.: pole A wakes the hardware latch and pole B pulls D8 low. The firmware records the service-wake reason immediately, so the button does not need to remain held during Linux boot. Uno D7 then maintains the latch through Q4.

After normal Linux shutdown, Raspberry Pi `gpio-poweroff` on GPIO22 drives U4D, producing the active-low D4 acknowledgement. Only then does the Uno disable the main switch and release its self-hold. A hard timeout remains the independent fallback.

## Electrical rules

- Never apply vehicle 12 V directly to an Arduino digital or analog pin.
- Never apply Uno 5 V logic directly to Raspberry Pi GPIO.
- ACC/service wake must work without supervisor power already present.
- Use the optocoupler interface for Pi/Uno handshake signals.
- Switch the high-current load on the 12 V side.
- Do not route full Pi/peripheral current through the Raspberry Pi GPIO header.
- Bond the isolated main 5 V negative to vehicle/PCG ground at one intentional star point.
- Place the source fuse close to the 12 V source.

## Rev A.1 release gate

Rev A.1 is **not approved for permanent vehicle installation yet**. Reverse-polarity, transient, startup/shutdown, watchdog, low-voltage, thermal, high-load, storage-integrity, and standby-current tests must pass first. Exact low-voltage thresholds remain deliberately unfrozen until actual Volt 12 V behavior is measured.