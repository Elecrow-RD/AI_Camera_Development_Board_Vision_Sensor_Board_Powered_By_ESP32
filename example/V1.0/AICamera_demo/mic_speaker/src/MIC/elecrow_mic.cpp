#include "elecrow_mic.h"
#include <Base64.h>
#include <HTTPClient.h>
#include "cJSON.h"


ELECROW_MIC::ELECROW_MIC(uint8_t Clk,uint8_t Dout,uint8_t I2S_SDATA,uint8_t I2S_BCLK,uint8_t I2S_LRCLK)
{
  Mic_clk=Clk;
  Mic_dout=Dout;
  Spk_SDATA=I2S_SDATA;
  Spk_BCLK=I2S_BCLK;
  Spk_LRCLK=I2S_LRCLK;
  last_error=ESP_OK;
}

void ELECROW_MIC::MIC_init()
{
  setPinsPdmRx(Mic_clk, Mic_dout);
  if(!begin(I2S_MODE_PDM_RX, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO))
  {
    Serial.println("Failed to initialize I2S bus!");
    while (1); 
  }
  Serial.println("initialize I2S success!");
}

size_t ELECROW_MIC::i2s_write(const uint8_t *buffer, size_t size) 
{
  size_t written = 0;
  size_t bytes_sent = 0;
  size_t bytes_to_send = 5000;
  last_error = ESP_FAIL;
  tx_chan = txChan();
  if (tx_chan == NULL) {
    return written;
  }
  while (written < size) {
    bytes_sent = 0;
    if(written>=5000)
    {
      aw9523.AW_set_MUTE(false);
    }
    esp_err_t err = i2s_channel_write(tx_chan, (char *)(buffer + written), bytes_to_send, &bytes_sent, _timeout);
    setWriteError(err);
    I2S_ERROR_CHECK_RETURN(err, written);
    written += bytes_sent;
  }
  aw9523.AW_set_MUTE(true);
  return written;
}

uint8_t ELECROW_MIC::MIC_ReadinSD(fs::FS & fs,const char *path)
{
  uint8_t err=0;
  uint8_t *wav_buffer;
  size_t wav_size;
  Serial.println("Recording 5 seconds of audio data...");
  wav_buffer=recordWAV(5, &wav_size);
  Serial.println("Recording end");
  File file = fs.open(path,FILE_WRITE);
  if(!file)
  {
    Serial.println("Failed to created Audio_File!");
    err=1;
    return err;
  }
  if(!(file.write(wav_buffer,wav_size)))
  {
    Serial.println("Failed to write Audio_File!");
    err=2;
    return err;
  }
  Serial.println("writeing end");
  free(wav_buffer);
  file.close();
  return err;
}

uint8_t ELECROW_MIC::MIC_ReadandOut()
{
  uint8_t err=0;
  uint8_t *wav_buffer;
  size_t wav_size;
  uint8_t *stereo_buffer;
  Serial.println("Recording 5 seconds of audio data...");
  wav_buffer=recordWAV(5, &wav_size);
  Serial.println("Recording end");
  end();
  setPins(Spk_BCLK, Spk_LRCLK, Spk_SDATA);
  if (!begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) 
  {
    Serial.println("Failed to initialize I2S bus!");
    while (1);
  }
  Serial.println("Playing the recorded audio...");
  stereo_buffer = (uint8_t*)heap_caps_malloc((wav_size-44)*2, MALLOC_CAP_SPIRAM);
  if (stereo_buffer==NULL) 
  {
    Serial.println("Memory allocation failure!");
    err=1;
    return err;
  }
  for (size_t i=0,j=0; i<(wav_size-44);i+=2,j+=4) 
  {
    stereo_buffer[j]=0;
    stereo_buffer[j+1]=0;
    
    stereo_buffer[j+2]=(wav_buffer[i+44]);
    stereo_buffer[j+3]=(wav_buffer[i+1+44]);
  }
  i2s_write(stereo_buffer,(wav_size-44)*2);
  free(stereo_buffer);
  free(wav_buffer);
  end();

  return err;
}


void ELECROW_MIC::mic_test(fs::FS & fs,const char *path)
{
  static uint8_t state=0;
  static uint8_t err=0;
  if(Serial.available())
  {
    state=(uint8_t)Serial.read();
    Serial.printf("MIC state=%d",state);
  }
  if(state)
  {
    err=MIC_ReadinSD(fs,path);
    if(!err)
    {
      state=0;
    }  
  }
}


void ELECROW_MIC::mic_test2()
{
  // 初始化I2S输出
  setPins(Spk_BCLK, Spk_LRCLK, Spk_SDATA);
  if (!begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) 
  {
    Serial.println("Failed to initialize I2S bus!");
    return ;
  }

  const int sample_rate = 16000;
  const float frequency = 440.0;  // Hz
  const float duration = 0.3;     // 秒
  const int num_samples = sample_rate * duration;
  
  // 分配立体声缓冲区 (16位采样，2通道)
  int16_t *stereo_buffer = (int16_t*)heap_caps_malloc(num_samples * 2 * sizeof(int16_t), MALLOC_CAP_SPIRAM);
  if (stereo_buffer == NULL) 
  {
    Serial.println("Memory allocation failure!");
    return ;
  }

  // 生成正弦波
  for (int i = 0; i < num_samples; i++) 
  {
    float t = (float)i / sample_rate;
    int16_t sample = (int16_t)(32767.0 * sin(2.0 * M_PI * frequency * t));
    
    // 立体声 - 左右声道相同
    stereo_buffer[2*i] = sample;     // 左声道
    stereo_buffer[2*i + 1] = sample; // 右声道
  }

  // 播放声音
  Serial.println("Playing beep sound...");
  i2s_write((uint8_t*)stereo_buffer, num_samples * 2 * sizeof(int16_t));
  
  // 清理
  free(stereo_buffer);
  end();
  
}

