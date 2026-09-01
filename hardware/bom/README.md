# PCG-PSM BOM Workspace

This directory tracks the hardware bill of materials from Rev A prototype through the permanent HAT.

## Rev A categories

The final component selections are still open. The BOM must cover:

| Category | Requirement |
| --- | --- |
| Input fuse | Sized for PCG-Core branch and wiring |
| Transient protection | Automotive TVS / surge suppression |
| Reverse polarity | Low-loss protected input stage |
| ACC conditioning | Safe automotive input to supervisor logic |
| Supervisor supply | Low-standby regulated supply for Uno/ATmega logic |
| Main switch | 12 V-side high-current switch / MOSFET control |
| Main DC/DC | Approximately 5.1 V / 10 A preferred |
| Pi interface | Safe 5 V to 3.3 V shutdown/heartbeat signaling |
| Voltage sensing | Protected 12 V and 5 V measurement networks |
| Temperature | Supply/power-stage temperature sensor |
| User interface | Service/wake button and status indication |
| Connectors | Automotive-appropriate power, ACC, and signal connectors |
| Wiring | Gauge selected for current, length, temperature, and voltage drop |

## Selection rules

1. Permanent parts should have documented electrical ratings.
2. Automotive or industrial-rated components are preferred for vehicle-facing circuitry.
3. The Pi high-current rail must not depend on the GPIO header for load current.
4. Protection devices must be selected as a system rather than by isolated headline ratings.
5. Prototype substitutions must be clearly identified and must not silently become production parts.
6. Final thresholds and ratings require bench measurements and in-vehicle validation.

A part-numbered Rev A BOM will replace this planning document when component selection is frozen.
