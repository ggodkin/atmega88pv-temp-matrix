#include "display.h"

namespace {

constexpr uint8_t DIGIT_Y = 6;
constexpr uint8_t SLOT_0 = 0;
constexpr uint8_t SLOT_1 = 4;
constexpr uint8_t SLOT_2 = 8;
constexpr uint8_t SLOT_3 = 12;
constexpr uint8_t DECIMAL_X = 11;

void drawMinus(uint8_t x, uint8_t y, Color color) {
  fillRect(x + 1, y + 4, 2, 1, color);
}

void drawBlankDigit(uint8_t x, uint8_t y) {
  fillRect(x,     y,     4, 9, WS2812::OFF);
}

void drawSignedTemperature(
    int16_t value10,
    Color color
) {
  const bool negative = value10 < 0;

  // Safely obtain magnitude without overflowing on INT16_MIN.
  uint16_t magnitude =
      negative
          ? static_cast<uint16_t>(-(static_cast<int32_t>(value10)))
          : static_cast<uint16_t>(value10);

  // Clear the four character slots.
  drawBlankDigit(SLOT_0, DIGIT_Y);
  drawBlankDigit(SLOT_1, DIGIT_Y);
  drawBlankDigit(SLOT_2, DIGIT_Y);
  drawBlankDigit(SLOT_3, DIGIT_Y);

  if (negative) {
    drawMinus(SLOT_0, DIGIT_Y, color);
  }

  // Four character positions allow:
  //   -99.9
  //    999.9
  //
  // For ordinary temperatures, display one decimal place.
  const uint8_t d1 =
      static_cast<uint8_t>((magnitude / 1000) % 10);

  const uint8_t d2 =
      static_cast<uint8_t>((magnitude / 100) % 10);

  const uint8_t d3 =
      static_cast<uint8_t>((magnitude / 10) % 10);

  const uint8_t d4 =
      static_cast<uint8_t>(magnitude % 10);

  if (negative) {
    // -XX.X
    drawDigit(SLOT_1, DIGIT_Y, d2, color);
    drawDigit(SLOT_2, DIGIT_Y, d3, color);
    drawDigit(SLOT_3, DIGIT_Y, d4, color);
    drawDot(DECIMAL_X, DIGIT_Y, color);
  } else if (magnitude < 1000) {
    // XX.X
    drawDigit(SLOT_1, DIGIT_Y, d2, color);
    drawDigit(SLOT_2, DIGIT_Y, d3, color);
    drawDigit(SLOT_3, DIGIT_Y, d4, color);
    drawDot(DECIMAL_X, DIGIT_Y, color);
  } else {
    // XXX.X
    drawDigit(SLOT_0, DIGIT_Y, d1, color);
    drawDigit(SLOT_1, DIGIT_Y, d2, color);
    drawDigit(SLOT_2, DIGIT_Y, d3, color);
    drawDigit(SLOT_3, DIGIT_Y, d4, color);
    drawDot(DECIMAL_X, DIGIT_Y, color);
  }
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

void Display::drawTemperatureF10(int16_t fahrenheit10) {
  clear();

  const Color color = temperatureColor(fahrenheit10);

  drawSignedTemperature(fahrenheit10, color);
  drawUnitF(color);

  WS2812::show();
}

void Display::drawTemperatureC10(
    int16_t celsius10,
    int16_t fahrenheit10
) {
  clear();

  const Color color = temperatureColor(fahrenheit10);

  drawSignedTemperature(celsius10, color);
  drawUnitC(color);

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
