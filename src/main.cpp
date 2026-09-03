#include <Arduino.h>
#include "display.h"
#include "ds18b20.h"

constexpr uint8_t BUTTON_PIN = 2;
constexpr uint32_t TEMP_INTERVAL_MS = 2000;
constexpr uint32_t DEBOUNCE_MS = 30;

enum class Unit : uint8_t {
  Fahrenheit,
  Celsius
};

Unit unit = Unit::Fahrenheit;

uint32_t lastTempMs = 0;
uint32_t lastButtonMs = 0;
bool lastButtonState = HIGH;

int16_t celsius10 = 0;
int16_t fahrenheit10 = 0;
bool haveTemperature = false;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Display::begin();

  if (DS18B20::readTemperatureF10(fahrenheit10)) {
    // F10 -> C10:
    // C10 = (F10 - 320) * 5 / 9
    int32_t value =
        (static_cast<int32_t>(fahrenheit10) - 320L) * 5L;

    if (value >= 0) {
      value += 4;
    } else {
      value -= 4;
    }

    value /= 9;
    celsius10 = static_cast<int16_t>(value);

    haveTemperature = true;
    Display::drawTemperatureF10(fahrenheit10);
  } else {
    Display::drawSensorError();
  }
}

void loop() {
  const uint32_t now = millis();

  // Button: active low, internal pull-up.
  const bool buttonState = digitalRead(BUTTON_PIN);

  if (buttonState != lastButtonState) {
    lastButtonMs = now;
    lastButtonState = buttonState;
  }

  static bool buttonHandled = false;

  if (buttonState == LOW &&
      (now - lastButtonMs) >= DEBOUNCE_MS) {

    if (!buttonHandled) {
      buttonHandled = true;

      // Toggle temperature unit only.
      unit =
          (unit == Unit::Fahrenheit)
              ? Unit::Celsius
              : Unit::Fahrenheit;
    }

  } else if (buttonState == HIGH) {
    // Button released: re-arm for the next press.
    buttonHandled = false;
  }

  // Read/update temperature periodically.
  if (!haveTemperature ||
      (now - lastTempMs) >= TEMP_INTERVAL_MS) {

    lastTempMs = now;

    int16_t newF10;

    if (DS18B20::readTemperatureF10(newF10)) {
      fahrenheit10 = newF10;

      int32_t value =
          (static_cast<int32_t>(fahrenheit10) - 320L) * 5L;

      if (value >= 0) {
        value += 4;
      } else {
        value -= 4;
      }

      value /= 9;

      celsius10 = static_cast<int16_t>(value);
      haveTemperature = true;
    } else {
      haveTemperature = false;
      Display::drawSensorError();
    }
  }

  // Draw the current temperature using the selected unit.
  if (haveTemperature) {
    if (unit == Unit::Fahrenheit) {
      Display::drawTemperatureF10(fahrenheit10);
    } else {
      Display::drawTemperatureC10(
          celsius10,
          fahrenheit10
      );
    }
  }
}