#include "ws2812.h"
#include <avr/interrupt.h>

namespace {

constexpr uint8_t WIDTH = 16;
constexpr uint8_t HEIGHT = 16;
constexpr uint8_t LED_MASK = _BV(PD7);

// Set to true when the physical LED matrix is mounted upside down.
constexpr bool FLIP_180 = true;

// One byte per LED: 0=off, 1=green, 2=red, 3=blue, 4=yellow.
uint8_t pixels[WS2812::LED_COUNT];

inline uint8_t colorComponent(WS2812::Color c, uint8_t component) {
  // 50% output brightness is encoded directly.
  // component: 0=G, 1=R, 2=B.
  switch (c) {
    case WS2812::GREEN:  return component == 0 ? 128 : 0;
    case WS2812::RED:    return component == 1 ? 128 : 0;
    case WS2812::BLUE:   return component == 2 ? 128 : 0;
    case WS2812::YELLOW: return (component == 0 || component == 1) ? 128 : 0;
    default:             return 0;
  }
}

inline uint16_t xyToIndex(uint8_t x, uint8_t y) {
  if (FLIP_180) {
    x = WIDTH - 1 - x;
    y = HEIGHT - 1 - y;
  }

  return static_cast<uint16_t>(y) * WIDTH + x;
}

// Send one WS2812 byte at 8 MHz.
// Each WS2812 bit is exactly 10 AVR clock cycles (~1.25 us).
// SBRS provides the different high times without any relative branch.
// There are deliberately NO RJMP/BRxx instructions in this routine.
inline void sendByte(uint8_t b, uint8_t hi, uint8_t lo) {
  asm volatile(
    // Bit 7
    "out %[port], %[hi] \n\t"
    "sbrs %[b], 7       \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"

    // Bit 6
    "out %[port], %[hi] \n\t"
    "sbrs %[b], 6       \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"

    // Bit 5
    "out %[port], %[hi] \n\t"
    "sbrs %[b], 5       \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"

    // Bit 4
    "out %[port], %[hi] \n\t"
    "sbrs %[b], 4       \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"

    // Bit 3
    "out %[port], %[hi] \n\t"
    "sbrs %[b], 3       \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"

    // Bit 2
    "out %[port], %[hi] \n\t"
    "sbrs %[b], 2       \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"

    // Bit 1
    "out %[port], %[hi] \n\t"
    "sbrs %[b], 1       \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"

    // Bit 0
    "out %[port], %[hi] \n\t"
    "sbrs %[b], 0       \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "nop                \n\t"
    "out %[port], %[lo] \n\t"
    "nop                \n\t"
    :
    : [b] "r" (b),
      [hi] "r" (hi),
      [lo] "r" (lo),
      [port] "I" (_SFR_IO_ADDR(PORTD))
  );
}

} // namespace

namespace WS2812 {

void begin() {
  DDRD |= LED_MASK;
  PORTD &= static_cast<uint8_t>(~LED_MASK);
  clear();
}

void clear() {
  for (uint16_t i = 0; i < LED_COUNT; ++i) {
    pixels[i] = OFF;
  }
}

void setPixel(uint8_t x, uint8_t y, Color color) {
  if (x >= WIDTH || y >= HEIGHT) return;
  pixels[xyToIndex(x, y)] = static_cast<uint8_t>(color);
}

void show() {
  const uint8_t snapshot = PORTD;
  const uint8_t hi = static_cast<uint8_t>(snapshot | LED_MASK);
  const uint8_t lo = static_cast<uint8_t>(snapshot & ~LED_MASK);

  uint8_t grb[3];

  noInterrupts();

  for (uint16_t i = 0; i < LED_COUNT; ++i) {
    const Color c = static_cast<Color>(pixels[i]);
    grb[0] = colorComponent(c, 0); // G
    grb[1] = colorComponent(c, 1); // R
    grb[2] = colorComponent(c, 2); // B

    sendByte(grb[0], hi, lo);
    sendByte(grb[1], hi, lo);
    sendByte(grb[2], hi, lo);
  }

  PORTD &= static_cast<uint8_t>(~LED_MASK);
  interrupts();

  // WS2812 reset/latch.
  delayMicroseconds(80);
}

} // namespace WS2812

