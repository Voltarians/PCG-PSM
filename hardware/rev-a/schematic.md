# PCG-PSM Rev A Schematic / Wiring Plan

This document is the schematic-level net plan for the Arduino Uno Rev A prototype. It is intended to be translated into a formal KiCad schematic after the first bench build verifies polarity and timing assumptions.

## 1. Main protected 12 V power path

```text
VEHICLE/BENCH +12V
      |
     F1  10 A ATO/ATC
      |
      +---- U1 LM74610-Q1 + Q1 BUK9Y8R8-60EL ----+---- PROTECTED_12V
                                                   |
                                                   +---- D1 SLD8S18A ---- GND
                                                   |
                                                   +---- C1 470uF/50V
                                                   +---- C2 1uF/50V
                                                   +---- C3 100nF/50V
                                                   |
                                                   +---- U2 BTS50060 smart high-side switch
                                                             |
                                                             +---- MAIN_12V_SW
                                                                      |
                                                                      +---- PS1 Mean Well DDR-60G-5
                                                                               |
                                                                               +---- MAIN_5V
                                                                               +---- MAIN_5V_GND
```

Bond MAIN_5V_GND to PCG/vehicle ground at one intentional star point.

### U2 control

```text
Uno D6 ---- 10k ---- U2 IN
                     |
                    100k
                     |
                    GND
```

D6 HIGH = main converter enabled.

### U2 diagnostic current sense

```text
U2 IS ----+---- Uno A3
          |
         470R 1%
          |
         GND
          |
        100nF
          |
         GND
```

The A3 reading is diagnostic only. Calibrate against an external ammeter before using it quantitatively.

## 2. Supervisor self-latching supply

```text
PROTECTED_12V ---- Q2 P-MOSFET -------------------- SUPERVISOR_12V
                      |
                      +---- 100k gate-to-source pull-up
                      +---- 12V Zener gate-to-source clamp
                      |
                      +---- Q3 collector (ACC hardware wake)
                      +---- Q4 collector (Uno SELF_HOLD)
                      +---- SW1 service-wake momentary pull-down

SUPERVISOR_12V ---- U3 NCV7805 ---- SUPERVISOR_5V ---- Arduino Uno 5V
```

### Q3 ACC wake

```text
ACC ---- 47k ---- Q3 base
                  |
                 100k
                  |
                 GND

Q3 emitter -> GND
Q3 collector -> Q2 gate
1N4148 reverse B-E clamp across Q3 base/emitter
```

ACC can therefore power the supervisor even when the Uno is completely off.

### Q4 Uno self-hold

```text
Uno D7 ---- 4.7k ---- Q4 base
                       |
                      100k
                       |
                      GND

Q4 emitter -> GND
Q4 collector -> Q2 gate
```

D7 HIGH holds supervisor power on after the ACC wake path disappears. D7 LOW releases the latch after shutdown.

### Service wake

SW1 is a momentary pushbutton that pulls the Q2 gate low through a current-limiting resistor. The Uno asserts D7 immediately after boot so the button can be released.

## 3. Ignition/ACC logic sense

ACC wake and ACC sensing are deliberately separate.

```text
ACC ---- 4.7k / 0.5W ---- U4A LED ---- GND
                     |          |
                     +-- 1N4148 antiparallel protection

SUPERVISOR_5V ---- 10k ----+---- Uno D2
                            |
                         U4A transistor
                            |
                           GND
```

Result: **D2 LOW = ACC active**.

## 4. Pi / Uno isolated handshake

Use remaining LTV-847 channels.

### Uno -> Pi shutdown request (U4B)

```text
Uno D3 ---- 1k ---- U4B LED ---- GND

Pi 3.3V ---- 10k ----+---- Pi GPIO SHUTDOWN_REQ
                      |
                  U4B transistor
                      |
                     Pi GND
```

D3 HIGH causes the Pi GPIO to go LOW. Pi software treats LOW as shutdown requested.

### Pi -> Uno heartbeat (U4C)

```text
Pi GPIO HEARTBEAT ---- 1k ---- U4C LED ---- Pi GND

SUPERVISOR_5V ---- 10k ----+---- Uno D5
                            |
                        U4C transistor
                            |
                           GND
```

Heartbeat is edge-based; inversion is irrelevant.

### Pi -> Uno shutdown acknowledgement (U4D)

```text
Pi GPIO SHUTDOWN_ACK ---- 1k ---- U4D LED ---- Pi GND

SUPERVISOR_5V ---- 10k ----+---- Uno D4
                            |
                        U4D transistor
                            |
                           GND
```

Result: **D4 LOW = shutdown acknowledged**.

## 5. Voltage and temperature sensing

### A0 protected vehicle voltage

```text
PROTECTED_12V ---- 180k 1% ----+---- Uno A0
                                |
                               22k 1%
                                |
                               GND
                                |
                              100nF
                                |
                               GND
```

### A1 main 5 V rail

```text
MAIN_5V ---- 47k 1% ----+---- Uno A1
                         |
                        47k 1%
                         |
                        GND
                         |
                       100nF
                         |
                        GND
```

### A2 temperature

```text
SUPERVISOR_5V ---- 10k 1% ----+---- Uno A2
                               |
                          10k NTC B3950
                               |
                              GND
```

Place the NTC near the main switching/protection area rather than next to the Arduino.

## 6. Main 5 V distribution

The DDR-60G-5 output should feed a small high-current distribution point rather than the Pi GPIO header.

Recommended branches:

```text
MAIN_5V
  |
  +---- Pi power branch -> short heavy-gauge USB-C power harness
  |
  +---- powered USB/CAN hub branch
  |
  +---- future peripheral branch
```

Set the converter so the Pi receives 5.10 V under representative load. Measure at the Pi end of the harness.

## 7. Grounding

Use one PCG star point joining:

- vehicle 12 V ground
- Uno/supervisor ground
- BTS50060 ground
- protected input ground
- Mean Well isolated output negative
- Pi ground
- USB hub ground

Do not allow the USB/CAN adapters to become the only connection between isolated 5 V negative and vehicle ground.

## 8. Bench bring-up order

1. Build only F1 + reverse protection + TVS and verify forward/reverse behavior with a current-limited bench supply.
2. Add Q2/U3 supervisor latch and verify ACC wake/self-hold/release without the Pi connected.
3. Add U4 optocoupler signaling and verify logic levels with a meter or scope.
4. Add U2 smart high-side switch with a dummy load instead of PS1.
5. Add PS1 and set output under dummy load.
6. Connect Pi only after MAIN_5V stability, overshoot, and shutdown behavior are verified.
7. Add the USB/CAN load after Pi operation is stable.

## 9. Not frozen yet

- Final low-voltage cutoff thresholds
- Final ADC calibration constants
- Final service-mode timeout
- EMI common-mode choke selection
- Exact Pi USB-C power connector implementation
- Enclosure and production connector family

Those items require bench measurements or mechanical layout before they can be frozen.