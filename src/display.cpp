#include "display.h"

namespace {

using WS2812::Color;

constexpr int16_t LOW_F10 = 950;
constexpr int16_t HIGH_F10 = 990;

// Seven-segment bits:
// 0 top, 1 upper-right, 2 lower-right, 3 bottom,
// 4 lower-left, 5 upper-left, 6 middle.
const uint8_t SEGMENT_MASK[10] PROGMEM = {
  0b0111111,
  0b0000110,
  0b1011011,
  0b1001111,
  0b1100110,
  0b1101101,
  0b1111101,
  0b0000111,
  0b1111111,
  0b1101111
};

void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, Color color) {
  for (uint8_t yy = 0; yy < h; ++yy) {
    for (uint8_t xx = 0; xx < w; ++xx) {
      WS2812::setPixel(x + xx, y + yy, color);
    }
  }
}

void drawDigit(uint8_t x, uint8_t y, uint8_t digit, Color color) {
  if (digit > 9) return;

  const uint8_t s = pgm_read_byte(&SEGMENT_MASK[digit]);

  fillRect(x + 1, y,     2, 1, (s & _BV(0)) ? color : WS2812::OFF);
  fillRect(x + 3, y + 1, 1, 3, (s & _BV(1)) ? color : WS2812::OFF);
  fillRect(x + 3, y + 5, 1, 3, (s & _BV(2)) ? color : WS2812::OFF);
  fillRect(x + 1, y + 8, 2, 1, (s & _BV(3)) ? color : WS2812::OFF);
  fillRect(x,     y + 5, 1, 3, (s & _BV(4)) ? color : WS2812::OFF);
  fillRect(x,     y + 1, 1, 3, (s & _BV(5)) ? color : WS2812::OFF);
  fillRect(x + 1, y + 4, 2, 1, (s & _BV(6)) ? color : WS2812::OFF);
}

void drawDot(uint8_t x, uint8_t y, Color color) {
  WS2812::setPixel(x, y + 9, color);
}

void drawF(Color color) {
  fillRect(12, 0, 3, 1, color);
  fillRect(12, 1, 1, 4, color);
  fillRect(13, 2, 2, 1, color);
}

Color temperatureColor(int16_t f10) {
  if (f10 >= LOW_F10 && f10 <= HIGH_F10) return WS2812::GREEN;
  if (f10 > HIGH_F10) return WS2812::RED;
  return WS2812::BLUE;
}

} // namespace

namespace Display {

void begin() {
  WS2812::begin();
  clear();
}

void clear() {
  WS2812::clear();
}

void drawTemperatureF10(int16_t fahrenheit10) {
  clear();

  const Color color = temperatureColor(fahrenheit10);

  // Preserve the current repository's visible layout:
  // digits start at y=6 and the F occupies the upper-right corner.
  constexpr uint8_t y = 6;

  if (fahrenheit10 >= 0 && fahrenheit10 < 1000) {
    // 0.0 through 99.9
    const uint16_t value = static_cast<uint16_t>(fahrenheit10);

    const uint8_t d1 = static_cast<uint8_t>((value / 100) % 10);
    const uint8_t d2 = static_cast<uint8_t>((value / 10) % 10);
    const uint8_t d3 = static_cast<uint8_t>(value % 10);

    drawDigit(1,  y, d1, color);
    drawDigit(6,  y, d2, color);
    drawDigit(11, y, d3, color);
    drawDot(10, y, color);
  } else if (fahrenheit10 >= 1000) {
    // 100.0 and above: display rounded integer.
    uint16_t value = static_cast<uint16_t>((fahrenheit10 + 5) / 10);
    if (value > 999) value = 999;

    drawDigit(1,  y, static_cast<uint8_t>((value / 100) % 10), color);
    drawDigit(6,  y, static_cast<uint8_t>((value / 10) % 10),  color);
    drawDigit(11, y, static_cast<uint8_t>(value % 10),           color);
  } else {
    // Negative temperatures are outside the original UI. Show 000 rather
    // than producing invalid digit arithmetic.
    drawDigit(1,  y, 0, color);
    drawDigit(6,  y, 0, color);
    drawDigit(11, y, 0, color);
  }

  drawF(color);
  WS2812::show();
}

void drawSensorError() {
  clear();

  WS2812::setPixel(0, 0, WS2812::RED);
  WS2812::setPixel(WIDTH - 1, 0, WS2812::GREEN);
  WS2812::setPixel(0, HEIGHT - 1, WS2812::BLUE);
  WS2812::setPixel(WIDTH - 1, HEIGHT - 1, WS2812::YELLOW);

  WS2812::show();
}

} // namespace Display
