#include <Arduino.h>
#include "display.h"
#include "ds18b20.h"

// ATmega88PV @ 8 MHz
//
// Hardware:
//   PD7 -> WS2812B DIN
//   PD4 -> DS18B20 DQ
//   PD2 -> push button (other side to GND)
//
// The DS18B20 requires an external pull-up resistor on DQ.

constexpr uint32_t SAMPLE_INTERVAL_MS = 2000UL;
constexpr uint8_t BUTTON_PIN = 2;       // PD2 / INT0
constexpr uint16_t BUTTON_DEBOUNCE_MS = 30;

int16_t lastTemperatureF10 = 0;
bool haveTemperature = false;
uint32_t nextSampleMs = 0;
bool celsiusMode = false;              // Fahrenheit is always the boot default.
bool buttonStableState = HIGH;
bool buttonLastReading = HIGH;
uint32_t buttonChangedMs = 0;

void setup() {
  Display::begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Start in the same four-corner diagnostic state used by the original.
  Display::drawSensorError();

  nextSampleMs = millis();
}

void loop() {
  const uint32_t now = millis();

  // Active-low push button with simple debounce. A press toggles the display
  // unit; the selected unit is intentionally not stored in EEPROM, so every
  // reboot starts in Fahrenheit.
  const bool reading = digitalRead(BUTTON_PIN);
  if (reading != buttonLastReading) {
    buttonChangedMs = now;
    buttonLastReading = reading;
  }

  if (static_cast<uint32_t>(now - buttonChangedMs) >= BUTTON_DEBOUNCE_MS &&
      reading != buttonStableState) {
    buttonStableState = reading;

    if (buttonStableState == LOW && haveTemperature) {
      celsiusMode = !celsiusMode;

      if (celsiusMode) {
        const int32_t c10 = (static_cast<int32_t>(lastTemperatureF10) - 320L) * 5L / 9L;
        Display::drawTemperatureC10(static_cast<int16_t>(c10), lastTemperatureF10);
      } else {
        Display::drawTemperatureF10(lastTemperatureF10);
      }
    }
  }

  if (!haveTemperature || static_cast<int32_t>(now - nextSampleMs) >= 0) {
    int16_t fahrenheit10;

    if (DS18B20::readTemperatureF10(fahrenheit10)) {
      lastTemperatureF10 = fahrenheit10;
      haveTemperature = true;
      if (celsiusMode) {
        const int32_t c10 = (static_cast<int32_t>(lastTemperatureF10) - 320L) * 5L / 9L;
        Display::drawTemperatureC10(static_cast<int16_t>(c10), lastTemperatureF10);
      } else {
        Display::drawTemperatureF10(lastTemperatureF10);
      }
    } else {
      haveTemperature = false;
      Display::drawSensorError();
    }

    nextSampleMs = millis() + SAMPLE_INTERVAL_MS;
  }
}
