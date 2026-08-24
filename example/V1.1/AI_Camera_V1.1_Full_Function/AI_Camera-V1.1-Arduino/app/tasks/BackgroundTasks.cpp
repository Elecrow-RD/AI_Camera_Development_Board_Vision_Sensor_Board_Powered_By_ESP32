void RGB_Light_task(void *pvParameters) {
  static uint8_t RGB_index;
  vTaskSuspend(RGB_Light_task_handle);
  while (1) {
    switch (RGB_index) {
      case 0: RGB_Light_index = 1; break;
      case 1: RGB_Light_index = 2; break;
      case 2: RGB_Light_index = 3; break;
    }
    RGB_index = (RGB_index + 1) % 3;  
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void video_task(void *pvParameters) {
  camera_fb_t *fb = NULL;

  static uint16_t *cropped_buf = (uint16_t *)heap_caps_malloc(screenWidth * screenHeight * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
  if (!cropped_buf) {
    vTaskDelete(NULL);
  }

  vTaskSuspend(video_stream_task_handle);

  while (1) {
    if (xSemaphoreTake(camera_mutex, portMAX_DELAY)) {
      fb = esp_camera_fb_get();

      if (!fb) continue;

      camera_save_buf = save_camera_frame(fb);

      if (fb->format == PIXFORMAT_RGB565) {
        uint16_t *src = (uint16_t *)fb->buf;
        int x_offset = (400 - 240) / 2;
        float y_scale = 284.0f / 296.0f;

        for (int y = 0; y < 284; y++) {
          int src_y = (int)(y / y_scale);
          for (int x = 0; x < 240; x++) {
            int src_x = x + x_offset;
            uint16_t pixel = src[src_y * 400 + src_x];
            cropped_buf[y * 240 + x] = (pixel << 8) | (pixel >> 8);
          }
        }

        g_camera_frame = cropped_buf;
        g_camera_frame_ready = true;
      }
      esp_camera_fb_return(fb);
      xSemaphoreGive(camera_mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(30));
  }
}

void audio_task(void *pvParameters) {
  while(1)
  {
    
    if(is_mute)
    {
      is_mute = false;
      vTaskDelay(50);
      BoardControl_SetAudioMuted(false);
      vTaskDelay(100);
    }

    size_t size = sizeof(Audio) / sizeof(Audio[0]);
    uint8_t *wav_buffer = (uint8_t*)heap_caps_malloc(size-44, MALLOC_CAP_SPIRAM);
    if (!wav_buffer) {
        return ;
    }

    memcpy(wav_buffer, Audio + 44, size - 44);

    size_t sample_count = (size - 44) / 2; 
    std::vector<int16_t> i2s_buffer(sample_count * 2); 

    for (size_t i = 0; i < sample_count; i++) {
        int16_t sample = (wav_buffer[i*2+1] << 8) | wav_buffer[i*2]; 
        sample = static_cast<int16_t>(sample * 0.1); 

        i2s_buffer[i] = sample;
    }

    free(wav_buffer);

    if(g_audio_output_device->Open(16000))
    {
      g_audio_output_device->Write(i2s_buffer.data(), i2s_buffer.size());
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    i2s_buffer.clear();         
    g_audio_output_device->SetVolume(speaker_value);
    vTaskSuspend(NULL);
  }
}

void SD_Card_task(void *pvParameters) {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      

    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(0))) {
        change_screen_sd_card = true;
        my_sd.SD_End();
        bool sd_init_result = my_sd.SD_init();
        if (sd_init_result) {
          
          vTaskDelay(pdMS_TO_TICKS(200));
          my_sd.SD_test();
          Percentages = String(percentage, 0) + "% Remaining";
          strncpy(g_sdcard_text,
                  Percentages.c_str(),
                  TEXT_MAX_LEN - 1);

          g_sdcard_text[TEXT_MAX_LEN - 1] = '\0';
          SD_Card_info_updata = true;

        } else {
          strncpy(g_sdcard_text,
                  "No Memory Card",
                  TEXT_MAX_LEN - 1);
          g_sdcard_text[TEXT_MAX_LEN - 1] = '\0';
          SD_Card_info_updata = true;
        }

      xSemaphoreGive(sd_mutex);
    }

  }
}

void WiFi_task(void *pvParameters) {

  while (1) {
    if (AIRecognition_flag == true) {
      AIRecognition_flag = false;
      state = WIFI_ONLINE_AI;
    }
    switch (state) {
      case WIFI_INIT:
        current_wifi_connected = false;
        connectToWiFi();
        state = WIFI_CONNECTED;
        setting_ap = false;
        break;

      case WIFI_CONNECTED:
        if (WiFi_Long_Press_Reset) {
          WiFi_Long_Press_Reset = false;
          current_wifi_connected = false;
          setting_ap = true;
        }

        if (!setting_ap) {
          checkWiFiConnection();
        }
        break;
      case WIFI_ONLINE_AI:
          load_anim_flag = true;
          AI_Recognition_func();

          state = WIFI_CONNECTED;
        break;
      default:
        state = WIFI_INIT;
        break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

