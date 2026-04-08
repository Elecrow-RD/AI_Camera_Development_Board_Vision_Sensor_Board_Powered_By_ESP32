#ifndef _ELECROW_AUDIO_H
#define _ELECROW_AUDIO_H

#include "ESP_I2S.h"
#include "src\AW9523\elecrow_aw9523.h"  

#define I2S_ERROR_CHECK_RETURN(x, r)                   \
  do {                                                 \
    last_error = (x);                                  \
    if (unlikely(last_error != ESP_OK)) {              \
      log_e("ERROR: %s", esp_err_to_name(last_error)); \
      return (r);                                      \
    }                                                  \
  } while (0)

class ELECROW_AUDIO : public I2SClass
{
  public:
    ELECROW_AUDIO(int SDATA,int BCLK,int LRCLK);

    ~ELECROW_AUDIO(){};

    void Audio_init();

    size_t i2s_write(const uint8_t *buffer, size_t size);

    //uint8_t Audio_play_wav(const uint8_t* fat,size_t size);
  protected:

  private:
  
  int I2S_SDATA;

  int I2S_BCLK;

  int I2S_LRCLK;

  esp_err_t last_error;

  i2s_chan_handle_t tx_chan;
};

// class ELECROW_AUDIO : public Audio
// {
//   public:

//     void Audio_init(uint8_t I2S_SDATA,uint8_t I2S_BCLK,uint8_t I2S_LRCLK);

//     void set_volume(uint8_t volume);

//     void Audio_wifi_connect(char* ssid,char* password);

//     void Audio_connect_host(const char* host);

//     void Audio_connect_fs(fs::FS &fs, const char* path);

//     void Audio_play();

//   protected:

//   private:

// };
#endif