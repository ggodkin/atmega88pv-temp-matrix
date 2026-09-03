#include "display.h"

namespace {

constexpr uint8_t DIGIT_Y = 6;

// Three display positions.
constexpr uint8_t DIGIT_0_X = 1;
constexpr uint8_t DIGIT_1_X = 6;
constexpr uint8_t DIGIT_2_X = 11;

constexpr uint8_t DECIMAL_X = 10;

constexpr int16_t LOW_F10 = 950;
constexpr int16_t HIGH_F10 = 990;

// Seven-segment bits:
// 0 top
// 1 upper-right
// 2 lower-right
// 3 bottom
// 4 lower-left
// 5 upper-left
// 6 middle

const uint8_t SEGMENT_MASK[10] PROGMEM = {
  0b0111111,  // 0
  0b0000110,  // 1
  0b1011011,  // 2
  0b1001111,  // 3
  0b1100110,  // 4
  0b1101101,  // 5
  0b1111101,  // 6
  0b0000111,  // 7
  0b1111111,  // 8
  0b1101111   // 9
};

void fillRect(
    uint8_t x,
    uint8_t y,
    uint8_t w,
    uint8_t h,
    WS2812::Color color
) {
  for (uint8_t yy = 0; yy < h; ++yy) {
    for (uint8_t xx = 0; xx < w; ++xx) {
      WS2812::setPixel(
          x + xx,
          y + yy,
          color);
    }
  }
}

void drawDigit(
    uint8_t x,
    uint8_t y,
    uint8_t digit,
    WS2812::Color color
) {
  if (digit > 9) {
    return;
  }

  const uint8_t s =
      pgm_read_byte(&SEGMENT_MASK[digit]);

  // Top
  fillRect(
      x + 1,
      y,
      2,
      1,
      (s & _BV(0))
          ? color
          : WS2812::OFF);

  // Upper-right
  fillRect(
      x + 3,
      y + 1,
      1,
      3,
      (s & _BV(1))
          ? color
          : WS2812::OFF);

  // Lower-right
  fillRect(
      x + 3,
      y + 5,
      1,
      3,
      (s & _BV(2))
          ? color
          : WS2812::OFF);

  // Bottom
  fillRect(
      x + 1,
      y + 8,
      2,
      1,
      (s & _BV(3))
          ? color
          : WS2812::OFF);

  // Lower-left
  fillRect(
      x,
      y + 5,
      1,
      3,
      (s & _BV(4))
          ? color
          : WS2812::OFF);

  // Upper-left
  fillRect(
      x,
      y + 1,
      1,
      3,
      (s & _BV(5))
          ? color
          : WS2812::OFF);

  // Middle
  fillRect(
      x + 1,
      y + 4,
      2,
      1,
      (s & _BV(6))
          ? color
          : WS2812::OFF);
}

void drawDot(
    uint8_t x,
    uint8_t y,
    WS2812::Color color
) {
  WS2812::setPixel(
      x,
      y + 9,
      color);
}

void drawMinus(
    uint8_t x,
    uint8_t y,
    WS2812::Color color
) {
  fillRect(
      x + 1,
      y + 4,
      2,
      1,
      color);
}

void drawUnitC(WS2812::Color color) {
  fillRect(
      12,
      0,
      3,
      1,
      color);

  fillRect(
      12,
      1,
      1,
      3,
      color);

  fillRect(
      12,
      4,
      3,
      1,
      color);
}

void drawUnitF(WS2812::Color color) {
  fillRect(
      12,
      0,
      3,
      1,
      color);

  fillRect(
      12,
      1,
      1,
      4,
      color);

  fillRect(
      13,
      2,
      2,
      1,
      color);
}

WS2812::Color temperatureColor(int16_t f10) {
  if (f10 >= LOW_F10 && f10 <= HIGH_F10) {
    return WS2812::GREEN;
  }

  if (f10 > HIGH_F10) {
    return WS2812::RED;
  }

  return WS2812::BLUE;
}

/*
 * Display a signed temperature using exactly three character positions.
 *
 * Positive:
 *
 *   0.0 .. 99.9  -> XX.X
 *   100.0+       -> XXX
 *
 * Negative:
 *
 *   -0.1 .. -9.9 -> -X.X
 *   -10.0+       -> -XX
 *
 * Examples:
 *
 *    8.4   -> 08.4
 *   12.3   -> 12.3
 *   99.9   -> 99.9
 *   100.0  -> 100
 *   101.6  -> 102
 *
 *   -0.5  -> -0.5
 *   -8.4  -> -8.4
 *   -9.9  -> -9.9
 *   -10.0 -> -10
 *   -10.9 -> -10
 *   -12.7 -> -12
 *   -99.9 -> -99
 *
 * For negative values of -10 degrees or below,
 * the decimal portion is discarded rather than rounded.
 *
 * Values outside the displayable range are clamped.
 */
void drawSignedTemperature(
    int16_t value10,
    WS2812::Color color
) {
  const bool negative = value10 < 0;

  /*
   * Convert to a positive magnitude safely.
   *
   * The int32_t conversion is important because INT16_MIN
   * cannot be negated safely as an int16_t.
   */
  const uint16_t magnitude =
      negative
          ? static_cast<uint16_t>(
                -(static_cast<int32_t>(value10)))
          : static_cast<uint16_t>(value10);

  if (negative) {

    if (magnitude < 100) {
      /*
       * ----------------------------------------------------
       * -0.1 through -9.9
       *
       * Three positions:
       *
       *   [minus] [integer] [tenths]
       *
       * Example:
       *
       *   -8.4
       * ----------------------------------------------------
       */

      drawMinus(
          DIGIT_0_X,
          DIGIT_Y,
          color);

      drawDigit(
          DIGIT_1_X,
          DIGIT_Y,
          static_cast<uint8_t>(
              magnitude / 10),
          color);

      drawDigit(
          DIGIT_2_X,
          DIGIT_Y,
          static_cast<uint8_t>(
              magnitude % 10),
          color);

      drawDot(
          DECIMAL_X,
          DIGIT_Y,
          color);

    } else {
      /*
       * ----------------------------------------------------
       * -10.0 and below
       *
       * Three positions:
       *
       *   [minus] [tens] [ones]
       *
       * No decimal point.
       *
       * Decimal portion is discarded.
       *
       * Examples:
       *
       *   -10.9 -> -10
       *   -12.7 -> -12
       *   -99.9 -> -99
       * ----------------------------------------------------
       */

      uint16_t whole =
          static_cast<uint16_t>(
              magnitude / 10);

      /*
       * The display only has room for:
       *
       *   -99
       *
       * Clamp anything more negative.
       */
      if (whole > 99) {
        whole = 99;
      }

      drawMinus(
          DIGIT_0_X,
          DIGIT_Y,
          color);

      drawDigit(
          DIGIT_1_X,
          DIGIT_Y,
          static_cast<uint8_t>(
              whole / 10),
          color);

      drawDigit(
          DIGIT_2_X,
          DIGIT_Y,
          static_cast<uint8_t>(
              whole % 10),
          color);
    }

  } else {

    if (magnitude < 1000) {
      /*
       * ----------------------------------------------------
       * 0.0 through 99.9
       *
       * Three positions:
       *
       *   [tens] [ones] [tenths]
       *
       * Decimal point is between the second and third
       * positions.
       *
       * Examples:
       *
       *   08.4
       *   12.3
       *   99.9
       * ----------------------------------------------------
       */

      drawDigit(
          DIGIT_0_X,
          DIGIT_Y,
          static_cast<uint8_t>(
              (magnitude / 100) % 10),
          color);

      drawDigit(
          DIGIT_1_X,
          DIGIT_Y,
          static_cast<uint8_t>(
              (magnitude / 10) % 10),
          color);

      drawDigit(
          DIGIT_2_X,
          DIGIT_Y,
          static_cast<uint8_t>(
              magnitude % 10),
          color);

      drawDot(
          DECIMAL_X,
          DIGIT_Y,
          color);

    } else {
      /*
       * ----------------------------------------------------
       * 100.0 and above
       *
       * Three positions:
       *
       *   [hundreds] [tens] [ones]
       *
       * No decimal point.
       *
       * Positive values are rounded to the nearest whole
       * degree.
       *
       * Examples:
       *
       *   100.0 -> 100
       *   101.2 -> 101
       *   101.6 -> 102
       * ----------------------------------------------------
       */

      uint16_t whole =
          static_cast<uint16_t>(
              (magnitude + 5) / 10);

      /*
       * Three digits maximum.
       */
      if (whole > 999) {
        whole = 999;
      }

      drawDigit(
          DIGIT_0_X,
          DIGIT_Y,
          static_cast<uint8_t>(
              (whole / 100) % 10),
          color);

      drawDigit(
          DIGIT_1_X,
          DIGIT_Y,
          static_cast<uint8_t>(
              (whole / 10) % 10),
          color);

      drawDigit(
          DIGIT_2_X,
          DIGIT_Y,
          static_cast<uint8_t>(
              whole % 10),
          color);
    }
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

void drawTemperatureF10(
    int16_t fahrenheit10
) {
  clear();

  const WS2812::Color color =
      temperatureColor(fahrenheit10);

  drawSignedTemperature(
      fahrenheit10,
      color);

  drawUnitF(color);

  WS2812::show();
}

void drawTemperatureC10(
    int16_t celsius10,
    int16_t fahrenheit10
) {
  clear();

  /*
   * Keep alarm coloring based on the Fahrenheit
   * thresholds, even when the display is showing Celsius.
   */
  const WS2812::Color color =
      temperatureColor(fahrenheit10);

  drawSignedTemperature(
      celsius10,
      color);

  drawUnitC(color);

  WS2812::show();
}

void drawSensorError() {
  clear();

  WS2812::setPixel(
      0,
      0,
      WS2812::RED);

  WS2812::setPixel(
      WIDTH - 1,
      0,
      WS2812::GREEN);

  WS2812::setPixel(
      0,
      HEIGHT - 1,
      WS2812::BLUE);

  WS2812::setPixel(
      WIDTH - 1,
      HEIGHT - 1,
      WS2812::YELLOW);

  WS2812::show();
}

} // namespace Display