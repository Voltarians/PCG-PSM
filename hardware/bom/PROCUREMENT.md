# PCG-PSM Rev A Procurement Plan

Date checked: **2026-09-01**

## Procurement decision

Use **DigiKey for the automotive control/protection parts** and buy the **Mean Well DDR-60G-5 separately from an authorized Mean Well distributor**. DigiKey's DDR-60G-5 product page currently reports that the converter is no longer available there, so the earlier one-vendor plan is no longer actionable.

The preferred current source for PS1 is **Power Supply Mall**, which identifies itself as an authorized Mean Well distributor and currently lists the DDR-60G-5. Shipping is calculated at checkout; free U.S. shipping starts at $200, so record the delivered price before ordering.

Use existing shop stock for ordinary resistors, capacitors, wire, heat-shrink, terminals, a momentary service switch, and a 10 kΩ NTC where suitable parts are already on hand.

## DigiKey specialty-parts order

| Ref | Qty | Manufacturer part | DigiKey identifier when confirmed | Est. unit price | Est. extended |
| --- | ---: | --- | --- | ---: | ---: |
| U1 | 1 | TI LM74610QDGKRQ1 | 296-43067-1-ND cut tape | $2.59 | $2.59 |
| Q1 | 1 | Nexperia BUK9Y8R8-60ELX | search exact manufacturer part | $2.54 | $2.54 |
| D1 | 1 | Littelfuse SLD8S18A | F8152CT-ND | $4.53 | $4.53 |
| U2 | 1 | Infineon BTS500601TEAAUMA2 | BTS500601TEAAUMA2CT-ND | $4.40 | $4.40 |
| Q2 | 1 | Vishay SQ7415CENW-T1_GE3 | 742-SQ7415CENW-T1_GE3CT-ND | $1.46 | $1.46 |
| U3 | 1 | onsemi NCV7805BDTRKG | search exact manufacturer part | $0.62 | $0.62 |
| U4 | 1 | Lite-On LTV-847, 16-DIP | search exact manufacturer part | $1.80 | $1.80 |
| F1 | 1 | Littelfuse 0287010.PXCN, 10 A ATO | F4199-ND | $0.44 | $0.44 |
| Q3,Q4 | 2 | Diotec 2N3904, TO-92 | 4878-2N3904CT-ND | $0.14 | $0.28 |
| DZ1 | 1 | Taiwan Semiconductor BZD27C12PWH | BZD27C12PWHCT-ND | $0.53 | $0.53 |
| D2/D3 | 2 | 1N4148 or documented equivalent | shop stock or DigiKey | ~$0.10 | ~$0.20 |

**DigiKey specialty-parts subtotal: approximately $19.39.**

DigiKey U.S. shipping currently starts at **$4.99 USPS Ground Advantage**, putting this portion at approximately **$24.38 before tax, tariffs if applicable, and any common passives added to the order.**

## Main converter — separate order

| Ref | Qty | Part | Preferred source | Current observed price | Notes |
| --- | ---: | --- | --- | ---: | --- |
| PS1 | 1 | Mean Well DDR-60G-5 | Power Supply Mall | about $35.83 | Authorized Mean Well distributor; shipping calculated at checkout |

PS1 requirements remain:

- 9–36 V DC input
- 5 V nominal output, adjustable approximately ±10%
- 10.8 A / 54 W output capability
- isolated output
- -40 to +85 °C operating range with applicable derating

Set PS1 so the **Raspberry Pi sees about 5.10 V at its power input under representative load**, not simply 5.10 V at the converter terminals.

### Alternate converter sources observed

- TRC Electronics: approximately $43.82
- Wellforces: approximately $41.08
- Newark: approximately $49.72

Do not buy anonymous marketplace or surplus units merely to save a few dollars on this permanent power component.

## Add only if not already in the shop

These parts are inexpensive and do not justify a separate supplier order. Add them to DigiKey only when suitable values are not already available.

### Resistors

- 180 kΩ, 1%, x1
- 100 kΩ, x4 or more
- 47 kΩ, 1%, x3 or more
- 22 kΩ, 1%, x1
- 10 kΩ, 1%, x6 or more
- 4.7 kΩ, 0.5 W, x2
- 4.7 kΩ standard, x1
- 1 kΩ, x4
- 680 Ω, x4 optional alternate optocoupler LED value
- 470 Ω, 1%, x1

Buy a few spares of every resistor value used in Rev A.

### Capacitors

- 470 µF, 50 V low-ESR electrolytic, x1
- 100 µF, 25 V or higher, x1
- 10 µF, 25 V or higher, x1
- 1 µF, 50 V ceramic/film, x1
- 100 nF, 50 V ceramic, x10

### Other common hardware

- 10 kΩ NTC thermistor; B3950 is acceptable for Rev A if calibrated in firmware
- Momentary normally-open service/wake pushbutton
- Inline or panel ATO/ATC fuse holder rated comfortably above 10 A
- 10 A ATO/ATC spare fuses
- 14 AWG copper wire for the 5 V high-current path
- 14–16 AWG copper wire for the 12 V main path
- 20–22 AWG wire for ACC and logic
- insulated crimp terminals / ferrules as appropriate
- heat-shrink and harness protection
- small prototype board or perfboard for Rev A low-current control circuitry

## Parts already available / excluded from this order

- Arduino Uno — already owned
- 12 V / 10 A bench source — already owned
- Raspberry Pi 5 8 GB — separate PCG-Core purchase
- Raspberry Pi Active Cooler — separate PCG-Core purchase
- enclosure — select after Rev A geometry is established
- USB/CAN hub — separate PCG-Core architecture item

## Expected Rev A cost

Current core parts are approximately:

- DigiKey specialty components: **$19.39**
- Lowest DigiKey shipping: **$4.99**
- DDR-60G-5 at Power Supply Mall: **about $35.83 plus its checkout shipping**

That makes the known pre-tax total **about $60.21 plus the converter seller's shipping** before adding any common parts not already in the shop.

If essentially every resistor, capacitor, thermistor, fuse holder, connector, and small hardware item must also be purchased, budget roughly **$70–85 before tax** depending on shipping and hardware selections.

## Purchase rules

1. Buy cut-tape/single quantities for Rev A, not reels.
2. Match manufacturer part numbers exactly for U1, Q1, D1, U2, Q2, U3, and PS1.
3. Do not substitute a generic buck converter for PS1 without reviewing its transient, thermal, input-range, isolation, and output-adjustment behavior.
4. Do not replace the automotive TVS or reverse-polarity stage with ordinary consumer parts.
5. Record actual invoice prices and received manufacturer markings before bench assembly.
6. Rev A remains a validation build; permanent in-vehicle use is gated on electrical and thermal testing.