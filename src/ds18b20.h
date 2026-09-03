#pragma once

#include <Arduino.h>

namespace DS18B20 {
  bool readTemperatureC(int16_t &celsius16);
  bool readTemperatureF10(int16_t &fahrenheit10);
}
