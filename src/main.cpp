#include <Arduino.h>
#include "display.h"
#include "ds18b20.h"

// ATmega88PV @ 8 MHz
//
// Hardware:
//   PD7 -> WS2812B DIN
//   PD4 -> DS18B20 DQ
//
// The DS18B20 requires an external pull-up resistor on DQ.

constexpr uint32_t SAMPLE_INTERVAL_MS = 2000UL;

int16_t lastTemperatureF10 = 0;
bool haveTemperature = false;
uint32_t nextSampleMs = 0;

void setup() {
  Display::begin();

  // Start in the same four-corner diagnostic state used by the original.
  Display::drawSensorError();

  nextSampleMs = millis();
}

void loop() {
  const uint32_t now = millis();

  if (!haveTemperature || static_cast<int32_t>(now - nextSampleMs) >= 0) {
    int16_t fahrenheit10;

    if (DS18B20::readTemperatureF10(fahrenheit10)) {
      lastTemperatureF10 = fahrenheit10;
      haveTemperature = true;
      Display::drawTemperatureF10(lastTemperatureF10);
    } else {
      haveTemperature = false;
      Display::drawSensorError();
    }

    nextSampleMs = millis() + SAMPLE_INTERVAL_MS;
  }
}
