String GetActivationCode() {
  String deviceId = GetMacAddress();

  String url = "https://api.thinknode.cc/ota/";

  HTTPClient http;
  http.begin(url);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Device-Id", deviceId);

  DynamicJsonDocument doc(256);
  JsonObject application = doc.createNestedObject("application");
  JsonObject board = doc.createNestedObject("board");
  //board["type"] = "AICamera";
  String requestBody;
  serializeJson(doc, requestBody);

  int httpCode = http.POST(requestBody);
  String response = "";

  if (httpCode > 0) {
    response = http.getString();
  } else {
    http.end();
    return "";
  }

  http.end();

  DynamicJsonDocument jsonDoc(2048);  
  DeserializationError error = deserializeJson(jsonDoc, response);

  if (error) {
    return "";
  }

  if (jsonDoc.containsKey("activation") && jsonDoc["activation"].containsKey("code")) {

    activationCode = jsonDoc["activation"]["code"].as<String>();
    challengeCode = jsonDoc["activation"]["challenge"].as<String>();

    get_active_code  = true;
    return activationCode;
  }

  return "";
}


bool IsDeviceActivated() {
  if (device_active) {

    return true;
  }

  serial_number = getSerialNumber();

  String device_id = GetMacAddress();
  String Client_id = GenerateUUIDSimple();
  static char url[256]; 
  if(current_active_screen == 1)
  {
      strncpy(url, "https://api.thinknode.cc/ota/activate", sizeof(url) - 1);
  }
  else if(current_active_screen == 3)
  {
      strncpy(url, "https://api.thinknode.cc/ota/activate", sizeof(url) - 1);
  }
  else if(current_active_screen == 2)
  {
      strncpy(url, "https://api.tenclass.net/xiaozhi/ota/activate", sizeof(url) - 1);
                    
  }
  url[sizeof(url) - 1] = '\0';

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, url);

  String payload = GetActivationPayload();

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Device-Id", device_id);
  http.addHeader("Client-Id", Client_id);

  int status_code = http.POST("{}");
  String resp = http.getString();

  if (status_code == 202) {
    if (device_active_steup == ACTIVATE_IDLE) {
      device_active_steup = ACTIVATE_START;
    }

    http.end();
    return false;
  } else if (status_code == 200) {
    device_active = true;

    http.end();
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    return true;
  }

  http.end();
  return false;
}

uint8_t *save_camera_frame(const camera_fb_t *src_fb) {
  if (!src_fb || !src_fb->buf || src_fb->len == 0) {

    return NULL;
  }

  memcpy(camera_save_buf, src_fb->buf, src_fb->len);
  saved_buf_len = src_fb->len;
  return camera_save_buf;
}

void aichat_task(void *pvParameters)
{
    vTaskSuspend(aichat_task_handle);
    while(1)
    {
      switch(aichat_steup)
      {
        
          case 1:
          {
              auto& ai_vox_engine = ai_vox::Engine::GetInstance();
              if(current_active_screen == 2)
              {
                  ai_vox_engine.SetOtaUrl("https://api.tenclass.net/xiaozhi/ota/");
                  ai_vox_engine.ConfigWebsocket("wss://api.tenclass.net/xiaozhi/v1/",
                                                {
                                                    {"Authorization", "Bearer test-token"},
                                                });
              }
              else if(current_active_screen == 1)
              {
                  ai_vox_engine.SetOtaUrl("https://api.thinknode.cc/ota/");
                  ai_vox_engine.ConfigWebsocket("wss://otc.thinknode.cc/v1/",
                                                {
                                                    {"Authorization", "Bearer test-token"},
                                                });
              }
              ai_vox_engine.Start(audio_input_device, g_audio_output_device);
              aichat_steup = 2;
              break;
          }

          case 2:
          {
              const auto events = g_observer->PopEvents();
                for (auto& event : events) {
                  if (auto activation_event = std::get_if<ai_vox::Observer::ActivationEvent>(&event)) {
                    activationCode = activation_event->code.c_str();
                    get_active_code  = true;
                    printf("activation code: %s, message: %s\n", activation_event->code.c_str(), activation_event->message.c_str());
                  } else if (auto state_changed_event = std::get_if<ai_vox::Observer::StateChangedEvent>(&event)) {
                    //printf("state changed from %" PRIu8 " to %" PRIu8 "\n",
                          // static_cast<uint8_t>(state_changed_event->old_state),
                          // static_cast<uint8_t>(state_changed_event->new_state));
                    switch (state_changed_event->new_state) {
                      case ai_vox::ChatState::kIdle: {
                        printf("Idle\n");
                        aichat_status = AIChat_Idle;
                        break;
                      }
                      case ai_vox::ChatState::kIniting: {
                        printf("Initing...\n");
                        aichat_status = AIChat_Initing;
                        break;
                      }
                      case ai_vox::ChatState::kStandby: {
                        printf("Standby\n");
                        device_active = true;
                        aichat_status = AIChat_Standby;
                        break;
                      }
                      case ai_vox::ChatState::kConnecting: {
                        printf("Connecting...\n");
                        aichat_status = AIChat_Connecting;
                        break;
                      }
                      case ai_vox::ChatState::kListening: {
                        printf("Listening...\n");
                        aichat_status = AIChat_Listening;
                        break;
                      }
                      case ai_vox::ChatState::kSpeaking: {
                        printf("Speaking...\n");

                        aichat_status = AIChat_Speaking;
                        break;
                      }
                      default: {
                        break;
                      }
                    }
                  } else if (auto emotion_event = std::get_if<ai_vox::Observer::EmotionEvent>(&event)) {
                    printf("emotion: %s\n", emotion_event->emotion.c_str());
                    std::string emotion_name = emotion_event->emotion;
                    emotion_index = get_emotion_index(emotion_name); 
                  } else if (auto chat_message_event = std::get_if<ai_vox::Observer::ChatMessageEvent>(&event)) {
                    switch (chat_message_event->role) {
                      case ai_vox::ChatRole::kAssistant: {
                        printf("role: assistant, content: %s\n", chat_message_event->content.c_str());
                        on_chat_message(chat_message_event->content); 
                        break;
                      }
                      case ai_vox::ChatRole::kUser: {
                        printf("role: user, content: %s\n", chat_message_event->content.c_str());
                        on_chat_message(chat_message_event->content); 
                        break;
                      }
                    }
                  } else if (auto iot_message_event = std::get_if<ai_vox::Observer::IotMessageEvent>(&event)) {
                    printf("IOT message: %s, function: %s\n", iot_message_event->name.c_str(), iot_message_event->function.c_str());
                    for (const auto& [key, value] : iot_message_event->parameters) {
                      if (std::get_if<bool>(&value)) {
                        printf("key: %s, value: %s\n", key.c_str(), std::get<bool>(value) ? "true" : "false");
                      } else if (std::get_if<std::string>(&value)) {
                        printf("key: %s, value: %s\n", key.c_str(), std::get<std::string>(value).c_str());
                      } else if (std::get_if<int64_t>(&value)) {
                        printf("key: %s, value: %lld\n", key.c_str(), std::get<int64_t>(value));
                      }
                    }

                    if (iot_message_event->name == "Led") {
                      if (iot_message_event->function == "TurnOn") {
                        led_status = true;
                        aichat_led_open = true;
                        g_rgb_iot_entity->UpdateState("state", true);  // Note: Must UpdateState after change the device state
                      } else if (iot_message_event->function == "TurnOff") {
                        led_status = false;
                        aichat_led_close = true;
                        g_rgb_iot_entity->UpdateState("state", false);  // Note: Must UpdateState after change the device state
                      }
                    }
                    else if (iot_message_event->name == "RGB") {
                      if (iot_message_event->function == "TurnOn") {
                        aichat_RGB_Light_Open = true;
                        g_rgb_iot_entity->UpdateState("state", true);  // Note: Must UpdateState after change the device state
                      } else if (iot_message_event->function == "TurnOff") {
                        aichat_RGB_Light_Close = true;

                        g_rgb_iot_entity->UpdateState("state", false);  // Note: Must UpdateState after change the device state
                      }
                    }
                    else if (iot_message_event->name == "Speaker") {
                      if (iot_message_event->function == "SetVolume") {
                        if (const auto it = iot_message_event->parameters.find("volume"); it != iot_message_event->parameters.end()) {
                          auto volume = it->second;
                          if (std::get_if<int64_t>(&volume)) {
                            const int64_t requested = std::get<int64_t>(volume);
                            const uint8_t applied = static_cast<uint8_t>(
                                requested < 0 ? 0 : (requested > 100 ? 100 : requested));

                            // Synchronise the shared UI value immediately. The
                            // UI loop will persist g_cfg.volume on its next pass.
                            speaker_value = applied;
                            g_audio_output_device->SetVolume(applied);
                            g_speaker_iot_entity->UpdateState("volume", applied);
                            if (applied > 0 && is_mute) {
                              BoardControl_SetAudioMuted(false);
                              is_mute = false;
                            }
                            printf("Speaker volume applied: %u%%\n",
                                   static_cast<unsigned>(applied));
                          }
                        }
                      }
                    }
                    else if (iot_message_event->name == "Screen") {
                      if (iot_message_event->function == "TurnOn") {
                        screen_power = true;
                        aw9523.AW_set_lcd_blight(g_cfg.brightness);
                        g_screen_iot_entity->UpdateState("power", true);  // Note: Must UpdateState after change the device state
                      } else if (iot_message_event->function == "TurnOff") {
                        screen_power = false;
                        aw9523.AW_set_lcd_blight(0);
                        g_screen_iot_entity->UpdateState("power", false);  // Note: Must UpdateState after change the device state
                      }else if (iot_message_event->function == "SetBrightness") {
                        if (const auto it = iot_message_event->parameters.find("brightness"); it != iot_message_event->parameters.end()) {
                          auto brightness = it->second;
                          if (std::get_if<int64_t>(&brightness)) {
                            uint8_t temp_value = std::get<int64_t>(brightness);
                            brightness_value = (temp_value < 1) ? 1 : (temp_value > 100 ? 100 : temp_value);
                            g_screen_iot_entity->UpdateState("brightness", std::get<int64_t>(brightness));  // Note: Must UpdateState after change the device state
                          }
                        }
                      }
                    }
                    
                    
                  }
                }
              IsDeviceActivated();
              break;
          }
      }
      
      // Drain observer events promptly so bursts of state, transcript,
      // emotion and IoT messages do not delay control commands.
      vTaskDelay(pdMS_TO_TICKS(20));
    }
    
}

bool result_save_flag = false;
void AI_Recognition_func() {
  camera_fb_t fake_fb;
  uint8_t *jpg_buf = NULL;
  size_t jpg_size = 0;

  fake_fb.buf = camera_save_buf;
  fake_fb.len = saved_buf_len;
  fake_fb.width = 400;
  fake_fb.height = 296;
  fake_fb.format = PIXFORMAT_RGB565;

  vTaskSuspend(video_stream_task_handle);

  if (!frame2jpg(&fake_fb, 80, &jpg_buf, &jpg_size)) {

    return;
  }
  String imageBase64 = base64::encode(jpg_buf, jpg_size);
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure();

  String deviceAddr = GetMacAddress();
  deviceAddr.toLowerCase();

  const String url = "https://service.thinknode.cc/api/users/image-analysis-for-eps32-camera";
  const String token = "3696b69a9aa3b624cac9f691797be9e9";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + token);

  String jsonRequest;
  jsonRequest += "{";
  jsonRequest += "\"role\": \"You are an object recognition assistant. Only return the names of the objects in the picture, separated by commas. Do not add any explanations or extra text.\",";
  jsonRequest += "\"image\": \"data:image/jpeg;base64," + imageBase64 + "\",";
  jsonRequest += "\"mac_address\": \"" + deviceAddr + "\"";
  jsonRequest += "}";

  int httpResponseCode = http.POST(jsonRequest);

  String response = http.getString();

  if (httpResponseCode == 200) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, response);

    if (error) {

      return;
    }

    if (doc.containsKey("code")) {
      int code = doc["code"];

      if (code == 200) {

        if (doc.containsKey("data") && doc["data"].containsKey("response")) {
          const char *content = doc["data"]["response"];

          strncpy(g_recognition_text,
                  content,
                  TEXT_MAX_LEN - 1);

          g_recognition_text[TEXT_MAX_LEN - 1] = '\0';

          if(lv_scr_act() == ui_AIRecognition)
          {
              g_show_recognition = true;
              if (sem_recognition != NULL) {
                xSemaphoreGive(sem_recognition);
              }
          }
        }

      } else if (code == 300) {
          strncpy(g_recognition_text, "the device is not bound", TEXT_MAX_LEN - 1);
          g_recognition_text[TEXT_MAX_LEN - 1] = '\0';
          g_show_recognition = true;
          if (sem_recognition != NULL) {
              xSemaphoreGive(sem_recognition);
          }
      } else if (code == 400) {
        if (doc.containsKey("msg")) {
          const char *msg = doc["msg"];
          g_show_recognition = true;
          strncpy(g_recognition_text,
                  msg,
                  TEXT_MAX_LEN - 1);
          if (sem_recognition != NULL) {
            xSemaphoreGive(sem_recognition);
          }
        }
      }
    } else {
    }
  } else {
    String msg = "HTTP Error: " + String(httpResponseCode);

    snprintf(g_recognition_text, TEXT_MAX_LEN, "The network is poor", httpResponseCode);

    g_recognition_text[TEXT_MAX_LEN - 1] = '\0';

    if(lv_scr_act() == ui_AIRecognition)
    {
        g_show_recognition = true;
        if (sem_recognition != NULL) {
          xSemaphoreGive(sem_recognition);
        }
    }
  }

  http.end();
  free(jpg_buf);
}

