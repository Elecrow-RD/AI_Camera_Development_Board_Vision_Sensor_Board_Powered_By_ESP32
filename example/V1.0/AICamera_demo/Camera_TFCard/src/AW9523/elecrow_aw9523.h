#ifndef _ELECROW_AW9253_H
#define _ELECROW_AW9253_H

#include <Adafruit_AW9523.h>

class ELECROW_AW9253 : public Adafruit_AW9523
{
  public:
    ELECROW_AW9253(){};

    ~ELECROW_AW9253(){};

    void AW_init();

    void AW_pinMode(uint8_t pin, uint8_t mode);

    void AW_digitalWrite(uint8_t pin, bool val);

    bool AW_digitalRead(uint8_t pin);

    void AW_analogWrite(uint8_t pin,uint8_t val);

    void AW_set_POWER(bool state);

    void AW_set_SLED(bool state);

    void AW_set_lcd_blight(uint8_t state);

    void AW_set_MUTE(bool state);

    void AW_set_RGB(uint32_t RGB);

    void AW_LED_test();
  protected:

  private:
    uint8_t WIRE_SDA;
    uint8_t WIRE_SCL;
};
/*IO Service*/
extern ELECROW_AW9253 aw9523;
#endif