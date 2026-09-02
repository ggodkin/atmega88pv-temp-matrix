#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// ----------------- Global Constants -----------------

// Pin configuration
const uint8_t LED_PIN  = 7;   // PD7 → WS2812B DIN
const uint8_t TEMP_PIN = 4;   // PD4 → DS18B20 data

// Matrix configuration
const int DISP_WIDTH   = 16;
const int DISP_HEIGHT  = 16;
const int LED_COUNT    = 256;

// Brightness (0–255)
const uint8_t LED_BRIGHTNESS = 128; // 50%

// Refresh interval (ms)
const unsigned long TEMP_INTERVAL_MS = 2000; // 2 seconds

// Temperature thresholds (Fahrenheit)
const float TEMP_LOW_THRESHOLD  = 95.0f;
const float TEMP_HIGH_THRESHOLD = 99.0f;

// Colors (scaled to 50% brightness)
const uint32_t COLOR_GREEN   = Adafruit_NeoPixel::Color(0, 128, 0);
const uint32_t COLOR_RED     = Adafruit_NeoPixel::Color(128, 0, 0);
const uint32_t COLOR_BLUE    = Adafruit_NeoPixel::Color(0, 0, 128);
const uint32_t COLOR_YELLOW  = Adafruit_NeoPixel::Color(128, 128, 0);
const uint32_t COLOR_OFF     = Adafruit_NeoPixel::Color(0, 0, 0);

// ----------------- NeoPixel Object -----------------
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

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
    if (oneWireReadBit()) v |= (1 << i);
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
bool dsStartConversion() {
  if (!oneWireReset()) return false;
  oneWireWriteByte(0xCC); // SKIP ROM (only one sensor)
  oneWireWriteByte(0x44); // CONVERT T
  return true;
}

bool dsReadScratchpad(uint8_t *scratch) {
  if (!oneWireReset()) return false;
  oneWireWriteByte(0xCC); // SKIP ROM
  oneWireWriteByte(0xBE); // READ SCRATCHPAD
  for (int i = 0; i < 9; i++) scratch[i] = oneWireReadByte();
  return true;
}

bool dsReadTemperature(float &celsius) {
  uint8_t scratch[9];
  if (!dsStartConversion()) return false;
  delay(800); // wait for conversion
  if (!dsReadScratchpad(scratch)) return false;
  int16_t raw = (scratch[1] << 8) | scratch[0];
  celsius = raw / 16.0f;
  return true;
}

// ----------------- Matrix helpers -----------------
int indexFromXY(int x, int y) {
  if (x < 0 || x >= DISP_WIDTH || y < 0 || y >= DISP_HEIGHT) return 0;
  return y * DISP_WIDTH + x;
}

void setPixel(int x, int y, uint32_t color) {
  int idx = indexFromXY(x, y);
  strip.setPixelColor(idx, color);
}

void clearMatrix() {
  strip.clear();
}

// ----------------- Digit drawing (7-seg style) -----------------
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

void rectangle(int ox, int oy, int w, int h, uint32_t color) {
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      setPixel(ox + xx, oy + yy, color);
    }
  }
}

void drawDigit(int ox, int oy, int digit, uint32_t color) {
  if (digit < 0 || digit > 9) return;
  uint8_t s = SEG[digit];

  rectangle(ox+1, oy,   2, 1, (s & 0b0000001) ? color : COLOR_OFF);
  rectangle(ox+3, oy+1, 1, 3, (s & 0b0000010) ? color : COLOR_OFF);
  rectangle(ox+3, oy+5, 1, 3, (s & 0b0000100) ? color : COLOR_OFF);
  rectangle(ox+1, oy+8, 2, 1, (s & 0b0001000) ? color : COLOR_OFF);
  rectangle(ox,   oy+5, 1, 3, (s & 0b0010000) ? color : COLOR_OFF);
  rectangle(ox,   oy+1, 1, 3, (s & 0b0100000) ? color : COLOR_OFF);
  rectangle(ox+1, oy+4, 2, 1, (s & 0b1000000) ? color : COLOR_OFF);
}

void drawDot(int x, int y, uint32_t color) {
  rectangle(x, y+9, 1, 1, color);
}

// ----------------- Display update -----------------
void drawTemperatureDisplay(float celsius) {
  clearMatrix();

  float f = celsius * 1.8f + 32.0f;
  uint32_t color;

  if (f >= TEMP_LOW_THRESHOLD && f <= TEMP_HIGH_THRESHOLD) color = COLOR_GREEN;
  else if (f > TEMP_HIGH_THRESHOLD) color = COLOR_RED;
  else color = COLOR_BLUE;

  int y = 4;

  if (f < 100.0f && f >= 0.0f) {
    // Show one decimal place
    int scaled = (int)(f * 10 + 0.5f);
    int d1 = (scaled / 100) % 10;
    int d2 = (scaled / 10) % 10;
    int d3 = scaled % 10;
    drawDigit(1, y, d1, color);
    drawDigit(6, y, d2, color);
    drawDigit(11,y, d3, color);
    drawDot(10, y, color);
  } else {
    // Show integer (up to 3 digits)
    int val = (int)(f + 0.5f);
    if (val > 999) val = 999;
    int d1 = (val / 100) % 10;
    int d2 = (val / 10) % 10;
    int d3 = val % 10;
    drawDigit(1, y, d1, color);
    drawDigit(6, y, d2, color);
    drawDigit(11,y, d3, color);
  }

  strip.show();
}

void drawNoTempCorners() {
  clearMatrix();

  setPixel(0, 0, COLOR_RED);
  setPixel(DISP_WIDTH - 1, 0, COLOR_GREEN);
  setPixel(0, DISP_HEIGHT - 1, COLOR_BLUE);
  setPixel(DISP_WIDTH - 1, DISP_HEIGHT - 1, COLOR_YELLOW);

  strip.show();
}

// ----------------- Main -----------------
unsigned long lastTempMs = 0;
float lastCelsius = NAN;
bool haveTemp = false;

void setup() {
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS); // use constant for brightness
  strip.show(); // all off

  pinMode(TEMP_PIN, INPUT);

  // Show orientation corners until sensor data arrives
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

      // Draw temperature shifted down by 2 lines
      clearMatrix();
      float f = c * 1.8f + 32.0f;
      uint32_t color;

      if (f >= TEMP_LOW_THRESHOLD && f <= TEMP_HIGH_THRESHOLD) color = COLOR_GREEN;
      else if (f > TEMP_HIGH_THRESHOLD) color = COLOR_RED;
      else color = COLOR_BLUE;

      int yShift = 6; // shift down by 2 lines (original was 4)
      if (f < 100.0f && f >= 0.0f) {
        int scaled = (int)(f * 10 + 0.5f);
        int d1 = (scaled / 100) % 10;
        int d2 = (scaled / 10) % 10;
        int d3 = scaled % 10;
        drawDigit(1, yShift, d1, color);
        drawDigit(6, yShift, d2, color);
        drawDigit(11,yShift, d3, color);
        drawDot(10, yShift, color);
      } else {
        int val = (int)(f + 0.5f);
        if (val > 999) val = 999;
        int d1 = (val / 100) % 10;
        int d2 = (val / 10) % 10;
        int d3 = val % 10;
        drawDigit(1, yShift, d1, color);
        drawDigit(6, yShift, d2, color);
        drawDigit(11,yShift, d3, color);
      }

      // Draw "F" in top‑right corner
      // Simple 3x5 block letter F
      rectangle(DISP_WIDTH-4, 0, 3, 1, color); // top bar
      rectangle(DISP_WIDTH-4, 1, 1, 4, color); // vertical bar
      rectangle(DISP_WIDTH-3, 2, 2, 1, color); // middle bar

      strip.show();
    } else {
      haveTemp = false;
      drawNoTempCorners();
    }
  }
}
