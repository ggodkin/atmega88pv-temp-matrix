#include <Arduino.h>

// ----------------- Pin configuration -----------------
const uint8_t LED_PIN  = 7;  // PD7
const uint8_t TEMP_PIN = 4;  // PD4

// ----------------- Matrix configuration -----------------
const int DISP_WIDTH  = 16;
const int DISP_HEIGHT = 16;
const int LED_COUNT   = 256;

// Positions (from substitutions)
const int DIGIT_X1 = 1;
const int DIGIT_X2 = 6;
const int DIGIT_X3 = 11;
const int DIGIT_Y  = 6;
const int DOT_X1   = 5;
const int DOT_X2   = 10;
const int DOT_ROW_OFFSET = 9;
const int UNIT_X   = 15;
const int UNIT_Y   = 0;

// Temperature thresholds (Fahrenheit)
const float GREEN_MIN_F = 95.0f;
const float GREEN_MAX_F = 99.0f;

// ----------------- DS18B20 address -----------------
const uint8_t DS_ADDR[8] = {
  0x79, 0xC0, 0x36, 0x0F, 0x1E, 0x64, 0xFF, 0x28
};

// ----------------- WS2812 buffer -----------------
struct RGB {
  uint8_t g; // GRB order
  uint8_t r;
  uint8_t b;
};

RGB leds[LED_COUNT];

// ----------------- OneWire low-level -----------------
void oneWireWriteBit(bool bit) {
  pinMode(TEMP_PIN, OUTPUT);
  digitalWrite(TEMP_PIN, LOW);
  if (bit) {
    delayMicroseconds(6);
    pinMode(TEMP_PIN, INPUT);
    delayMicroseconds(64);
  } else {
    delayMicroseconds(60);
    pinMode(TEMP_PIN, INPUT);
    delayMicroseconds(10);
  }
}

bool oneWireReadBit() {
  pinMode(TEMP_PIN, OUTPUT);
  digitalWrite(TEMP_PIN, LOW);
  delayMicroseconds(6);
  pinMode(TEMP_PIN, INPUT);
  delayMicroseconds(9);
  bool bit = digitalRead(TEMP_PIN);
  delayMicroseconds(55);
  return bit;
}

void oneWireWriteByte(uint8_t v) {
  for (int i = 0; i < 8; i++) {
    oneWireWriteBit(v & 0x01);
    v >>= 1;
  }
}

uint8_t oneWireReadByte() {
  uint8_t v = 0;
  for (int i = 0; i < 8; i++) {
    if (oneWireReadBit()) {
      v |= (1 << i);
    }
  }
  return v;
}

bool oneWireReset() {
  pinMode(TEMP_PIN, OUTPUT);
  digitalWrite(TEMP_PIN, LOW);
  delayMicroseconds(480);
  pinMode(TEMP_PIN, INPUT);
  delayMicroseconds(70);
  bool presence = (digitalRead(TEMP_PIN) == LOW);
  delayMicroseconds(410);
  return presence;
}

// ----------------- DS18B20 helpers -----------------
void dsMatchROM() {
  oneWireWriteByte(0x55); // MATCH ROM
  for (int i = 0; i < 8; i++) {
    oneWireWriteByte(DS_ADDR[i]);
  }
}

bool dsStartConversion() {
  if (!oneWireReset()) return false;
  dsMatchROM();
  oneWireWriteByte(0x44); // CONVERT T
  return true;
}

bool dsReadScratchpad(uint8_t *scratch) {
  if (!oneWireReset()) return false;
  dsMatchROM();
  oneWireWriteByte(0xBE); // READ SCRATCHPAD
  for (int i = 0; i < 9; i++) {
    scratch[i] = oneWireReadByte();
  }
  return true;
}

bool dsReadTemperature(float &celsius) {
  uint8_t scratch[9];
  if (!dsStartConversion()) return false;
  // Max conversion time ~750ms for 12-bit; we wait 800ms
  delay(800);
  if (!dsReadScratchpad(scratch)) return false;

  int16_t raw = (scratch[1] << 8) | scratch[0];
  celsius = raw / 16.0f; // 12-bit resolution
  return true;
}

// ----------------- WS2812 driver (bit-bang) -----------------
void wsWriteByte(uint8_t b) {
  // Timing tuned for ~8MHz; may need fine-tuning
  for (int i = 7; i >= 0; i--) {
    if (b & (1 << i)) {
      // '1' bit: high longer
      digitalWrite(LED_PIN, HIGH);
      delayMicroseconds(1); // ~0.8us
      digitalWrite(LED_PIN, LOW);
      delayMicroseconds(1); // ~0.45us
    } else {
      // '0' bit: low longer
      digitalWrite(LED_PIN, HIGH);
      delayMicroseconds(0); // ~0.4us
      digitalWrite(LED_PIN, LOW);
      delayMicroseconds(1); // ~0.85us
    }
  }
}

void wsShow() {
  noInterrupts();
  for (int i = 0; i < LED_COUNT; i++) {
    wsWriteByte(leds[i].g);
    wsWriteByte(leds[i].r);
    wsWriteByte(leds[i].b);
  }
  interrupts();
  delayMicroseconds(60); // reset latch
}

// ----------------- Matrix helpers -----------------
int indexFromXY(int x, int y) {
  if (x < 0 || x >= DISP_WIDTH || y < 0 || y >= DISP_HEIGHT) return 0;
  return y * DISP_WIDTH + x;
}

void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  int idx = indexFromXY(x, y);
  leds[idx].r = r;
  leds[idx].g = g;
  leds[idx].b = b;
}

void clearMatrix() {
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i].r = leds[i].g = leds[i].b = 0;
  }
}

// ----------------- Drawing primitives -----------------
void rectangle(int ox, int oy, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      int x = ox + xx;
      int y = oy + yy;
      if (x >= 0 && x < DISP_WIDTH && y >= 0 && y < DISP_HEIGHT) {
        setPixel(x, y, r, g, b);
      }
    }
  }
}

void drawUnitF(int ox, int oy, uint8_t r, uint8_t g, uint8_t b) {
  const bool F[5][3] = {
    {1,1,1},
    {1,0,0},
    {1,1,1},
    {1,0,0},
    {1,0,0}
  };
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 3; x++) {
      if (F[y][x]) {
        setPixel(ox + x, oy + y, r, g, b);
      }
    }
  }
}

// Segment encoding (same as ESPHome)
const uint8_t SEG[10] = {
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

void drawDigit(int ox, int oy, int digit, uint8_t r, uint8_t g, uint8_t b) {
  if (digit < 0 || digit > 9) return;
  uint8_t s = SEG[digit];
  uint8_t off_r = 0, off_g = 0, off_b = 0;

  // Top
  rectangle(ox+1, oy,   2, 1, (s & 0b0000001) ? r : off_r,
                               (s & 0b0000001) ? g : off_g,
                               (s & 0b0000001) ? b : off_b);
  // Upper-right
  rectangle(ox+3, oy+1, 1, 3, (s & 0b0000010) ? r : off_r,
                               (s & 0b0000010) ? g : off_g,
                               (s & 0b0000010) ? b : off_b);
  // Lower-right
  rectangle(ox+3, oy+5, 1, 3, (s & 0b0000100) ? r : off_r,
                               (s & 0b0000100) ? g : off_g,
                               (s & 0b0000100) ? b : off_b);
  // Bottom
  rectangle(ox+1, oy+8, 2, 1, (s & 0b0001000) ? r : off_r,
                               (s & 0b0001000) ? g : off_g,
                               (s & 0b0001000) ? b : off_b);
  // Lower-left
  rectangle(ox,   oy+5, 1, 3, (s & 0b0010000) ? r : off_r,
                               (s & 0b0010000) ? g : off_g,
                               (s & 0b0010000) ? b : off_b);
  // Upper-left
  rectangle(ox,   oy+1, 1, 3, (s & 0b0100000) ? r : off_r,
                               (s & 0b0100000) ? g : off_g,
                               (s & 0b0100000) ? b : off_b);
  // Middle
  rectangle(ox+1, oy+4, 2, 1, (s & 0b1000000) ? r : off_r,
                               (s & 0b1000000) ? g : off_g,
                               (s & 0b1000000) ? b : off_b);
}

void drawMinus(int ox, int oy, uint8_t r, uint8_t g, uint8_t b) {
  rectangle(ox+1, oy+4, 2, 1, r, g, b);
}

void drawDot(int x, int oy, uint8_t r, uint8_t g, uint8_t b) {
  rectangle(x, oy + DOT_ROW_OFFSET, 1, 1, r, g, b);
}

// ----------------- Display update -----------------
void drawTemperatureDisplay(float celsius) {
  clearMatrix();

  float f = celsius * 1.8f + 32.0f;
  float val = f; // fixed Fahrenheit

  uint8_t r, g, b;
  if (f >= GREEN_MIN_F && f <= GREEN_MAX_F) {
    r = 0; g = 255; b = 0;
  } else if (f > GREEN_MAX_F) {
    r = 255; g = 0; b = 0;
  } else {
    r = 0; g = 0; b = 255;
  }

  // Unit glyph "F"
  int ux = min(UNIT_X, DISP_WIDTH - 3);
  int uy = UNIT_Y;
  drawUnitF(ux, uy, r, g, b);

  int y = DIGIT_Y;
  int x1 = DIGIT_X1;
  int x2 = DIGIT_X2;
  int x3 = DIGIT_X3;
  int dot_x2 = DOT_X2;

  bool neg = (val < 0);
  int abs_val_scaled = (int)(fabs(val) * 10.0f + 0.5f);
  int d1 = 0, d2 = 0, d3 = 0;

  if (neg) {
    int t_int = (int)(fabs(val) + 0.5f);
    if (t_int > 99) t_int = 99;
    d1 = t_int / 10;
    d2 = t_int % 10;
    drawMinus(x1, y, r, g, b);
    drawDigit(x2, y, d1, r, g, b);
    drawDigit(x3, y, d2, r, g, b);
  } else if (abs_val_scaled >= 1000) {
    int t_int = abs_val_scaled / 10;
    if (t_int > 999) t_int = 999;
    d1 = (t_int / 100) % 10;
    d2 = (t_int / 10) % 10;
    d3 = t_int % 10;
    drawDigit(x1, y, d1, r, g, b);
    drawDigit(x2, y, d2, r, g, b);
    drawDigit(x3, y, d3, r, g, b);
  } else {
    d1 = (abs_val_scaled / 100) % 10;
    d2 = (abs_val_scaled / 10) % 10;
    d3 = abs_val_scaled % 10;
    drawDigit(x1, y, d1, r, g, b);
    drawDigit(x2, y, d2, r, g, b);
    drawDigit(x3, y, d3, r, g, b);
    drawDot(dot_x2, y, r, g, b);
  }

  wsShow();
}

void drawNoTempCorners() {
  clearMatrix();
  uint8_t r = 0, g = 255, b = 0;
  int max_x = DISP_WIDTH - 1;
  int max_y = DISP_HEIGHT - 1;
  setPixel(0,      0,      r, g, b);
  setPixel(max_x,  0,      r, g, b);
  setPixel(0,      max_y,  r, g, b);
  setPixel(max_x,  max_y,  r, g, b);
  wsShow();
}

// ----------------- Main -----------------
unsigned long lastTempMs = 0;
const unsigned long TEMP_INTERVAL_MS = 5000;

float lastCelsius = NAN;
bool haveTemp = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(TEMP_PIN, INPUT);

  drawNoTempCorners();
}

void loop() {
  unsigned long now = millis();

  if (!haveTemp || (now - lastTempMs) >= TEMP_INTERVAL_MS) {
    float c;
    if (dsReadTemperature(c)) {
      lastCelsius = c;
      haveTemp = true;
      lastTempMs = now;
      drawTemperatureDisplay(lastCelsius);
    } else {
      haveTemp = false;
      drawNoTempCorners();
    }
  }

  // Nothing else; simple periodic update
}
