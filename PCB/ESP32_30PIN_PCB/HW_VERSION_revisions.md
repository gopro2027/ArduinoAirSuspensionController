# HW_VERSION divider: board revision table

From V5 onward, every manifold board runs the same `manifold_v5` firmware. At boot, the firmware tells the board revision apart by reading the voltage on the **HW_VERSION** net (ESP32 GPIO39). The code is in `OASMan_ESP32/src/board_version.cpp`.

## Rules for a new board revision
- **Divider:** top resistor from **+3V3S** to HW_VERSION, bottom resistor from HW_VERSION to GND.
- **REV1 and later:**
  - Top resistor is **10.0k**; bottom resistor comes from the table below.
  - Use **0.1 %** resistors. The higher slots are only about ±(20 mV + 2.8 %) wide.
  - Recommended: 100 nF from HW_VERSION to GND.
- **Claim the next unused revision** and never reuse a slot.
- **Keep the same module:** ESP32-WROOM-32E-N16 (16MB) and the V5 valve pinout. All revisions share one firmware file and one partition table.
- **The firmware must know the slot.** A board whose slot the firmware doesn't know yet runs as REV0 and logs a warning. Add the slot's behavior to the firmware before the board ships.

## Table

| Revision | Top | Bottom | Nominal | Firmware reads it as this revision up to |
|---|---|---|---|---|
| V5_REV0 (V5.0, built) | 100k (R167) | 10k (R168) | 0.300 V | 328 mV |
| V5_REV1 | 10.0k | 1.21k | 0.356 V | 390 mV |
| V5_REV2 | 10.0k | 1.47k | 0.423 V | 456 mV |
| V5_REV3 | 10.0k | 1.74k | 0.489 V | 520 mV |
| V5_REV4 | 10.0k | 2.00k | 0.550 V | 591 mV |
| V5_REV5 | 10.0k | 2.37k | 0.632 V | 671 mV |
| V5_REV6 | 10.0k | 2.74k | 0.710 V | 751 mV |
| V5_REV7 | 10.0k | 3.16k | 0.792 V | 830 mV |
| V5_REV8 | 10.0k | 3.57k | 0.868 V | 916 mV |
| V5_REV9 | 10.0k | 4.12k | 0.963 V | 1013 mV |
| V5_REV10 | 10.0k | 4.75k | 1.063 V | 1116 mV |
| V5_REV11 | 10.0k | 5.49k | 1.170 V | 1225 mV |
| V5_REV12 | 10.0k | 6.34k | 1.280 V | 1338 mV |
| V5_REV13 | 10.0k | 7.32k | 1.395 V | 1453 mV |
| V5_REV14 | 10.0k | 8.45k | 1.511 V | 1571 mV |
| V5_REV15 | 10.0k | 9.76k | 1.630 V | 1698 mV |
| V5_REV16 | 10.0k | 11.5k | 1.765 V | 1845 mV |
| V5_REV17 | 10.0k | 14.0k | 1.925 V | 1999 mV |
| V5_REV18 | 10.0k | 16.9k | 2.073 V | 2154 mV |
| V5_REV19 | 10.0k | 21.0k | 2.235 V | 2318 mV |
| V5_REV20 | 10.0k | 26.7k | 2.401 V | 2480 mV |

The firmware picks the nearest nominal voltage. A reading below 150 mV or above 2480 mV counts as unknown.

## Why the gaps grow
The ESP32 ADC error and the 3.3 V rail tolerance both grow with the voltage, so a fixed step isn't safe across the range. A 50 mV step works at 0.4 V but not at 2 V. Each slot is therefore sized to about ±(20 mV + 2.8 % of the voltage). 21 slots fit inside the ESP32 ADC's accurate range of 0.15–2.45 V.
