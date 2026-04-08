#include <Arduino.h>
#include <Wire.h>
#include "src\AW9523\elecrow_aw9523.h"

// ==================== Pin Definitions ====================
// I2C pins connected to AW9523
#define I2C_SCL 15
#define I2C_SDA 16

// ==================== Global Variables ====================
// RGB pattern index
uint8_t rgb_pattern_index = 0;

// Pointer to I2C bus
static TwoWire* wi = &Wire;

// Timing for color changes
const uint16_t STEP_DELAY_MS = 20; // Delay per step in rainbow fade
const uint8_t BRIGHTNESS = 0xFF;   // Max brightness

// ==================== Helper Functions ====================
// Converts individual red, green, blue components (0-255) to 24-bit color
uint32_t RGB(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// Smooth fade between two colors
void fadeColor(uint32_t fromColor, uint32_t toColor, uint16_t steps, uint16_t delayMs) {
  int r1 = (fromColor >> 16) & 0xFF;
  int g1 = (fromColor >> 8) & 0xFF;
  int b1 = fromColor & 0xFF;

  int r2 = (toColor >> 16) & 0xFF;
  int g2 = (toColor >> 8) & 0xFF;
  int b2 = toColor & 0xFF;

  for (uint16_t i = 0; i <= steps; i++) {
    uint8_t r = r1 + (r2 - r1) * i / steps;
    uint8_t g = g1 + (g2 - g1) * i / steps;
    uint8_t b = b1 + (b2 - b1) * i / steps;
    aw9523.AW_set_RGB(RGB(r, g, b));
    delay(delayMs);
  }
}

// Generate a "rainbow" color based on a step (0-255)
uint32_t rainbowColor(uint8_t step) {
  uint8_t r, g, b;
  if (step < 85) {
    r = 255 - step * 3;
    g = step * 3;
    b = 0;
  } else if (step < 170) {
    step -= 85;
    r = 0;
    g = 255 - step * 3;
    b = step * 3;
  } else {
    step -= 170;
    r = step * 3;
    g = 0;
    b = 255 - step * 3;
  }
  return RGB(r, g, b);
}

// ==================== Arduino Setup ====================
void setup(void) {
  Serial.begin(115200);
  while (!Serial);

  // Initialize I2C on custom pins
  wi->setPins(I2C_SDA, I2C_SCL);
  wi->begin();
  delay(100);

  // Initialize AW9523
  aw9523.AW_init();
  aw9523.AW_set_POWER(true);
  Serial.println("AW9523 Initialized");
}

// ==================== Arduino Main Loop ====================
void loop() {
  switch (rgb_pattern_index) {
    case 0: // Pattern 1: Rainbow fade across red, green, blue
      Serial.println("Pattern 1: Rainbow fade");
      for (uint8_t i = 0; i < 255; i++) {
        aw9523.AW_set_RGB(rainbowColor(i));
        delay(STEP_DELAY_MS);
      }
      break;

    case 1: // Pattern 2: Smooth cycling between red -> green -> blue
      Serial.println("Pattern 2: Smooth RGB cycle");
      fadeColor(RGB(255, 0, 0), RGB(0, 255, 0), 50, STEP_DELAY_MS); // Red -> Green
      fadeColor(RGB(0, 255, 0), RGB(0, 0, 255), 50, STEP_DELAY_MS); // Green -> Blue
      fadeColor(RGB(0, 0, 255), RGB(255, 0, 0), 50, STEP_DELAY_MS); // Blue -> Red
      break;
  }

  // Switch to next pattern
  rgb_pattern_index = (rgb_pattern_index + 1) % 2;
}