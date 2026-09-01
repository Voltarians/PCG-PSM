# PCG-PSM Rev A.1 Schematic Contract

Status: **bench-build schematic frozen for Rev A.1**. Permanent in-vehicle use remains gated on validation.

The graphical sheets are in `hardware/rev-a/schematic/`. This document is the exact electrical contract behind those drawings.

## Sheet 1 — protected 12 V path and main 5 V converter

```text
J1-1 VBAT+
  -> F1 10 A ATO/ATC
  -> Q1 + U1 reverse-battery ideal-diode stage
  -> PROTECTED_12V
       -> D1 SLD8S18A TVS to PCG_GND
       -> C1 470 uF / 50 V to PCG_GND
       -> C2 1 uF / 50 V to PCG_GND
       -> C3 100 nF / 50 V to PCG_GND
       -> U2 BTS50060-1TEA smart high-side switch
  -> SWITCHED_12V
  -> J3-1
  -> PS1 Mean Well DDR-60G-5 pin 5 +VIN

J1-2 PCG_GND -> J3-2 -> PS1 pin 6 -VIN
```

### U1 — TI LM74610QDGKRQ1, VSSOP-8

| Pin | Name | Rev A.1 connection |
| ---: | --- | --- |
| 1 | VCAPL | C4 low side |
| 2 | Gate Pull Down | Q1 gate |
| 3 | NC | no connection |
| 4 | Anode | Q1 source / fused input side |
| 5 | NC | no connection |
| 6 | Gate Drive | Q1 gate |
| 7 | VCAPH | C4 high side |
| 8 | Cathode | Q1 drain / `PROTECTED_12V` side |

**C4 = 2.2 uF X7R, 16 V**, directly between U1 pins 7 and 1.

### Q1 — Nexperia BUK9Y8R8-60ELX, LFPAK56

- pins 1–3: Source
- pin 4: Gate
- exposed mounting base: Drain

Q1 source is on the U1 ANODE/fused-input side; Q1 drain is on the U1 CATHODE/protected-bus side.

### U2 — Infineon BTS50060-1TEA

| Pin | Function | Rev A.1 connection |
| ---: | --- | --- |
| 1 | GND | PCG_GND |
| 2 | IN | Uno D6 through R1 |
| 3 / tab | OUT | `SWITCHED_12V` / J3-1 |
| 4 | IS | Uno A3 diagnostic current-sense node |
| 5 | VS | `PROTECTED_12V` |

U2 control:

```text
Uno D6 MAIN_EN -> R1 10 kΩ -> U2 pin 2 IN
                                  |
                              R2 100 kΩ
                                  |
                                GND
```

U2 diagnostic current sense:

```text
U2 pin 4 IS ----+---- Uno A3
                |
              R3 470 Ω 1%
                |
               GND
                |
             C5 4.7 nF
                |
               GND
```

R3 keeps the specified fault-sense current within the Uno ADC range. A3 is a diagnostic/calibrated PCG-current estimate only; whole-vehicle 12 V current remains sourced from the Volt factory battery-current data when available.

### PS1 — Mean Well DDR-60G-5

| Terminal | Function |
| ---: | --- |
| 1, 2 | -VOUT |
| 3, 4 | +VOUT |
| 5 | +VIN |
| 6 | -VIN |

Adjust PS1 so the **Raspberry Pi power input measures about 5.10 V under representative load**. The high-current 5 V path does not pass through the Uno shield or Raspberry Pi GPIO header.

## Sheet 2 — self-latching supervisor, ACC, Uno, and analog sensing

### Q2 supervisor latch

Q2 is Vishay `SQ7415CENW-T1_GE3`, automotive P-channel 60 V MOSFET.

- pin 1: Gate
- pins 2–4: Source
- pins 5–8 / exposed drain pad: Drain

```text
PROTECTED_12V -> Q2 Source
Q2 Drain      -> SUPERVISOR_12V

Q2 Source ---- R5 100 kΩ ---- Q2 Gate
Q2 Source ---- DZ1 12 V ----- Q2 Gate
                              |
Q2 Gate ------- R4 4.7 kΩ ----+---- LATCH_PULLDOWN
```

DZ1 cathode goes to Q2 Source; DZ1 anode goes to Q2 Gate. **R4 is required** to limit pull-down/Zener current.

`LATCH_PULLDOWN` is pulled to ground by any of:

- Q3 collector — ACC hardware wake
- Q4 collector — Uno self-hold
- SW1 pole A — manual service wake

### U3 — NCV7805BDTRKG

| Pin | Connection |
| ---: | --- |
| 1 IN | SUPERVISOR_12V |
| 2 / tab | PCG_GND |
| 3 OUT | SUPERVISOR_5V |

Input bypass: C6 0.33 uF plus C7 10 uF / 35 V. Output bypass: C8 100 nF plus C9 100 uF / 10 V.

`SUPERVISOR_5V` powers the Uno through its **5V pin**. Do not connect ordinary USB VBUS at the same time during external-power bench testing; use a data-only/isolated programming cable when appropriate.

### Q3 — hardware ACC wake

```text
ACC -> R6 47 kΩ 0.5 W -> Q3 base
                            |
                         R7 100 kΩ
                            |
                           GND

Q3 collector -> LATCH_PULLDOWN
Q3 emitter   -> GND
D2 1N4148 reverse B-E clamp across Q3
```

This path can wake the supervisor when the Uno is completely unpowered.

### SW1 — service wake

SW1 is **DPST, momentary, normally open**.

- pole A: `LATCH_PULLDOWN` to GND
- pole B: `SERVICE_SENSE_N` to GND
- `SERVICE_SENSE_N` goes to Uno D8, configured with `INPUT_PULLUP`

The firmware latches the service-boot decision immediately so the button does not have to remain pressed while Linux boots.

### Q4 — Uno self-hold

```text
Uno D7 SELF_HOLD -> R8 4.7 kΩ -> Q4 base
                                      |
                                   R9 100 kΩ
                                      |
                                     GND

Q4 collector -> LATCH_PULLDOWN
Q4 emitter   -> GND
```

D7 HIGH keeps the supervisor alive after the wake source disappears. After safe Pi shutdown, D7 LOW releases Q2 and removes supervisor power.

### U4A — ACC logic sense

LTV-847 channel 1:

```text
ACC -> R10 4.7 kΩ 0.5 W -> U4 pins 1/2 LED -> GND
D3 1N4148 antiparallel across LED pins 1/2

SUPERVISOR_5V -> R11 10 kΩ -> Uno D2 / U4 pin 16 collector
U4 pin 15 emitter -> GND
```

Result: **Uno D2 LOW = ACC active**.

### Uno analog channels

**A0 — protected vehicle voltage**

```text
PROTECTED_12V -> R12 180 kΩ 1% -> divider node
                                        |
                                  R13 22 kΩ 1%
                                        |
                                       GND

C10 100 nF from divider node to GND
divider node -> R14 10 kΩ -> Uno A0
```

D4 is `SBAT54SLT1G`, automotive dual Schottky clamp:

- pin 1 -> GND
- pin 3 -> Uno A0 clamp node
- pin 2 -> SUPERVISOR_5V

**A1 — main 5 V rail sense**

```text
J4-1 MAIN_5V -> R15 47 kΩ 1% -> Uno A1 node
                                      |
                                R16 47 kΩ 1%
                                      |
                                     GND
C11 100 nF from A1 node to GND
J4-2 -> PCG_GND
```

**A2 — temperature**

```text
SUPERVISOR_5V -> R17 10 kΩ 1% -> Uno A2 node
                                         |
                                  TH1 10 kΩ NTC B3950
                                         |
                                        GND
```

Mount TH1 near the power/protection area, not beside the Uno.

## Sheet 3 — Raspberry Pi 5 isolated GPIO handshake

### J7 Pi signal header

J7 carries **signals only; no Pi 5 V power**.

| J7 | Pi physical pin | Signal | Direction |
| ---: | ---: | --- | --- |
| 1 | 1 | 3.3 V reference | Pi -> interface pull-ups only |
| 2 | 6 | GND | Pi-side optocoupler return |
| 3 | 11 / GPIO17 | shutdown request | Uno -> Pi |
| 4 | 13 / GPIO27 | heartbeat | Pi -> Uno |
| 5 | 15 / GPIO22 | safe-to-cut-power | Pi -> Uno |

### U4 complete LTV-847 channel map

| Channel | LED pins | transistor collector/emitter | Assignment |
| --- | --- | --- | --- |
| U4A | 1 / 2 | 16 / 15 | ACC sense |
| U4B | 3 / 4 | 14 / 13 | Uno shutdown request -> GPIO17 |
| U4C | 5 / 6 | 12 / 11 | GPIO27 heartbeat -> Uno D5 |
| U4D | 7 / 8 | 10 / 9 | GPIO22 safe-power-off -> Uno D4 |

### U4B — shutdown request

```text
Uno D3 -> R18 1 kΩ -> U4B LED pins 3/4 -> Uno GND
Pi 3.3V -> R19 10 kΩ -> GPIO17 / U4B collector pin 14
U4B emitter pin 13 -> Pi GND
```

Uno D3 HIGH causes GPIO17 LOW.

Pi configuration:

```text
dtoverlay=gpio-shutdown,gpio_pin=17,active_low=1,gpio_pull=up,debounce=100
```

### U4C — heartbeat

```text
GPIO27 -> R20 1 kΩ -> U4C LED pins 5/6 -> Pi GND
SUPERVISOR_5V -> R21 10 kΩ -> Uno D5 / U4C collector pin 12
U4C emitter pin 11 -> Uno GND
```

The PCG-PSM Pi service toggles GPIO27 at 1 Hz. Uno BOOT requires at least one real edge; RUN faults after approximately 15 seconds without an edge.

### U4D — safe-to-cut-power acknowledgement

```text
GPIO22 -> R22 1 kΩ -> U4D LED pins 7/8 -> Pi GND
SUPERVISOR_5V -> R23 10 kΩ -> Uno D4 / U4D collector pin 10
U4D emitter pin 9 -> Uno GND
```

Pi configuration:

```text
dtoverlay=gpio-poweroff,gpiopin=22
```

The supervisor must treat the `gpio-poweroff` hardware signal as the normal **safe-to-remove-power acknowledgement**, rather than relying on an early userspace flag. A hard timeout remains as the independent fallback if Linux never reaches safe poweroff.

## Connector contract

| Connector | Pin | Function |
| --- | ---: | --- |
| J1 | 1 | VBAT+ |
| J1 | 2 | PCG_GND |
| J2 | 1 | ACC |
| J2 | 2 | PCG_GND |
| J3 | 1 | SWITCHED_12V to PS1 +VIN |
| J3 | 2 | PCG_GND to PS1 -VIN |
| J4 | 1 | MAIN_5V sense only |
| J4 | 2 | PCG_GND |
| J7 | 1 | Pi 3.3 V / physical 1 |
| J7 | 2 | Pi GND / physical 6 |
| J7 | 3 | GPIO17 / physical 11 |
| J7 | 4 | GPIO27 / physical 13 |
| J7 | 5 | GPIO22 / physical 15 |

## Grounding and high-current distribution

Use one intentional PCG star point joining vehicle 12 V ground, supervisor/Uno ground, U2 ground, protected-input return, PS1 output negative, Pi ground, and powered USB/CAN-hub ground. Do not allow a USB or CAN adapter to become the accidental sole bond between the isolated converter output and vehicle ground.

The DDR-60G-5 high-current output should branch to the Pi and powered USB/CAN subsystem with short heavy-gauge conductors. The exact Pi USB-C power-harness implementation remains a mechanical/electrical item to freeze after bench voltage-drop testing.

## Rev A.1 bring-up sequence

1. Current-limit the 12 V bench source.
2. Build F1 + U1/Q1 + D1 only and verify normal and reverse-polarity behavior.
3. Add C1–C4 and verify protected-bus startup/transient behavior.
4. Build Q2/U3 latch and verify ACC wake, SW1 service wake, D7 self-hold, and complete power release without the Pi.
5. Verify every U4 channel with a meter/scope before connecting Pi GPIO.
6. Add U2 with a dummy load; verify D6 enable and IS diagnostic behavior.
7. Add PS1 and a dummy 5 V load; verify output, startup overshoot, thermal rise, and shutdown.
8. Connect Pi only after the 5 V rail is proven stable.
9. Validate GPIO17 shutdown, GPIO27 heartbeat loss, and GPIO22 safe-poweroff acknowledgement repeatedly.
10. Add USB/CAN loads and repeat thermal, low-voltage, and shutdown testing.

## Still intentionally not frozen

- final 12 V low/critical/emergency cutoff thresholds
- final ADC calibration constants
- final service timeout
- EMI/common-mode choke selection based on measurement
- final Pi high-current connector/harness
- enclosure and production connector family

Those values require bench or in-vehicle measurements rather than assumptions.