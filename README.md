
Roland Juno G LCD Emulator
==========================

LCD replacement project for Roland Juno G.
The original source is [dpeddi's work](https://github.com/dpeddi/LCDJunoG) under BSD-3-Clause license.
```
 * Copyright (c) 2022 Eddi De Pieri
 * Most code borrowed by Pico-DMX by Jostein Løwer 
 * Modified for performance by bjaan
```

Slight modifications as follows
- (WIP) LCD now uses 8-bit parallel mode for better refresh rate.
- Pin definitions in `platformio.ini` now work correctly. See `platformio.ini` for pinout notes.
- Fix few minor bugs.
- Refactor codes for better readability.

PlatformIO Build Environments
=============================

There are 2 (+1) environments defined:
- `env:pico`: LCD runs in SPI mode.
- `env:pico-parallel`: LCD runs in 8bit parallel mode.
- `env:pico-debugger`: Generates Juno G LCD signals for debugging.

Althogh predefined pin assignments are carefully selected to make PCB creation easier,
you can choose other pins to match your needs.

BOM:
====

- Raspberry Pi Pico
- Display ILI9488 4.0” (purchased from [AliExpress](https://ja.aliexpress.com/item/1005003033844928.html))
- 1.0mm pitch 18-pin flexible flat cable. Length: 210mm (shorter might work), contacts on same side.
- FFC/FPC connector
  - 1.0mm pitch 18-pin, bottom connection; for Juno G connection.
  - (Needed in parallel mode only) 0.5mm pitch 40-pin, top connection; for LCD connection.
- (Optional) 0.1uF ceramic capacitor; for "LCD CONTRAST" nob stability.

Predefined Pin Assignments (Parallel mode):
===========================================

| JunoG Pin | JunoG Function | RpiPico Pin |
| --------- | -------------- | ----------- |
| 18        | +5V            | VSYS        |
| 17        | GND            | GND         |
| 16        | +3V            | (Not used)  |
| 15        | RST            | GP14        |
| 14        | CS1            | GP12        |
| 13        | CS2            | GP13        |
| 12        | RS             | GP11        |
| 11        | WE             | GP10        |
| 10        | D0             | GP2         |
| 9         | D1             | GP3         |
| 8         | D2             | GP4         |
| 7         | D3             | GP5         |
| 6         | D4             | GP6         |
| 5         | D5             | GP7         |
| 4         | D6             | GP8         |
| 3         | D7             | GP9         |
| 2         | BRGT           | GP28        |
| 1         | BRGT Vref      | +3V         |

| LCD Pin# | LCD Function  | RpiPico Pin         |
| -------- | ------------- | ------------------- |
| 40       | IM2           | GND                 |
| 39       | IM1           | 3.3V                |
| 38       | IM0           | 3.3V                |
| 37       | GND           | GND                 |
| 36       | LED-K         | GND                 |
| 35       | LED-K         | GND                 |
| 34       | LED-K         | GND                 |
| 33       | LED-A         | (external resistor) |
| 32       | DB15          | (not used)          |
| 31       | DB14          | (not used)          |
| 30       | DB13          | (not used)          |
| 29       | DB12          | (not used)          |
| 28       | DB11          | (not used)          |
| 27       | DB10          | (not used)          |
| 26       | DB9           | (not used)          |
| 25       | DB8           | (not used)          |
| 24       | DB7           | GP7                 |
| 23       | DB6           | GP6                 |
| 22       | DB5           | GP5                 |
| 21       | DB4           | GP4                 |
| 20       | DB3           | GP3                 |
| 19       | DB2           | GP2                 |
| 18       | DB1           | GP1                 |
| 17       | DB0           | GP0                 |
| 16       | GND           | GND                 |
| 15       | RESET         | GP27                |
| 14       | SPI SDO       | (not used)          |
| 13       | SPI SDI/SDA   | (not used)          |
| 12       | RDX           | 3.3V                |
| 11       | WRX/SPI SCL   | GP26                |
| 10       | DCX/RS(SPI-4) | GP25                |
| 9        | CSX/SPI CS    | GND                 |
| 8        | FMARK(TE)     | (not used)          |
| 7        | VDDA          | 3.3V                |
| 6        | VDDI          | 3.3V                |
| 5        | GND           | GND                 |
| 4        | YD            | (not used)          |
| 3        | XR            | (not used)          |
| 2        | YU            | (not used)          |
| 1        | XL            | (not used)          |


Predefined Pin Assignments (SPI mode):
======================================

| JunoG Pin | JunoG Function | RpiPico Pin |
| --------- | -------------- | ----------- |
| 18        | +5V            | VSYS        |
| 17        | GND            | GND         |
| 16        | +3V            | (Not used)  |
| 15        | RST            | GP14        |
| 14        | CS1            | GP12        |
| 13        | CS2            | GP13        |
| 12        | RS             | GP11        |
| 11        | WE             | GP10        |
| 10        | D0             | GP2         |
| 9         | D1             | GP3         |
| 8         | D2             | GP4         |
| 7         | D3             | GP5         |
| 6         | D4             | GP6         |
| 5         | D5             | GP7         |
| 4         | D6             | GP8         |
| 3         | D7             | GP9         |
| 2         | BRGT           | GP26        |
| 1         | BRGT Vref      | +3V         |