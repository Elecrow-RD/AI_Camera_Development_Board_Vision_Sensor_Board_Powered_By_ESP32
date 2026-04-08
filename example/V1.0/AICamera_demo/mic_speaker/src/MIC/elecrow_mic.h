#ifndef _ELECROW_MIC_H
#define _ELECROW_MIC_H

#include "ESP_I2S.h"
#include "FS.h"
#include "src\AW9523\elecrow_aw9523.h"  
#include "src\Audio\elecrow_audio.h"

class ELECROW_MIC : public I2SClass
{
  public:
    ELECROW_MIC(uint8_t Clk,uint8_t Dout,uint8_t I2S_SDATA,uint8_t I2S_BCLK,uint8_t I2S_LRCLK);

    ~ELECROW_MIC(){};

    void MIC_init();

    size_t i2s_write(const uint8_t *buffer, size_t size);

    uint8_t MIC_ReadinSD(fs::FS & fs,const char *path);

    uint8_t MIC_ReadandOut();

    void mic_test(fs::FS & fs,const char *path);

    void mic_test2();

  protected:

  private:

    uint8_t Mic_clk;

    uint8_t Mic_dout;

    uint8_t Spk_SDATA;

    uint8_t Spk_BCLK;

    uint8_t Spk_LRCLK;

    esp_err_t last_error;

    i2s_chan_handle_t tx_chan;

};
#endif