# ATmega88PV Temperature Matrix — Tight Firmware

This is a tighter rewrite of the original 16x16 WS2812B thermometer firmware.

## Hardware

- ATmega88PV / ATmega88P
- CPU: 8 MHz
- 16x16 WS2812B matrix (256 LEDs)
- DS18B20 temperature sensor
- USBasp programming

Pins are intentionally unchanged from the original project:

| Function | AVR pin | Arduino pin |
|---|---:|---:|
| WS2812B DIN | PD7 | 7 |
| DS18B20 DQ | PD4 | 4 |

The DS18B20 data line requires an external pull-up resistor, typically 4.7 kOhm to VCC.

## Major changes

### 1. No 768-byte RGB framebuffer

The original Adafruit_NeoPixel implementation allocates 3 bytes per LED:

`256 × 3 = 768 bytes`

That is most of the ATmega88PV's 1 KB SRAM.

This rewrite stores a compact one-byte color index per pixel:

`256 bytes`

That leaves substantially more SRAM for the stack and application state.

### 2. No Adafruit_NeoPixel dependency

The firmware uses a small ATmega AVR-specific WS2812 transmitter based on the hand-tuned 8 MHz AVR timing technique used by established NeoPixel implementations.

The timing-critical routine is intentionally isolated in `ws2812.cpp`.

**Important:** validate the WS2812 waveform with an oscilloscope or logic analyzer on the actual hardware before treating the driver as production-ready.

### 3. DS18B20 CRC validation

The complete 9-byte scratchpad is read and the Dallas/Maxim CRC-8 is checked before accepting a temperature.

### 4. Integer temperature arithmetic

The application stores Fahrenheit in tenths of a degree:

- 98.6 F = 986
- 95.0 F = 950
- 99.0 F = 990

No floating-point math is required for temperature conversion or display.

### 5. Single display implementation

The duplicated drawing logic from the original firmware is gone.

All temperature rendering is in `display.cpp`.

### 6. Font data in flash

The seven-segment table is stored in PROGMEM rather than SRAM.

## Display behavior

- 95.0–99.0 F: green
- >99.0 F: red
- <95.0 F: blue
- Sensor failure: four colored corner pixels
- Fahrenheit indicator: upper-right
- Temperature digits begin at row 6, preserving the latest visible layout

## Matrix wiring

`ws2812.cpp` currently uses row-major mapping:

`index = y * 16 + x`

If the physical matrix is serpentine, change only `xyToIndex()`.

For a serpentine matrix, use:

```cpp
return (y & 1)
    ? static_cast<uint16_t>(y) * WIDTH + (WIDTH - 1 - x)
    : static_cast<uint16_t>(y) * WIDTH + x;
```

## Build

PlatformIO:

```text
pio run
```

Upload:

```text
pio run --target upload
```

The existing USBasp flags are preserved in `platformio.ini`.

## RAM target

The key RAM allocation is now approximately:

- 256 bytes: color-index framebuffer
- 4 bytes: WS2812 GRB staging buffer
- 9 bytes: DS18B20 scratchpad
- a small amount for state/stack

This is dramatically safer than a 768-byte RGB framebuffer on a 1024-byte SRAM MCU.

## Important hardware test

Because the WS2812 driver is timing-sensitive at 8 MHz:

1. Build the firmware.
2. Program the ATmega88PV.
3. Verify a simple all-off frame.
4. Verify the corner diagnostic frame.
5. Verify the temperature display.
6. If the LEDs behave incorrectly, capture PD7 with a logic analyzer or oscilloscope and verify approximately 800 kHz bit timing.

Do not add `Serial`, interrupts, or other timing-sensitive activity to the WS2812 transmit section without re-evaluating the timing.

