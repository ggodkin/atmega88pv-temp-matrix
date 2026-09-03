#include "ds18b20.h"

namespace {

constexpr uint8_t TEMP_PIN = 4;  // PD4

inline void releaseBus() {
  // External pull-up required on DQ.
  DDRD &= static_cast<uint8_t>(~_BV(PD4));
  PORTD &= static_cast<uint8_t>(~_BV(PD4));
}

inline void driveLow() {
  PORTD &= static_cast<uint8_t>(~_BV(PD4));
  DDRD |= _BV(PD4);
}

void writeBit(uint8_t bit) {
  driveLow();

  if (bit) {
    // ~6 us low, then release for the remainder of the slot.
    delayMicroseconds(6);
    releaseBus();
    delayMicroseconds(64);
  } else {
    // ~60 us low.
    delayMicroseconds(60);
    releaseBus();
    delayMicroseconds(10);
  }
}

uint8_t readBit() {
  driveLow();
  delayMicroseconds(6);
  releaseBus();
  delayMicroseconds(9);

  const uint8_t bit = (PIND & _BV(PD4)) ? 1 : 0;

  delayMicroseconds(55);
  return bit;
}

void writeByte(uint8_t value) {
  for (uint8_t i = 0; i < 8; ++i) {
    writeBit(value & 1);
    value >>= 1;
  }
}

uint8_t readByte() {
  uint8_t value = 0;
  for (uint8_t i = 0; i < 8; ++i) {
    value |= static_cast<uint8_t>(readBit() << i);
  }
  return value;
}

bool reset() {
  driveLow();
  delayMicroseconds(480);

  releaseBus();
  delayMicroseconds(70);

  const bool present = !(PIND & _BV(PD4));

  delayMicroseconds(410);
  return present;
}

// Dallas/Maxim CRC-8, polynomial x^8+x^5+x^4+1 (0x31 reflected = 0x8C).
uint8_t crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0;

  while (len--) {
    uint8_t in = *data++;

    for (uint8_t i = 0; i < 8; ++i) {
      const uint8_t mix = (crc ^ in) & 1;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      in >>= 1;
    }
  }

  return crc;
}

bool startConversion() {
  if (!reset()) return false;

  writeByte(0xCC); // SKIP ROM
  writeByte(0x44); // CONVERT T
  return true;
}

bool readScratchpad(uint8_t *scratch) {
  if (!reset()) return false;

  writeByte(0xCC); // SKIP ROM
  writeByte(0xBE); // READ SCRATCHPAD

  for (uint8_t i = 0; i < 9; ++i) {
    scratch[i] = readByte();
  }

  return crc8(scratch, 8) == scratch[8];
}

bool rawToC16(int16_t raw, int16_t &c16) {
  // DS18B20 temperature is raw / 16 degrees C.
  c16 = raw;
  return true;
}

} // namespace

namespace DS18B20 {

bool readTemperatureC(int16_t &celsius16) {
  uint8_t scratch[9];

  if (!startConversion()) return false;

  // 12-bit conversion at 5V can take up to 750 ms.
  delay(750);

  if (!readScratchpad(scratch)) return false;

  const int16_t raw =
      static_cast<int16_t>((static_cast<uint16_t>(scratch[1]) << 8) |
                           scratch[0]);

  return rawToC16(raw, celsius16);
}

bool readTemperatureF10(int16_t &fahrenheit10) {
  int16_t c16;

  if (!readTemperatureC(c16)) return false;

  // F*10 = (C*18) + 320.
  // Since c16 = C*16:
  // F*10 = c16 * 9 / 8 + 320.
  int32_t value = static_cast<int32_t>(c16) * 9;

  if (value >= 0) {
    value += 4;  // nearest integer
  } else {
    value -= 4;
  }

  value /= 8;
  value += 320;

  if (value < -32768L || value > 32767L) return false;

  fahrenheit10 = static_cast<int16_t>(value);
  return true;
}

} // namespace DS18B20
