# PCG-PSM Rev A Bill of Materials

Status: **Rev A baseline frozen for bench build.** Parts may only be changed after documenting the reason and retesting the affected protection/power function.

The design uses the existing Arduino Uno as the supervisor and keeps the high-current 5 V converter external to the future HAT. Whole-vehicle 12 V current remains sourced from the Volt OEM battery-current data where available. The Infineon main power switch provides a local diagnostic current-sense output without adding a series current shunt.

## Power-path selections

| Ref | Qty | Part | Function | Key rating / reason | Approx. 1-off USD |
| --- | ---: | --- | --- | --- | ---: |
| F1 | 1 | Littelfuse 0287010.U | Source fuse | ATO/ATC automotive, 10 A, 32 V | 0.47 |
| U1 | 1 | TI LM74610QDGKRQ1 | Reverse-battery ideal-diode controller | AEC-Q100, 0.48–42 V, zero-IQ, intended for automotive reverse-polarity protection | 2.59 |
| Q1 | 1 | Nexperia BUK9Y8R8-60ELX | Ideal-diode N-MOSFET | AEC-Q101, 60 V, 5.6 mΩ max @ 10 V | 2.54 |
| D1 | 1 | Littelfuse SLD8S18A | Load-dump / surge TVS | AEC-Q101, 18 V standoff, 29.2 V max clamp, 7 kW 10/1000 µs class | 4.53 |
| U2 | 1 | Infineon BTS500601TEAAUMA2 | Main 12 V high-side power switch | Automotive PROFET, ~6 mΩ, 13.5 A nominal class, protected switch with analog current sense | 4.47 |
| PS1 | 1 | Mean Well DDR-60G-5 | Main PCG 5 V converter | 9–36 V input, 4.5–5.5 V adjustable output, 10.8 A / 54 W, -40 to +85 °C with derating | 33.30 |

### Main converter setting

Set PS1 so the Raspberry Pi sees **5.10 V at its power input under representative operating load**. Do not simply set 5.10 V at the converter and assume the cable drop is negligible. Final wiring and connector choice must be validated at maximum expected Pi + USB load.

## Supervisor / latch selections

| Ref | Qty | Part | Function | Notes | Approx. USD |
| --- | ---: | --- | --- | --- | ---: |
| Q2 | 1 | Infineon IPD90P04P4L04ATMA2 | Supervisor self-latch P-MOSFET | Automotive AEC-Q101, -40 V, 4.3 mΩ; used only on the protected/clamped bus | 2.75 |
| U3 | 1 | onsemi NCV7805BDTRKG | Uno 5 V supervisor regulator | Automotive AEC-Q100, 35 V input, 5 V / 1 A; simple Rev A choice because supervisor power is latched off when idle | 0.62 |
| Q3,Q4 | 2 | 2N3904 or MMBT3904 | ACC wake and Uno self-hold gate pull-down | General-purpose NPN; use documented 40 V or higher parts | ~0.20 |
| DZ1 | 1 | 12 V gate-source Zener | Protect Q2 VGS | 0.5 W or greater | ~0.15 |
| SW1 | 1 | Momentary service/wake switch | Manual service wake | Starts latch; Uno then assumes self-hold | ~1.00 |

### Self-latch behavior

Protected 12 V feeds Q2. Q2 normally remains OFF from a gate-to-source pull-up. ACC or the service button pulls its gate low and powers U3/Uno. The Uno then asserts SELF_HOLD through Q4. After Linux shutdown completes, the Uno releases SELF_HOLD; Q2 turns off and removes supervisor power, minimizing standby draw.

## Pi / Uno interface

| Ref | Qty | Part | Function | Approx. USD |
| --- | ---: | --- | --- | ---: |
| U4 | 1 | Lite-On LTV-847 | Four-channel optocoupler | ACC sense, shutdown request, Pi heartbeat, shutdown acknowledgement | 1.80 |
| RLED | 4 | 680 Ω–1 kΩ | Optocoupler LED limiting | Select per 3.3 V/5 V driving side | ~0.20 |
| RPU | 4 | 10 kΩ | Logic pull-ups | Pi side to 3.3 V; Uno side to 5 V | ~0.20 |
| D2 | 1 | 1N4148 | ACC opto LED reverse clamp | Antiparallel across ACC-sense opto LED | ~0.05 |

The optocouplers prevent 5 V Uno logic from ever being directly applied to 3.3 V Pi GPIO and prevent back-powering either controller when one side is unpowered.

## Measurement networks

| Input | Initial network | Uno input | Purpose |
| --- | --- | --- | --- |
| Protected vehicle 12 V | 180 kΩ / 22 kΩ, 1%, 100 nF across lower resistor | A0 | Vehicle supply voltage |
| Main 5 V rail | 47 kΩ / 47 kΩ, 1%, 100 nF across lower resistor | A1 | Pi supply health |
| 10 kΩ NTC | 10 kΩ fixed resistor divider | A2 | Power-area temperature |
| BTS50060 IS | 470 Ω 1% to ground + 100 nF | A3 | Approximate PCG input current / switch diagnostics |

The A3 current-sense channel is diagnostic only. It does **not** replace the Volt factory negative-terminal battery-current sensor for whole-vehicle current.

## ACC / wake hardware

Use two ACC paths:

1. **Hardware wake:** ACC -> 47 kΩ -> Q3 base, with a 100 kΩ base-emitter pull-down and reverse B-E clamp diode. Q3 pulls the Q2 latch gate low without requiring the Uno to already be powered.
2. **Logic sense:** ACC -> 4.7 kΩ, 0.5 W -> U4 optocoupler LED, with 1N4148 antiparallel protection. Uno reads the optocoupler collector as an active-low ignition input.

This avoids the circular failure mode where an unpowered supervisor would need to sense ACC before it could power itself.

## Bulk / bypass parts

Initial power-entry population:

- 470 µF, 50 V low-ESR electrolytic after reverse protection / TVS node
- 1 µF, 50 V ceramic or film
- 100 nF, 50 V ceramic
- 100 µF + 10 µF + 100 nF on supervisor 5 V rail near Uno supply entry
- Local 100 nF bypass at every logic IC / optocoupler-side rail as appropriate

An input common-mode choke is intentionally **not frozen yet**. Add it only after conducted-noise measurements identify the needed impedance/current rating.

## Wiring / connectors

- **12 V source and main converter input:** 14–16 AWG copper; final size depends on run length and installation temperature.
- **5 V high-current rail:** 14 AWG preferred and as short as practical because voltage drop matters much more at 5 V.
- **Logic / ACC / telemetry:** 20–22 AWG is adequate for the prototype harness.
- Place F1 close to the vehicle 12 V source.
- Use one intentional PCG star-ground point.
- If the isolated Mean Well output is used, bond its 5 V negative to PCG/vehicle ground at that defined point so USB/CAN grounds do not become the accidental bonding path.
- The main 5 V load must not be routed through the Raspberry Pi GPIO header.

## Approximate Rev A hardware cost

Using current one-off distributor pricing, the selected electronics are roughly **$60–70 before shipping**, excluding the Raspberry Pi, Active Cooler, existing Arduino Uno, enclosure, wire, and any USB hub. The DDR-60G-5 is the dominant cost.

## Source references

- Mean Well DDR-60G-5 datasheet / product family: https://www.meanwell.com/
- TI LM74610-Q1: https://www.ti.com/product/LM74610-Q1
- Infineon BTS50060-1TEA: https://www.infineon.com/part/BTS50060-1TEA
- Nexperia BUK9Y8R8-60EL: https://www.nexperia.com/product/BUK9Y8R8-60EL
- Littelfuse SLD8S series: https://www.littelfuse.com/
- onsemi NCV7805: https://www.onsemi.com/

## Release rule

This BOM is **buildable Rev A hardware**, not an automotive production approval. Permanent installation remains gated on bench surge/reverse-polarity testing, repeated startup/shutdown testing, thermal load testing, measured standby current, and in-vehicle validation.