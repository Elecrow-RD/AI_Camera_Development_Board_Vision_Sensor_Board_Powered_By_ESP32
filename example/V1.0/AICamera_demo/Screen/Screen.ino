
#include <Arduino.h>
#include "src\GFX\LovyanGFX_Driver.h"
#include "src\AW9523\elecrow_aw9523.h" 

#define I2C_SCL                     15
#define I2C_SDA                     16

static TwoWire* wi = &Wire;

void setup(void)
{

  // Initialize I2C with project-defined pins
  wi->setPins(I2C_SDA, I2C_SCL);
  wi->begin();
  delay(100);

  // Initialize AW9523 and enable power domain used in the main project
  aw9523.AW_init();
  aw9523.AW_set_POWER(true);
  aw9523.AW_set_lcd_blight(100);
  // LovyanGFX init
  gfx.init();
  gfx.initDMA();
  gfx.startWrite();
  gfx.fillScreen(TFT_WHITE);
  gfx.setCursor(100, 140);
  gfx.printf("white");
}

void loop(void)
{

}