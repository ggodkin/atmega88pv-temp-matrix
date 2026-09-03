#pragma once

#include <Arduino.h>

// Minimal WS2812B driver for ATmega88P/V @ 8 MHz.
//
// The framebuffer is NOT RGB. Each matrix pixel stores only a 3-bit color
// index, so 256 LEDs require 256 bytes rather than 768 bytes.
//
// Color order is GRB, as expected by the current hardware.

namespace WS2812 {
  constexpr uint8_t LED_PIN = 7;  // PD7
  constexpr uint16_t LED_COUNT = 256;

  enum Color : uint8_t {
    OFF = 0,
    GREEN = 1,
    RED = 2,
    BLUE = 3,
    YELLOW = 4
  };

  void begin();
  void clear();
  void setPixel(uint8_t x, uint8_t y, Color color);
  void show();
}
