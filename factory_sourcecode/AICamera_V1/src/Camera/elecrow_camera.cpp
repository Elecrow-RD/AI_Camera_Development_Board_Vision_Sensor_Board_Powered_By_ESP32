#include "elecrow_camera.h"

//WebServer server(80);

ELECROW_CAMERA::ELECROW_CAMERA(void) {}

ELECROW_CAMERA::~ELECROW_CAMERA(void) {}


void ELECROW_CAMERA::camera_init(camera_config_t *config)
{

  esp_err_t err = esp_camera_init(config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x\r\n", err);
    while(1);
  }
}

void ELECROW_CAMERA::camera_sensor_init()
{
  sensor_t *s=esp_camera_sensor_get();
  if(s!=NULL)
  {
    #ifdef DEBUG_PRINT
    Serial.printf("Camera ID = 0x%x\r\n",s->id.PID);
    
    switch(s->id.PID)
    {
      case 0x96:
        Serial.println("Camera is OV9650");
        break;
      case 0x77:
        Serial.println("Camera is OV7725");
        break;
      case 0x26:
        Serial.println("Camera is OV2640");
        break;
      case 0x3660:
        Serial.println("Camera is OV3660");
        break;
      case 0x5640:
        Serial.println("Camera is OV5640");
        break;
      case 0x76:
        Serial.println("Camera is OV7670");
        break;
      case 0x1410:
        Serial.println("Camera is NT99141");
        break;
      case 0x2145:
        Serial.println("Camera is GC2145");
        break;
      case 0x232a:
        Serial.println("Camera is GC032A");
        break;
      case 0x9b:
        Serial.println("Camera is GC0308");
        break;
      case 0x30:
        Serial.println("Camera is BF3005");
        break;
      case 0x20a6:
        Serial.println("Camera is BF20A6");
        break;
      case 0xda4a:
        Serial.println("Camera is SC101IOT");
        break;
      case 0x9a46:
        Serial.println("Camera is SC030IOT");
        break;
      case 0x0031:
        Serial.println("Camera is SC031GS");
        break;
    }
    #endif
    s->set_brightness(s, -2);     // -2 to 2
    s->set_contrast(s, -2);       // -2 to 2
    s->set_saturation(s, -2);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 0);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    s->set_exposure_ctrl(s, 0);  // 0 = disable , 1 = enable
    s->set_aec2(s, 1);           // 0 = disable , 1 = enable
    s->set_ae_level(s, -2);       // -2 to 2
    s->set_aec_value(s, 0);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
  }

}

camera_fb_t* ELECROW_CAMERA::camera_fb_get(void)
{
  return esp_camera_fb_get();
}

void ELECROW_CAMERA::camera_fb_return(camera_fb_t *fb)
{
  esp_camera_fb_return(fb);
}

void ELECROW_CAMERA::camera_deinit(void)
{
  esp_camera_deinit();
}

