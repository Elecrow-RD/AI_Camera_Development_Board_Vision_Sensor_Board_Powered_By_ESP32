#include "app/AppContext.h"

// Arduino only auto-compiles the sketch root and src/. Keep app modules
// outside the existing src tree and aggregate their implementations here.
#include "app/iot/IotService.cpp"
#include "app/display/DisplayService.cpp"
#include "app/hardware/BoardControl.h"
#include "app/hardware/BoardControl.cpp"

// AppContext preserves the original anonymous namespace. Include declarations
// only after IotService.cpp closes that namespace, matching the old sketch.
#include "app/ai/AiService.h"
#include "app/tasks/BackgroundTasks.h"
#include "app/ui/UiRuntime.h"

void setup(void) {
  Serial.begin(115200);
  while (!Serial);

  pinMode(0, INPUT_PULLUP);
  /*I2C Init*/
  //wi->setPins(I2C_SDA, I2C_SCL);
  wi->begin(I2C_SDA, I2C_SCL,100000U);
  delay(200);
  /* extern chip init*/
  aw9523.AW_init();
  aw9523.AW_set_POWER(true);
  BoardControl_Init();
  delay(100);
  aw9523.AW_set_lcd_blight(0);
  lvgl_mutex = xSemaphoreCreateMutex();
  if (lvgl_mutex == NULL) {

  }
  /*LCD Init*/
  gfx.init();
  gfx.initDMA();
  gfx.startWrite();
  gfx.fillScreen(TFT_BLACK);

  // my_camera.camera_init(&config);
  // my_camera.camera_sensor_init();

  lv_init();
  size_t buffer_size = sizeof(lv_color_t) * screenWidth * screenHeight;
  static lv_color_t *buf = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  static lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  lv_disp_draw_buf_init(&draw_buf, buf, buf1, screenWidth * screenHeight);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  loadPreferences(g_cfg);

  if (g_cfg.ssid.isEmpty()) {
    restartFlag = true;
  } else {
    restartFlag = false;
  }

  camera_save_buf = (uint8_t *)heap_caps_malloc(236800, MALLOC_CAP_SPIRAM);

  /* cmaera SemaphoreHandle */
  camera_mutex = xSemaphoreCreateMutex();
  /* AI Recogition Result sem */
  sem_recognition = xSemaphoreCreateBinary();
  /* SD Card Task mutex */
  sd_mutex = xSemaphoreCreateMutex(); 
  /* LVGL & Camera timer*/
  camera_timer = lv_timer_create(lvgl_camera_update_cb, 30, NULL);

  InitIot();
  auto& ai_vox_engine = ai_vox::Engine::GetInstance();
  ai_vox_engine.SetObserver(g_observer);
  ai_vox_engine.SetTrigger(kTriggerPin);
  ai_vox_engine.SetOtaUrl("https://api.thinknode.cc/ota/");
  ai_vox_engine.ConfigWebsocket("wss://otc.thinknode.cc/v1/",
                                {
                                    {"Authorization", "Bearer test-token"},
                                });

  xTaskCreatePinnedToCore(RGB_Light_task, "RGB_Light_handle", 1024, NULL, 1, &RGB_Light_task_handle, 1);
  //xTaskCreatePinnedToCore(video_task, "video_stream", 4096, NULL, 6, &video_stream_task_handle, 1);
  xTaskCreatePinnedToCore(audio_task,   "audio_task_handle",  2048, NULL, 4, &audio_task_handle,    1);
  xTaskCreatePinnedToCore(SD_Card_task, "SD_Card_handle",     2048, NULL, 3, &SD_Card_task_handle,  1);
  xTaskCreatePinnedToCore(WiFi_task,    "wifi_handle",        8192, NULL, 1, &WiFi_task_handle,     1);
  xTaskCreatePinnedToCore(aichat_task,  "aichat_task_handle", 8192, NULL, 6, &aichat_task_handle,   1);
  ui_init();

  g_cfg.brightness = brightness_value;

  g_audio_output_device->SetVolume(speaker_value);
  g_speaker_iot_entity->UpdateState("volume", speaker_value);
  is_mute = true;
  
  aw9523.AW_set_lcd_blight(g_cfg.brightness);
}

String activationCode = "";
String challengeCode = "";

#include "app/ai/AiService.cpp"
#include "app/tasks/BackgroundTasks.cpp"
#include "app/ui/UiRuntime.cpp"

void loop() {
  lv_timer_handler(); 
    
  lv_wifi_rssi_show();
  system_set();
  AIChat_Status_lvgl();
  device_active_check_steup_handle();

  if (In_AIRecognition_Save_Src) {
    
    if (g_show_recognition) {
      g_show_recognition = false;
      lv_obj_add_flag(ui_AIRecognitionSpinner, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_arc_opa(ui_AIRecognitionSpinner, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
      lv_obj_set_style_arc_opa(ui_AIRecognitionSpinner, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
      _ui_screen_change(&ui_AIRecognitionSave, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_AIRecognitionSave_screen_init);
      camera_output = ui_AIRecognitionSaveImg;
      lv_img_set_src(camera_output, &img_dsc);
      /*
      if (xSemaphoreTake(sem_recognition, 0) == pdTRUE) {

        lv_label_set_text(ui_AIRecognitionResultLab, g_recognition_text);
        lv_obj_clear_flag(ui_AIRecognitionResultLab, LV_OBJ_FLAG_HIDDEN);
        vTaskResume(audio_task_handle);
      }
      */
    }
    if (img_save_to_sd_flag) {
      img_save_to_sd_flag = false;
      my_sd.SD_End();
      bool sd_init_result = my_sd.SD_init();
      lv_timer_t * timer = lv_timer_create(hide_sd_label_cb, 2100, ui_AIRecognitionSaveSDLab);
      lv_timer_set_repeat_count(timer, 1);
      if (sd_init_result == false) {
        lv_label_set_text(ui_AIRecognitionSaveSDLab, "SD card does not exist");
        lv_obj_clear_flag(ui_AIRecognitionSaveSDLab, LV_OBJ_FLAG_HIDDEN);
        return;
      }
      static int file_idx = 0;
      char filePath[32];                                 // 声明一个字符数组
      sprintf(filePath, "/capture_%d.bmp", file_idx++);  // 填充内容
      ELECROW_SD::SD_Save_Result res = my_sd.save_rgb565_to_bmp(SD, filePath, (uint16_t *)g_camera_frame, 240, 280);

      if (res != ELECROW_SD::SAVE_OK) {
        
      } else {
        // 成功提示
        lv_label_set_text(ui_AIRecognitionSaveSDLab, "Image saved to SD card");
        lv_obj_clear_flag(ui_AIRecognitionSaveSDLab, LV_OBJ_FLAG_HIDDEN);
      }

    }
  }

  if(AIChat_Device_Bound_Check == 1)
  {
      vTaskDelay(pdMS_TO_TICKS(300));
      if(ui_ServerConnectSrc != NULL)
      {
        lv_obj_add_flag(ui_Label4, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_ServerConnectSpinner, LV_OBJ_FLAG_HIDDEN);
      }
      _ui_screen_change(&ui_ServerConnectSrc, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_ServerConnectSrc_screen_init);
      //lv_label_set_text(ui_Label4, "Opening HTTP Connection\nto \"https://api.thinknode.cc/\nota/\"");
      device_active_check = true;
      device_active = false;
      current_active_screen = 1;
      aichat_steup = 1;
      if(is_mute == true)
      {
        BoardControl_SetAudioMuted(false);
        is_mute = false;
      }
      AIChat_Device_Bound_Check = 0;

  }
  else if (AIChat_Device_Bound_Check == 2)
  {
      vTaskDelay(pdMS_TO_TICKS(300));
      if(ui_ServerConnectSrc != NULL)
      {
        lv_obj_add_flag(ui_Label4, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_ServerConnectSpinner, LV_OBJ_FLAG_HIDDEN);
      }
      _ui_screen_change(&ui_ServerConnectSrc, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_ServerConnectSrc_screen_init);
      //lv_label_set_text(ui_Label4, "Opening HTTP Connection\nto \"https://xiaozhi.me/\nota/\"");
      device_active_check = true;
      device_active = false;
      current_active_screen = 2;
      aichat_steup = 1;
      if(is_mute == true)
      {
        BoardControl_SetAudioMuted(false);
        is_mute = false;
      }
      AIChat_Device_Bound_Check = 0;
  }
  else if (AIChat_Device_Bound_Check == 3)
  {   
      if(camera_is_no_init)
      {
          vTaskDelay(pdMS_TO_TICKS(100));
          camera_is_no_init = false;
          my_camera.camera_init(&config);
          my_camera.camera_sensor_init();
          xTaskCreatePinnedToCore(video_task, "video_stream", 4096, NULL, 6, &video_stream_task_handle, 1);
      }
      if(ui_AIRecognitionSpinner != NULL)
      {
          lv_obj_add_flag(ui_AIRecognitionSpinner, LV_OBJ_FLAG_HIDDEN);
      }
      //
      vTaskResume(video_stream_task_handle);
      camera_output = NULL;
      In_AIRecognition_Save_Src = false;
      lv_obj_add_flag(ui_AIRecognitionSaveSDLab, LV_OBJ_FLAG_HIDDEN);
      vTaskDelay(pdMS_TO_TICKS(300));
      if(ui_ServerConnectSrc != NULL)
      {
        lv_obj_add_flag(ui_Label4, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_ServerConnectSpinner, LV_OBJ_FLAG_HIDDEN);
      }

      //_ui_screen_change(&ui_ServerConnectSrc, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_ServerConnectSrc_screen_init);
      //lv_label_set_text(ui_Label4, "Opening HTTP Connection\nto \"https://api.thinknode.cc/\nota/\"");
      // 移除设备绑定查询,直接进入界面
      // device_active_check = true;
      // device_active = false;
      // aichat_steup = 1;
      gpio_isr_handler_remove(kTriggerPin);
      current_fun_is_Recognition = true;
      _ui_screen_change(&ui_AIRecognition, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_AIRecognition_screen_init);
      current_active_screen = 3;
      if(is_mute == true)
      {
        BoardControl_SetAudioMuted(false);
        is_mute = false;
      }
      AIChat_Device_Bound_Check = 0;
    }

    if(aichat_exit)
    {
      aichat_exit = false;
      lv_label_set_text(ui_Label2, "Closing...");
      auto& ai_vox_engine = ai_vox::Engine::GetInstance();
      ai_vox_engine.DisableconnectWebSocket();
      vTaskSuspend(aichat_task_handle);
      if(current_active_screen != 3)
      {
        vTaskDelay(pdMS_TO_TICKS(200));
        
        _ui_screen_change(&ui_AIChatExitScr, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_AIChatExitScr_screen_init);
        //_ui_screen_change(&ui_AIChatMenu, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_AIChatMenu_screen_init);
      }
      
    }

    if(current_fun_is_Recognition)
    {
      lv_obj_t* currentPage = lv_scr_act();

      if(currentPage == ui_AIRecognition) {
          int buttonState = digitalRead(0);

          if (lastButtonState == HIGH && buttonState == LOW) {
              buttonPressed = true;
          }
          lastButtonState = buttonState;  

          if (buttonPressed) {
              buttonPressed = false;  
              AI_Recognition_flag = true;  
          }
      } 
      else if(currentPage == ui_AIRecognitionSave) {
          int buttonState = digitalRead(0);

          if (lastButtonState == HIGH && buttonState == LOW) {
              buttonPressed = true;
          }

          lastButtonState = buttonState; 

          if (buttonPressed) {
              buttonPressed = false;  
              AI_Recognition_exit_flag = true;  
          }
      }
    }

    if(Touch)
    {
      Touch = 0;
      auto& ai_vox_engine = ai_vox::Engine::GetInstance();
      ai_vox_engine.Touch();
    }

    if(load_anim_flag)
    {
      load_anim_flag = false;
      lv_obj_clear_flag(ui_AIRecognitionSpinner, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_arc_opa(ui_AIRecognitionSpinner, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }

    if(AI_Recognition_flag)
    {

      AI_Recognition_flag = false;
      vTaskSuspend(video_stream_task_handle);

      camera_output = NULL;
      lv_obj_add_flag(ui_AIRecognitionTitleCont, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIRecognitionIdentifyBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_press_lab, LV_OBJ_FLAG_HIDDEN);
      //AIRecognition_start_flag = true;
      //关闭AI识别的功能
      //AIRecognition_flag = true;
      g_show_recognition = true;
      In_AIRecognition_Save_Src = true;

    }
    else if(AI_Recognition_exit_flag)
    {
      AI_Recognition_exit_flag = false;
      lv_obj_clear_flag(ui_AIRecognitionTitleCont, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(ui_AIRecognitionIdentifyBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(ui_press_lab, LV_OBJ_FLAG_HIDDEN);
      vTaskResume(video_stream_task_handle);
      camera_output = NULL;
      In_AIRecognition_Save_Src = false;
      lv_obj_add_flag(ui_AIRecognitionSaveSDLab, LV_OBJ_FLAG_HIDDEN);
      _ui_screen_change(&ui_AIRecognition, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_AIRecognition_screen_init);
    }
  
  vTaskDelay(pdMS_TO_TICKS(5));
}
