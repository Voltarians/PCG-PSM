# PCG-PSM Rev A.1 Bill of Materials

Status: **Rev A.1 baseline frozen for bench build.** Parts may only be changed after documenting the reason and retesting the affected protection/power function.

The design uses the existing Arduino Uno as the supervisor and keeps the high-current 5 V converter external to the future HAT. Whole-vehicle 12 V current remains sourced from the Volt OEM battery-current data where available. The Infineon main power switch provides a local diagnostic current-sense output without adding a series current shunt.

## Power-path selections

| Ref | Qty | Part | Function | Key rating / reason | Approx. 1-off USD |
| --- | ---: | --- | --- | --- | ---: |
| F1 | 1 | Littelfuse 0287010.U / 0287010.PXCN | Source fuse | ATO/ATC automotive, 10 A | ~0.44 |
| U1 | 1 | TI LM74610QDGKRQ1 | Reverse-battery ideal-diode controller | AEC-Q100 automotive controller | ~2.59 |
| Q1 | 1 | Nexperia BUK9Y8R8-60ELX | Ideal-diode N-MOSFET | AEC-Q101, 60 V, low RDS(on) | ~2.54 |
| D1 | 1 | Littelfuse SLD8S18A | Load-dump / surge TVS | AEC-Q101 automotive load-dump TVS | ~4.53 |
| U2 | 1 | Infineon BTS500601TEAAUMA2 | Main 12 V high-side power switch | Automotive PROFET with protection and analog current sense | ~4.40 |
| PS1 | 1 | Mean Well DDR-60G-5 | Main PCG 5 V converter | 9–36 V input, 4.5–5.5 V adjustable output, 10.8 A / 54 W | ~35.83 current alternate-distributor observation |

Set PS1 so the Raspberry Pi sees **about 5.10 V at its power input under representative load**. Do not route the full Pi/peripheral current through the Raspberry Pi GPIO header.

## Supervisor / latch selections

| Ref | Qty | Part | Function | Notes | Approx. USD |
| --- | ---: | --- | --- | --- | ---: |
| Q2 | 1 | Vishay SQ7415CENW-T1_GE3 | Supervisor self-latch P-MOSFET | AEC-Q101, -60 V | ~1.46 |
| U3 | 1 | onsemi NCV7805BDTRKG | Uno 5 V supervisor regulator | Automotive AEC-Q100, 5 V / 1 A | ~0.62 |
| Q3,Q4 | 2 | 2N3904 or MMBT3904 | ACC wake and Uno self-hold | documented 40 V or higher NPNs | ~0.28 pair |
| DZ1 | 1 | BZD27C12PWH or equivalent 12 V automotive Zener | Q2 VGS clamp | AEC-Q101 preferred | ~0.53 |
| SW1 | 1 | DPST momentary normally-open switch | Manual service wake + D8 sense | pole A wakes latch; pole B grounds service-sense input | varies |

Q2 gate network is **R5 100 kΩ source-to-gate pull-up + DZ1 source/gate clamp + R4 4.7 kΩ between Gate and LATCH_PULLDOWN**. R4 is not optional; it limits pull-down/Zener current.

## Pi / Uno isolated interface

| Ref | Qty | Part | Function | Approx. USD |
| --- | ---: | --- | --- | ---: |
| U4 | 1 | Lite-On LTV-847, DIP-16 | Four-channel optocoupler | ACC sense, shutdown request, heartbeat, safe-poweroff acknowledgement | ~1.80 |
| D2,D3 | 2 | 1N4148 or documented equivalent | Q3 reverse B-E clamp and ACC-opto reverse clamp | small-signal diode |
| D4 | 1 | onsemi SBAT54SLT1G | A0 ADC dual-Schottky rail clamp | automotive AEC-Q101 version; pin 1 GND, pin 3 A0, pin 2 SUPERVISOR_5V | add to Rev A.1 order |

LTV-847 pin contract:

- CH1 LED 1/2, transistor C/E 16/15 — ACC sense
- CH2 LED 3/4, transistor C/E 14/13 — Uno shutdown request to Pi GPIO17
- CH3 LED 5/6, transistor C/E 12/11 — Pi GPIO27 heartbeat to Uno
- CH4 LED 7/8, transistor C/E 10/9 — Pi GPIO22 safe-poweroff acknowledgement to Uno

## Resistors — Rev A.1 exact values

| Ref(s) | Qty | Value / rating | Function |
| --- | ---: | --- | --- |
| R1 | 1 | 10 kΩ | Uno D6 to U2 IN series |
| R2 | 1 | 100 kΩ | U2 IN pull-down |
| R3 | 1 | 470 Ω 1% | BTS50060 IS sense-to-ground |
| R4 | 1 | 4.7 kΩ | Q2 gate pull-down current limiter |
| R5 | 1 | 100 kΩ | Q2 gate-to-source pull-up |
| R6 | 1 | 47 kΩ, 0.5 W | ACC hardware-wake base resistor |
| R7 | 1 | 100 kΩ | Q3 base-emitter pull-down |
| R8 | 1 | 4.7 kΩ | Uno D7 to Q4 base |
| R9 | 1 | 100 kΩ | Q4 base-emitter pull-down |
| R10 | 1 | 4.7 kΩ, 0.5 W | ACC-sense optocoupler LED |
| R11 | 1 | 10 kΩ | ACC-sense output pull-up |
| R12 | 1 | 180 kΩ 1% | vehicle-voltage divider upper |
| R13 | 1 | 22 kΩ 1% | vehicle-voltage divider lower |
| R14 | 1 | 10 kΩ | A0 series protection |
| R15,R16 | 2 | 47 kΩ 1% | main-5-V divider |
| R17 | 1 | 10 kΩ 1% | NTC divider fixed resistor |
| R18,R20,R22 | 3 | 1 kΩ | Pi/Uno optocoupler LED resistors |
| R19,R21,R23 | 3 | 10 kΩ | optocoupler transistor pull-ups |

SW1 service sense uses the Uno D8 `INPUT_PULLUP`; no separate R24 is required in Rev A.1.

## Capacitors — Rev A.1 exact values

| Ref | Value | Function |
| --- | --- | --- |
| C1 | 470 µF / 50 V low-ESR | protected 12 V bulk |
| C2 | 1 µF / 50 V ceramic or film | protected 12 V bypass |
| C3 | 100 nF / 50 V ceramic | protected 12 V HF bypass |
| C4 | 2.2 µF X7R / 16 V | LM74610 charge-pump capacitor, directly pins 7–1 |
| C5 | 4.7 nF | BTS50060 IS diagnostic filter |
| C6 | 0.33 µF | NCV7805 input bypass |
| C7 | 10 µF / 35 V | supervisor-regulator input bulk |
| C8 | 100 nF | NCV7805 output bypass |
| C9 | 100 µF / 10 V or higher | supervisor 5 V bulk |
| C10 | 100 nF | vehicle-voltage ADC filter |
| C11 | 100 nF | main-5-V ADC filter |

## Sensor

| Ref | Part | Function |
| --- | --- | --- |
| TH1 | 10 kΩ NTC, B3950 acceptable for Rev A | power-area temperature; calibrate in firmware |

## Connector contract

| Connector | Pin(s) | Function |
| --- | --- | --- |
| J1 | 1 VBAT+, 2 GND | protected-power input |
| J2 | 1 ACC, 2 GND | vehicle control input |
| J3 | 1 SWITCHED_12V, 2 GND | external DDR-60G-5 input |
| J4 | 1 MAIN_5V, 2 GND | 5 V sense only |
| J7 | 1 Pi 3V3, 2 Pi GND, 3 GPIO17, 4 GPIO27, 5 GPIO22 | isolated Pi signal header; no Pi 5 V |

## Wiring

- 12 V source and main converter input: **14–16 AWG copper**, final size by run length and temperature.
- 5 V high-current rail: **14 AWG preferred**, short as practical.
- ACC/logic/telemetry: **20–22 AWG** for Rev A harness.
- F1 must be near the vehicle 12 V source.
- Use one intentional PCG star-ground point.
- Bond PS1 output negative to the PCG/vehicle ground at the intentional star point so USB/CAN adapters do not become the accidental bonding path.

## Cost target

The specialty electronics remain in roughly the **$55–65 class before all shipping**, with PS1 the dominant item. Common passives, fuse holder, DPST service button, connectors, wire, and thermistor can raise a fully purchased Rev A.1 build into roughly the **$70–90 range**. Existing Uno, Pi, cooler, enclosure, bulk wire, and USB/CAN hub are excluded.

See `PROCUREMENT.md` for the current buying plan.

## Release rule

This BOM is a **buildable Rev A.1 bench design**, not an automotive production approval. Permanent installation remains gated on reverse-polarity/transient testing, repeated boot/shutdown testing, watchdog testing, thermal load testing, measured standby current, and in-vehicle validation.