# ATmega88PV 16×16 Temperature Matrix

A compact thermometer built around an **ATmega88PV**, a **16×16 WS2812B LED matrix**, and a **DS18B20 temperature sensor**.

The firmware is designed for the ATmega88PV's limited SRAM while providing a responsive temperature display and push-button Fahrenheit/Celsius selection.

## Hardware

* ATmega88PV / ATmega88P
* 8 MHz CPU clock
* 16×16 WS2812B matrix (256 LEDs)
* DS18B20 temperature sensor
* USBasp programmer

### Pin assignments

| Function    | AVR pin | Arduino pin |
| ----------- | ------: | ----------: |
| WS2812B DIN |     PD7 |           7 |
| DS18B20 DQ  |     PD4 |           4 |
| F/C button  |     PD2 |           2 |

The DS18B20 data line requires an external pull-up resistor, typically **4.7 kΩ to VCC**.

The F/C push button is connected between **PD2 and GND**. The firmware uses the ATmega's internal pull-up resistor, so no external button resistor is required.

## Temperature measurement

The DS18B20 is operated at **12-bit resolution**.

A temperature conversion can take up to approximately **750 ms**. Conversion is performed asynchronously so that the main loop continues running while the sensor is measuring.

This keeps the push button responsive even while a temperature conversion is in progress.

Temperature values are stored using integer arithmetic:

* Fahrenheit is stored in tenths of a degree.
* Celsius is stored in tenths of a degree.
* No floating-point arithmetic is required.

For example:

```text
98.6 °F → 986
37.0 °C → 370
```

The DS18B20 scratchpad is checked using the Dallas/Maxim CRC-8 before a reading is accepted.

## Display

The temperature uses a compact three-digit display in the center/lower portion of the 16×16 matrix.

The display supports:

* Positive temperatures
* Negative temperatures
* Decimal point
* Fahrenheit (`F`)
* Celsius (`C`)
* Temperature-dependent color
* Sensor-error indication

### Temperature colors

The display uses Fahrenheit thresholds regardless of the currently selected display unit:

| Temperature | Color |
| ----------- | ----- |
| Below 95 °F | Blue  |
| 95–99 °F    | Green |
| Above 99 °F | Red   |

For example, when the display is set to Celsius, a temperature of 37 °C is still colored according to its Fahrenheit equivalent.

### Fahrenheit / Celsius selection

Pressing the button toggles between:

```text
F → C → F → C ...
```

The selected unit is not stored in EEPROM. After a reboot, the display starts in Fahrenheit.

The button uses software debouncing.

Changing the unit redraws the current temperature immediately; it does not wait for another sensor reading.

## Sensor error indication

If the DS18B20 cannot be read successfully, the display shows four colored corner pixels.

| Position     | Color  |
| ------------ | ------ |
| Top-left     | Red    |
| Top-right    | Green  |
| Bottom-left  | Blue   |
| Bottom-right | Yellow |

This provides both a clear sensor-error indication and a convenient way to determine the physical orientation of the LED matrix.

A successful sensor reading automatically restores the normal temperature display.

## Temperature update timing

The firmware uses a non-blocking sensor conversion.

The conversion itself takes up to approximately 750 ms at 12-bit resolution. The interval between conversions is controlled by:

```cpp
constexpr uint32_t TEMP_INTERVAL_MS = 1000;
```

The interval can be reduced if more frequent updates are desired.

For approximately one temperature update per second, an interval of about **250 ms** provides roughly:

```text
750 ms sensor conversion
+ 250 ms interval
≈ 1 second per update
```

The button remains responsive regardless of the temperature conversion timing.

## LED matrix mapping

The WS2812B driver currently uses row-major pixel mapping:

```text
index = y * 16 + x
```

The physical matrix orientation therefore depends on how the matrix is wired.

If the matrix uses serpentine wiring, the coordinate-to-pixel mapping can be adjusted in `ws2812.cpp`.

## Memory usage

The firmware uses a compact one-byte color index for each framebuffer pixel.

For the 256-pixel matrix:

```text
256 pixels × 1 byte = 256 bytes
```

This leaves substantially more SRAM available for the application and stack on the ATmega88PV, which has only 1 KB of SRAM.

## Project structure

```text
src/
├── main.cpp
├── display.cpp
├── display.h
├── ds18b20.cpp
├── ds18b20.h
├── ws2812.cpp
└── ws2812.h
```

### `main.cpp`

Handles:

* Main application loop
* Button input and debouncing
* Fahrenheit/Celsius selection
* Temperature update scheduling
* Sensor conversion state
* Display updates

### `ds18b20.cpp`

Handles:

* DS18B20 communication
* Temperature conversion
* Scratchpad reading
* CRC validation
* Celsius/Fahrenheit conversion
* Non-blocking conversion timing

### `display.cpp`

Handles:

* Temperature rendering
* Digit rendering
* Decimal point
* Minus sign
* Fahrenheit/Celsius indicator
* Temperature colors
* Sensor-error display

### `ws2812.cpp`

Handles:

* WS2812B initialization
* Pixel framebuffer
* Color handling
* WS2812B data transmission
* Matrix coordinate mapping

## Building

The project uses PlatformIO.

Build the firmware:

```text
pio run
```

Upload using USBasp:

```text
pio run --target upload
```

The USBasp programming configuration is contained in `platformio.ini`.

## Hardware considerations

The WS2812B data signal is timing-sensitive when driven directly by an 8 MHz AVR.

The WS2812B data line is connected to **PD7**.

The DS18B20 requires an external pull-up resistor on its data line. A **4.7 kΩ resistor to VCC** is a typical value.

## Operation

After power-up:

1. The firmware initializes the LED matrix.
2. The DS18B20 is read.
3. The temperature is displayed.
4. The firmware periodically obtains new temperature readings.
5. The display remains responsive to the F/C button during sensor conversion.
6. Pressing the button immediately switches between Fahrenheit and Celsius.
7. If the sensor fails, the four-corner diagnostic display is shown.
8. Normal temperature display resumes automatically when a valid sensor reading is obtained.
