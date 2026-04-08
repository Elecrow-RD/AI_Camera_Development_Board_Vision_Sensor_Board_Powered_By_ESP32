// ============================================================
// Example: Toggle AW9523 LED with a physical button on GPIO0
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include "src/AW9523/elecrow_aw9523.h"  // AW9523 driver API

// ==================== Pin Definitions ====================
#define I2C_SCL 15           // I2C Clock
#define I2C_SDA 16           // I2C Data
#define BUTTON_PIN 0         // GPIO0 connected to a push button

// ==================== Global Variables ====================
static TwoWire* wi = &Wire;   // I2C bus
bool ledState = false;        // Current LED state (on/off)
bool lastButtonState = HIGH;  // Last button state (assumes pull-up)
unsigned long lastDebounceTime = 0;  
const unsigned long debounceDelay = 50;  // 50ms debounce

// ==================== Arduino Setup ====================
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;  // Wait for Serial to be ready
  }

  // ------------------- I2C Setup -------------------
  // Configure I2C pins
  wi->setPins(I2C_SDA, I2C_SCL);
  wi->begin();
  delay(100);

  // ------------------- AW9523 Setup -------------------
  aw9523.AW_init();         // Initialize the AW9523 chip
  aw9523.AW_set_POWER(true); // Enable the power domain for LEDs

  // ------------------- Button Setup -------------------
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // GPIO0 input with internal pull-up resistor
  Serial.println("Setup complete. Press the button to toggle the LED.");
}

// ==================== Arduino Main Loop ====================
void loop() {
  // Read the button state (active LOW)
  bool reading = digitalRead(BUTTON_PIN);

  // Check if the reading changed from the last loop
  if (reading != lastButtonState) {
    lastDebounceTime = millis();  // reset debounce timer
  }

  // Only toggle if the button has been stable for debounceDelay
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Edge detection: trigger only when HIGH → LOW
    static bool buttonPressed = false;
    if (reading == LOW && !buttonPressed) {
      // Button just pressed
      buttonPressed = true;  // mark as pressed
      ledState = !ledState;  // toggle LED
      aw9523.AW_set_SLED(ledState);
      Serial.printf("Button pressed. LED is now %s\n", ledState ? "ON" : "OFF");
    } 
    else if (reading == HIGH) {
      // Button released
      buttonPressed = false;
    }
  }

  lastButtonState = reading;  // save the last reading
}
