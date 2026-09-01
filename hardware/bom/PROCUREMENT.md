# PCG-PSM Rev A.1 Procurement Plan

Date checked: **2026-09-01**

## Procurement decision

Use **DigiKey for the automotive control/protection parts** and buy the **Mean Well DDR-60G-5 separately from an authorized Mean Well distributor**. DigiKey's DDR-60G-5 listing is no longer actionable for a new unit, so PS1 stays on a separate order.

The preferred observed PS1 source remains **Power Supply Mall**, which identifies itself as an authorized Mean Well distributor. Record the delivered checkout price before ordering.

Use existing shop stock for ordinary resistors, capacitors, wire, heat-shrink, terminals, and NTCs only when their values/ratings match the Rev A.1 BOM.

## Specialty-parts order

| Ref | Qty | Manufacturer part | Notes |
| --- | ---: | --- | --- |
| U1 | 1 | TI LM74610QDGKRQ1 | exact part |
| Q1 | 1 | Nexperia BUK9Y8R8-60ELX | exact part |
| D1 | 1 | Littelfuse SLD8S18A | exact automotive load-dump TVS |
| U2 | 1 | Infineon BTS500601TEAAUMA2 | exact smart high-side switch |
| Q2 | 1 | Vishay SQ7415CENW-T1_GE3 | exact AEC-Q101 -60 V P-MOSFET |
| U3 | 1 | onsemi NCV7805BDTRKG | automotive 5 V regulator |
| U4 | 1 | Lite-On LTV-847, DIP-16 | quad optocoupler |
| D4 | 1 | onsemi SBAT54SLT1G | **new Rev A.1 item**; AEC-Q101 dual Schottky A0 clamp |
| F1 | 1+ spare | Littelfuse 10 A ATO/ATC | use appropriate fuse holder |
| Q3,Q4 | 2 | 2N3904 / documented equivalent | 40 V or higher |
| DZ1 | 1 | BZD27C12PWH or equivalent 12 V automotive Zener | Q2 VGS clamp |
| D2,D3 | 2 | 1N4148 or documented equivalent | small-signal clamps |

## Main converter — separate order

| Ref | Qty | Part | Preferred source | Last observed price | Notes |
| --- | ---: | --- | --- | ---: | --- |
| PS1 | 1 | Mean Well DDR-60G-5 | authorized Mean Well distributor / Power Supply Mall | about $35.83 | shipping calculated at checkout |

PS1 requirements remain 9–36 V input, 4.5–5.5 V adjustable output, 10.8 A / 54 W capability, isolated output, and suitable temperature derating. Set it so the **Pi sees about 5.10 V at its actual power input under representative load**.

## Rev A.1 resistor shopping list

Buy several spares of each common value.

- 180 kΩ 1%, x1 minimum
- 100 kΩ, x4 minimum
- 47 kΩ 1%, x2 minimum for R15/R16
- 47 kΩ 0.5 W, x1 minimum for R6
- 22 kΩ 1%, x1 minimum
- 10 kΩ 1%, x5 minimum where precision is called out
- 10 kΩ standard, x3+ minimum for logic pull-ups/series nodes
- 4.7 kΩ standard, x2 minimum for R4/R8
- 4.7 kΩ 0.5 W, x1 minimum for R10
- 1 kΩ, x3 minimum for R18/R20/R22
- 470 Ω 1%, x1 minimum for R3

The exact designator/value mapping is in `README.md` and `../rev-a/schematic.md`.

## Rev A.1 capacitor shopping list

- C1: 470 µF / 50 V low-ESR electrolytic, x1
- C2: 1 µF / 50 V ceramic or film, x1
- C3: 100 nF / 50 V ceramic, x1
- C4: **2.2 µF X7R / 16 V**, x1 — LM74610 charge-pump capacitor
- C5: **4.7 nF**, x1 — BTS50060 diagnostic-filter capacitor
- C6: 0.33 µF, x1
- C7: 10 µF / 35 V, x1
- C8: 100 nF, x1
- C9: 100 µF / 10 V or higher, x1
- C10,C11: 100 nF, x2
- extra 100 nF bypass capacitors, several

## Other hardware

- TH1: 10 kΩ NTC; B3950 acceptable for Rev A if calibrated
- **SW1: DPST momentary normally-open pushbutton** — one pole wakes the latch and one pole grounds Uno D8 service sense
- inline/panel ATO/ATC fuse holder rated comfortably above 10 A
- spare 10 A ATO/ATC fuses
- 14 AWG copper wire for the 5 V high-current path
- 14–16 AWG copper for the 12 V main path
- 20–22 AWG wire for ACC/logic
- insulated crimp terminals/ferrules as appropriate
- heat-shrink and harness protection
- perfboard/prototype board for Rev A low-current circuitry, unless a dedicated prototype PCB is made
- 5-pin low-current Pi signal connector/header for J7
- 2-pin connectors for J1/J2/J3/J4 sized appropriately for their actual current duty

## Parts already available / excluded

- Arduino Uno — already owned
- 12 V / 10 A bench source — already owned
- Raspberry Pi 5 8 GB — separate PCG-Core purchase
- Raspberry Pi Active Cooler — separate PCG-Core purchase
- enclosure — select after Rev A geometry is established
- powered USB/CAN hub — separate PCG-Core architecture item

## Budget

The prior specialty-parts subtotal was around $19–20 before PS1. Rev A.1 adds the SBAT54SLT1G and makes the DPST service switch requirement explicit. With PS1 around the mid-$30 range, expect the **specialty electrical core to remain around $55–65 before all shipping/tax**.

If every passive, fuse holder, DPST switch, connector, thermistor, and harness item must also be purchased, budget approximately **$70–90 before tax**. Existing shop stock can reduce that significantly.

## Purchase rules

1. Buy single/cut-tape quantities for Rev A.1, plus inexpensive spares where sensible.
2. Match manufacturer part numbers exactly for U1, Q1, D1, U2, Q2, D4, U3, and PS1.
3. Do not substitute a generic buck converter for PS1 without reviewing transient, thermal, input-range, isolation, and output-adjustment behavior.
4. Do not replace the automotive TVS, reverse-polarity stage, or A0 clamp with anonymous consumer parts.
5. Verify Q2 and Q1 package/footprint pinout before soldering.
6. Record actual invoice prices and received manufacturer markings before assembly.
7. Rev A.1 remains a validation build; permanent vehicle installation is gated on electrical and thermal testing.