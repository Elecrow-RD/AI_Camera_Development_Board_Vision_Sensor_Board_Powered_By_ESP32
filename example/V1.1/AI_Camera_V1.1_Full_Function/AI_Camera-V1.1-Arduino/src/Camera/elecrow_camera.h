#ifndef _ELECROW_CAMERA_H
#define _ELECROW_CAMERA_H

#include <WiFi.h>
#include <esp_camera.h>
#include <WebServer.h>

class ELECROW_CAMERA
{
  public:

    ELECROW_CAMERA();

    ~ELECROW_CAMERA();
    
    void camera_init(camera_config_t *config);
    
    void camera_sensor_init(void);

    void camera_web(char* ssid,char* password);

    void addRequestHandlers();
    
    void camera_server_handleClient(void);

    camera_fb_t* camera_fb_get(void);

    void camera_fb_return(camera_fb_t *fb);

    void camera_deinit();
    
  protected:
  
  private:
  
};



#endif