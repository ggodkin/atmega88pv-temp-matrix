#pragma once

#include <Arduino.h>
#include "ws2812.h"

namespace Display {
  constexpr uint8_t WIDTH = 16;
  constexpr uint8_t HEIGHT = 16;

  void begin();
  void clear();
  void drawTemperatureF10(int16_t fahrenheit10);
  void drawSensorError();
}
