#include <Arduino.h>
#include <Wire.h>
#include "src/AW9523/elecrow_aw9523.h"
#include "src\MIC\elecrow_mic.h"

/*IIC Pins*/
#define I2C_SCL   15
#define I2C_SDA   16

/*MIC Pins*/
#define MIC_CLK_PIN     1
#define MIC_DOUT_PIN    2

/*SPK Pins*/
#define I2S_DOUT_PIN    3
#define I2S_LRC_PIN     4
#define I2S_BCLK_PIN    5

static TwoWire * wi = &Wire;

static ELECROW_AUDIO my_audio(I2S_DOUT_PIN,I2S_BCLK_PIN,I2S_LRC_PIN);
static ELECROW_MIC my_mic(MIC_CLK_PIN, MIC_DOUT_PIN,I2S_DOUT_PIN,I2S_BCLK_PIN,I2S_LRC_PIN);

void setup(void)
{
    Serial.begin(115200);
    while(!Serial);

    wi->setPins(I2C_SDA, I2C_SCL);
    wi->begin();
    
    /*Power*/
    aw9523.AW_init();
    aw9523.AW_set_POWER(true);
    delay(100);
    aw9523.AW_set_MUTE(true);
    delay(100);
    aw9523.AW_set_MUTE(false);
    delay(100);
    aw9523.AW_set_MUTE(true);   
    
}
void loop()
{
    my_mic.MIC_init();
    my_mic.MIC_ReadandOut();
    delay(1000);

}