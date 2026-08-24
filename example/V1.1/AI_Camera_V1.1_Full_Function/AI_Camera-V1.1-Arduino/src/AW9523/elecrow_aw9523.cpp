#include "elecrow_aw9523.h"

ELECROW_AW9253 aw9523;

void ELECROW_AW9253::AW_init()
{
  if(!begin(0x5A)) 
  {
    Serial.println("AW9523 not found? Check wiring!");
    while(1) delay(10); 
  }

  Serial.println("AW9523 found!");

    pinMode(4,AW9523_LED_MODE);
    pinMode(5,AW9523_LED_MODE);
    pinMode(6,AW9523_LED_MODE);
    pinMode(11,AW9523_LED_MODE);
  }

void ELECROW_AW9253::AW_pinMode(uint8_t pin, uint8_t mode)
{
  pinMode(pin,mode);
}

void ELECROW_AW9253::AW_digitalWrite(uint8_t pin, bool val)
{
  digitalWrite(pin,val);
}

bool ELECROW_AW9253::AW_digitalRead(uint8_t pin)
{
  digitalRead(pin);
}

void ELECROW_AW9253::AW_analogWrite(uint8_t pin,uint8_t val)
{
  analogWrite(pin,val);
}


void ELECROW_AW9253::AW_set_POWER(bool state)
{
  pinMode(8,OUTPUT);
  digitalWrite(8,state);
}

void ELECROW_AW9253::AW_set_SLED(bool state)
{
  pinMode(9,OUTPUT);
  digitalWrite(9,state);
}

/*brightness:0-255*/
void ELECROW_AW9253::AW_set_lcd_blight(uint8_t brightness)
{
  if(brightness!=0)
    analogWrite(11,((brightness*2)+55));
  else
    analogWrite(11,0);
}


void ELECROW_AW9253::AW_set_MUTE(bool state)
{
  
  pinMode(12,OUTPUT);
  digitalWrite(12,state);
}

void ELECROW_AW9253::AW_set_RGB(uint32_t RGB)
{
  uint8_t R,G,B;
  R=(RGB&0xFF0000)>>16;
  G=(RGB&0x00FF00)>>8;
  B=(RGB&0x0000FF);
  analogWrite(5,R);
  analogWrite(4,G);
  analogWrite(6,B);
}

