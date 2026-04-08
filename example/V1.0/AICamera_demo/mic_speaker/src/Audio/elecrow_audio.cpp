#include "elecrow_audio.h"

ELECROW_AUDIO::ELECROW_AUDIO(int SDATA,int BCLK,int LRCLK)
{
  I2S_SDATA=SDATA;
  I2S_BCLK=BCLK;
  I2S_LRCLK=LRCLK;
  last_error=ESP_OK;
}

void ELECROW_AUDIO::Audio_init()
{
  setPins(I2S_BCLK, I2S_LRCLK, I2S_SDATA);
  if (!begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) 
  {
    Serial.println("Failed to initialize I2S bus!");
    while (1);
  }
}

size_t ELECROW_AUDIO::i2s_write(const uint8_t *buffer, size_t size) 
{
  size_t written = 0;
  size_t bytes_sent = 0;
  size_t bytes_to_send = 4000;
  last_error = ESP_FAIL;
  tx_chan = txChan();
  if (tx_chan == NULL) {
    return written;
  }
  while (written < size) {
    bytes_sent = 0;
    if(written>=4000)
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

// uint8_t ELECROW_AUDIO::Audio_play_wav(const uint8_t*  ,size_t size)
// {
//   uint8_t err=0;
//   uint8_t *wav_buffer;
//   wav_buffer=(uint8_t*)heap_caps_malloc((size-44)*2, MALLOC_CAP_SPIRAM);
//   if(wav_buffer==NULL) 
//   {
//     Serial.println("Memory allocation failure!");
//     err=1;
//     return err;
//   }
//   for(size_t i=0,j=0; i<(size-44);i+=2,j+=4) 
//   {
//     int16_t sample = (fat[i+45] << 8) | fat[i+44];
//     sample = static_cast<int16_t>(sample * 0.1);

//     // 左声道（静音）
//     wav_buffer[j] = 0;
//     wav_buffer[j+1] = 0;

//     // 右声道（处理后的音频数据）
//     wav_buffer[j+2] = sample & 0xFF;       // 低8位
//     wav_buffer[j+3] = (sample >> 8) & 0xFF; // 高8位
//   }
//   write(wav_buffer,(size-44)*2);
//   free(wav_buffer);
//   //end();
//   return err;
// }

// void ELECROW_AUDIO::Audio_init(uint8_t I2S_SDATA,uint8_t I2S_BCLK,uint8_t I2S_LRCLK)
// {
//   setPinout(I2S_BCLK, I2S_LRCLK, I2S_SDATA);
// }

// void ELECROW_AUDIO::set_volume(uint8_t volume)
// {
//   setVolume(volume);
// }

// void ELECROW_AUDIO::Audio_wifi_connect(char* ssid,char* password)
// {
//   WiFi.persistent(false);
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(ssid, password);
//   if (WiFi.waitForConnectResult() != WL_CONNECTED) {
//     Serial.printf("WiFi failure %d\n", WiFi.status());
//     delay(5000);
//     ESP.restart();
//   }
//   Serial.println("WiFi connected");
// }

// void ELECROW_AUDIO::Audio_connect_host(const char* host)
// {
//   bool state=false;
//   state=connecttohost(host);
//   Serial.printf("connect_host state=%d",state);
// }

// void ELECROW_AUDIO::Audio_connect_fs(fs::FS &fs, const char* path)
// {
//   bool state=false;
//   state=connecttoFS(fs,path);
//   Serial.printf("connect_fs state=%d",state);
// }

// void ELECROW_AUDIO::Audio_play()
// {
//   static uint8_t state=0;
//   static uint8_t lock=0;
//   if(Serial.available())
//   {
//     state=(uint8_t)Serial.read();
//     Serial.printf("Audio state=%d",state);
//   }
//   if(state)
//   {
//     loop();
//     lock=1;
//   }
//   else if((!state)&&(lock))
//   {
//     stopSong();
//     Audio_connect_host("http://music.163.com/song/media/outer/url?id=2086327879.mp3");
//     lock=0;
//   }
    
// }




