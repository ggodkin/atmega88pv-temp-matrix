#pragma once

#include <Arduino.h>

namespace DS18B20 {

  // Existing blocking API.
  // Used during startup.
  bool readTemperatureC(int16_t &celsius16);
  bool readTemperatureF10(int16_t &fahrenheit10);

  // Non-blocking temperature conversion.
  // Call startConversion(), then continue running the main loop.
  bool startConversion();

  // Returns true when the conversion has had enough time to complete.
  bool conversionReady();

  // Reads the completed conversion.
  bool readTemperatureF10AfterConversion(int16_t &fahrenheit10);
}