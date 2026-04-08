#include "ai_vox_engine_impl.h"

#include <cJSON.h>
#include <esp_crt_bundle.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ai_vox_observer.h"
#include "audio_input_engine.h"
#include "audio_output_engine.h"
#include "espressif_button/button_gpio.h"
#include "espressif_button/iot_button.h"
#include "fetch_config.h"

#ifndef CLOGGER_SEVERITY
#define CLOGGER_SEVERITY CLOGGER_SEVERITY_WARN
#endif

#include "clogger/clogger.h"

namespace ai_vox {

namespace {

enum WebScoketFrameType : uint8_t {
  kWebsocketTextFrame = 0x01,    // 文本帧
  kWebsocketBinaryFrame = 0x02,  // 二进制帧
  kWebsocketCloseFrame = 0x08,   // 关闭连接
  kWebsocketPingFrame = 0x09,    // Ping 帧
  kWebsocketPongFrame = 0x0A,    // Pong 帧
};

std::string GetMacAddress() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char mac_str[18] = {0};
  snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return std::string(mac_str);
}

std::string Uuid() {
  // UUID v4 需要 16 字节的随机数据
  uint8_t uuid[16] = {0};

  // 使用 ESP32 的硬件随机数生成器
  esp_fill_random(uuid, sizeof(uuid));

  // 设置版本 (版本 4) 和变体位
  uuid[6] = (uuid[6] & 0x0F) | 0x40;  // 版本 4
  uuid[8] = (uuid[8] & 0x3F) | 0x80;  // 变体 1

  // 将字节转换为标准的 UUID 字符串格式
  char uuid_str[37] = {0};
  snprintf(uuid_str,
           sizeof(uuid_str),
           "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
           uuid[0],
           uuid[1],
           uuid[2],
           uuid[3],
           uuid[4],
           uuid[5],
           uuid[6],
           uuid[7],
           uuid[8],
           uuid[9],
           uuid[10],
           uuid[11],
           uuid[12],
           uuid[13],
           uuid[14],
           uuid[15]);

  return std::string(uuid_str);
}

void DeleteCjsonObj(cJSON *obj) {
  if (obj != nullptr) {
    cJSON_Delete(obj);
  }
}

void CJSONFree(void *obj) {
  if (obj != nullptr) {
    cJSON_free(obj);
  }
}
}  // namespace

EngineImpl &EngineImpl::GetInstance() {
  static std::once_flag s_once_flag;
  static EngineImpl *s_instance = nullptr;
  std::call_once(s_once_flag, []() { s_instance = new EngineImpl; });
  return *s_instance;
}

EngineImpl::EngineImpl()
    : uuid_(Uuid()),
      ota_url_("https://api.tenclass.net/xiaozhi/ota/"),
      websocket_url_("wss://api.tenclass.net/xiaozhi/v1/"),
      websocket_headers_{
          {"Authorization", "Bearer test-token"},
      },
#ifdef ARDUINO_ESP32S3_DEV
      //wake_net_([this]() { task_queue_.Enqueue([this]() { OnWakeUp(); }); }),
#endif
      task_queue_("AiVoxMain", 1024 * 4, tskIDLE_PRIORITY + 1) {
  CLOGD();
}

EngineImpl::~EngineImpl() {
  CLOGD();
  // TODO
}

void EngineImpl::SetObserver(std::shared_ptr<Observer> observer) {
  std::lock_guard lock(mutex_);
  if (state_ != State::kIdle) {
    return;
  }

  observer_ = std::move(observer);
}

void EngineImpl::SetTrigger(const gpio_num_t gpio) {
  std::lock_guard lock(mutex_);
  if (state_ != State::kIdle) {
    return;
  }

  trigger_pin_ = gpio;
}

void EngineImpl::SetOtaUrl(const std::string url) {
  std::lock_guard lock(mutex_);
  // if (state_ != State::kIdle) {
  //   return;
  // }
  ota_url_ = std::move(url);
}

void EngineImpl::ConfigWebsocket(const std::string url, const std::map<std::string, std::string> headers) {
  std::lock_guard lock(mutex_);
  // if (state_ != State::kIdle) {
  //   return;
  // }

  websocket_url_ = std::move(url);
  for (auto [key, value] : headers) {
    websocket_headers_.insert_or_assign(std::move(key), std::move(value));
  }
}

void EngineImpl::RegisterIotEntity(std::shared_ptr<iot::Entity> entity) {
  std::lock_guard lock(mutex_);
  if (state_ != State::kIdle) {
    return;
  }
  iot_manager_.RegisterEntity(std::move(entity));
}

void EngineImpl::Start(std::shared_ptr<AudioInputDevice> audio_input_device, std::shared_ptr<AudioOutputDevice> audio_output_device) {
  CLOGD();

  std::lock_guard lock(mutex_);
  // if (state_ != State::kIdle) {
  //   printf("state != Kidle\n");
  //   return;
  // }
  if (web_socket_client_ != nullptr) {
      esp_websocket_client_stop(web_socket_client_);
      vTaskDelay(pdMS_TO_TICKS(300));
      esp_websocket_client_destroy(web_socket_client_);
      web_socket_client_ = nullptr;
      vTaskDelay(pdMS_TO_TICKS(300));
  }

  audio_input_device_ = std::move(audio_input_device);
  audio_output_device_ = std::move(audio_output_device);

  button_config_t btn_cfg = {
      .long_press_time = 1000,
      .short_press_time = 50,
  };

  button_gpio_config_t gpio_cfg = {
      .gpio_num = trigger_pin_,
      .active_level = 0,
      .enable_power_save = true,
      .disable_pull = false,
  };

  ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &button_handle_));
  ESP_ERROR_CHECK(iot_button_register_cb(button_handle_, BUTTON_SINGLE_CLICK, nullptr, OnButtonClick, this));
  ChangeState(State::kInited);

  LoadProtocol();

  esp_websocket_client_config_t websocket_cfg;
  memset(&websocket_cfg, 0, sizeof(websocket_cfg));
  websocket_cfg.uri = websocket_url_.c_str();
  websocket_cfg.task_prio = tskIDLE_PRIORITY;
  websocket_cfg.crt_bundle_attach = esp_crt_bundle_attach;
  websocket_cfg.buffer_size = 1024;
  websocket_cfg.use_global_ca_store = false;

  CLOGI("url: %s", websocket_cfg.uri);
  web_socket_client_ = esp_websocket_client_init(&websocket_cfg);
  if (web_socket_client_ == nullptr) {
    CLOGE("esp_websocket_client_init failed with %s", websocket_cfg.uri);
    abort();
  }
  for (const auto &[key, value] : websocket_headers_) {
    esp_websocket_client_append_header(web_socket_client_, key.c_str(), value.c_str());
  }
  esp_websocket_client_append_header(web_socket_client_, "Protocol-Version", "1");
  esp_websocket_client_append_header(web_socket_client_, "Device-Id", GetMacAddress().c_str());
  esp_websocket_client_append_header(web_socket_client_, "Client-Id", uuid_.c_str());
  esp_websocket_register_events(web_socket_client_, WEBSOCKET_EVENT_ANY, &EngineImpl::OnWebsocketEvent, this);
}

void EngineImpl::OnButtonClick(void *button_handle, void *self) {
  reinterpret_cast<EngineImpl *>(self)->OnButtonClick();
}

void EngineImpl::OnWebsocketEvent(void *self, esp_event_base_t base, int32_t event_id, void *event_data) {

  reinterpret_cast<EngineImpl *>(self)->OnWebsocketEvent(base, event_id, event_data);
}

void EngineImpl::OnButtonClick() {
  task_queue_.Enqueue([this]() { OnTriggered(); });
}

void EngineImpl::OnWebsocketEvent(esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
  switch (event_id) {
    case WEBSOCKET_EVENT_BEGIN: {
      CLOGI("WEBSOCKET_EVENT_BEGIN");
      break;
    }
    case WEBSOCKET_EVENT_CONNECTED: {
      CLOGI("WEBSOCKET_EVENT_CONNECTED");
      task_queue_.Enqueue([this]() { OnWebSocketConnected(); });
      break;
    }
    case WEBSOCKET_EVENT_DISCONNECTED: {
      CLOGI("WEBSOCKET_EVENT_DISCONNECTED");
      printf("WEBSOCKET_EVENT_DISCONNECTED\n");
      task_queue_.Enqueue([this]() { OnWebSocketDisconnected(); });
      break;
    }
    case WEBSOCKET_EVENT_DATA: {
      if (!data->fin) {
        abort();
      }

      switch (data->op_code) {
        case kWebsocketTextFrame: {
          FlexArray<uint8_t> frame(data->data_len);
          memcpy(frame.data(), data->data_ptr, data->data_len);
          task_queue_.Enqueue([this, frame = std::move(frame)]() mutable { OnJsonData(std::move(frame)); });
          break;
        }
        case kWebsocketBinaryFrame: {
          FlexArray<uint8_t> frame(data->data_len);
          memcpy(frame.data(), data->data_ptr, data->data_len);
          // printf("WEBSOCKET_EVENT_DATA: len=%d\n", data->data_len);
          task_queue_.Enqueue([this, frame = std::move(frame)]() mutable { OnAudioFrame(std::move(frame)); });
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    case WEBSOCKET_EVENT_ERROR: {
      CLOGE("WEBSOCKET_EVENT_ERROR");
      break;
    }
    case WEBSOCKET_EVENT_FINISH: {
      CLOGI("WEBSOCKET_EVENT_FINISH");
      task_queue_.Enqueue([this]() { OnWebSocketDisconnected(); });
      break;
    }
    default: {
      break;
    }
  }
}

// std::vector<uint8_t> audio_buffer;

// void EngineImpl::OnAudioFrame(FlexArray<uint8_t> &&data) {
//     audio_buffer.insert(audio_buffer.end(), data.data(), data.data() + data.size());
//     if (audio_buffer.size() >= kI2SBlockSize) {
//         FlexArray<uint8_t> block(audio_buffer.size());
//         memcpy(block.data(), audio_buffer.data(), audio_buffer.size());
//         audio_output_engine_->Write(std::move(block));
//         audio_buffer.clear();
//     }
// }
void EngineImpl::OnAudioFrame(FlexArray<uint8_t> &&data) {
  if (audio_output_engine_) {
    audio_output_engine_->Write(std::move(data));
  }
}

void EngineImpl::OnJsonData(FlexArray<uint8_t> &&data) {
  std::unique_ptr<cJSON, decltype(&DeleteCjsonObj)> root_obj(cJSON_ParseWithLength(reinterpret_cast<const char *>(data.data()), data.size()),
                                                             &DeleteCjsonObj);
  if (!cJSON_IsObject(root_obj.get())) {
    CLOGE("Invalid JSON data");
    return;
  }

  std::string type;
  auto *type_json = cJSON_GetObjectItem(root_obj.get(), "type");
  if (cJSON_IsString(type_json)) {
    type = type_json->valuestring;
  } else {
    CLOGE("Missing or invalid 'type' field in JSON data");
    return;
  }
  CLOGI("Received JSON type: %s", type.c_str());
  
  if (type == "hello") {
    const auto state = state_;
    if (state_ != State::kWebsocketConnected && state_ != State::kWebsocketConnectedWithWakeup) {
      CLOGE("Invalid state: %u", state_);
      return;
    }

    auto session_id_json = cJSON_GetObjectItem(root_obj.get(), "session_id");
    if (cJSON_IsString(session_id_json)) {
      session_id_ = session_id_json->valuestring;
      CLOGI("Session ID: %s", session_id_.c_str());
    }

    SendIotDescriptions();
    SendIotUpdatedStates(true);
    StartListening();

    if (state == State::kWebsocketConnectedWithWakeup) {
      std::unique_ptr<cJSON, decltype(&DeleteCjsonObj)> message_obj(cJSON_CreateObject(), &DeleteCjsonObj);
      cJSON_AddStringToObject(message_obj.get(), "session_id", session_id_.c_str());
      cJSON_AddStringToObject(message_obj.get(), "type", "listen");
      cJSON_AddStringToObject(message_obj.get(), "state", "detect");
      cJSON_AddStringToObject(message_obj.get(), "text", "你好小智");
      auto json_str = cJSON_PrintUnformatted(message_obj.get());
      CLOGI("Sending JSON: %s", json_str);
      esp_websocket_client_send_text(web_socket_client_, json_str, strlen(json_str), pdMS_TO_TICKS(5000));
    }
  } else if (type == "goodbye") {
    auto session_id_json = cJSON_GetObjectItem(root_obj.get(), "session_id");
    std::string session_id;
    if (cJSON_IsString(session_id_json)) {
      if (session_id_ != session_id_json->valuestring) {
        return;
      }
    }
  } else if (type == "tts") {
    auto *state_json = cJSON_GetObjectItem(root_obj.get(), "state");
    if (cJSON_IsString(state_json)) {
      if (strcmp("start", state_json->valuestring) == 0) {
        CLOG("tts start");

        if (state_ == State::kSpeaking) {
          CLOGI("already speaking");
          return;
        } else if (state_ != State::kListening) {
          CLOGW("invalid state: %u", state_);
          return;
        }

        audio_input_engine_.reset();
        transmit_queue_.reset();
#ifdef ARDUINO_ESP32S3_DEV
        //wake_net_.Start(audio_input_device_);
#endif
        audio_output_engine_ = std::make_shared<AudioOutputEngine>(audio_output_device_, audio_frame_duration_);
        ChangeState(State::kSpeaking);
      } else if (strcmp("stop", state_json->valuestring) == 0) {
        CLOG("tts stop");
        if (audio_output_engine_) {
          audio_output_engine_->NotifyDataEnd([this]() { task_queue_.Enqueue([this]() { OnAudioOutputDataConsumed(); }); });
        }
      } else if (strcmp("sentence_start", state_json->valuestring) == 0) {
        auto text = cJSON_GetObjectItem(root_obj.get(), "text");
        if (text != nullptr) {
          CLOG("<< %s", text->valuestring);
          if (observer_) {
            observer_->PushEvent(Observer::ChatMessageEvent{ChatRole::kAssistant, text->valuestring});
          }
        }
      } else if (strcmp("sentence_end", state_json->valuestring) == 0) {
        // TODO:
      }
    }
  } else if (type == "stt") {
    auto text = cJSON_GetObjectItem(root_obj.get(), "text");
    if (text != nullptr) {
      CLOG(">> %s", text->valuestring);
      if (observer_) {
        observer_->PushEvent(Observer::ChatMessageEvent{ChatRole::kUser, text->valuestring});
      }
    }
  } else if (type == "llm") {
    auto emotion = cJSON_GetObjectItem(root_obj.get(), "emotion");
    if (cJSON_IsString(emotion)) {
      CLOG("emotion: %s", emotion->valuestring);
      if (observer_) {
        observer_->PushEvent(Observer::EmotionEvent{emotion->valuestring});
      }
    }
  } else if (type == "iot") {
    auto commands = cJSON_GetObjectItem(root_obj.get(), "commands");
    if (cJSON_IsArray(commands)) {
      auto count = cJSON_GetArraySize(commands);
      for (size_t i = 0; i < count; ++i) {
        auto *command = cJSON_GetArrayItem(commands, i);
        if (!cJSON_IsObject(command)) {
          continue;
        }

        auto *name_json = cJSON_GetObjectItem(command, "name");
        auto *method_json = cJSON_GetObjectItem(command, "method");
        auto *parameters_json = cJSON_GetObjectItem(command, "parameters");

        if (!cJSON_IsString(name_json) || !cJSON_IsString(method_json) || !cJSON_IsObject(parameters_json)) {
          continue;
        }

        std::string name = name_json->valuestring;
        std::string method = method_json->valuestring;
        std::map<std::string, iot::Value> parameters;
        auto *parameter = parameters_json->child;
        while (parameter) {
          auto *key = parameter->string;
          auto *value = parameter->valuestring;
          if (cJSON_IsString(parameter)) {
            parameters[key] = std::string(value);
          } else if (cJSON_IsNumber(parameter)) {
            parameters[key] = static_cast<int64_t>(parameter->valueint);
          } else if (cJSON_IsBool(parameter)) {
            parameters[key] = static_cast<bool>(parameter->valueint);
          }
          parameter = parameter->next;
        }
        auto iot_message = Observer::IotMessageEvent{name, method, parameters};
        if (observer_) {
          observer_->PushEvent(iot_message);
        }
      }
    }
  } else {
    CLOGE("Unknown JSON type: %s", type.c_str());
  }
}

void EngineImpl::OnWebSocketConnected() {
  CLOGI();
  if (state_ == State::kWebsocketConnecting) {
    ChangeState(State::kWebsocketConnected);
  } else if (state_ == State::kWebsocketConnectingWithWakeup) {
    ChangeState(State::kWebsocketConnectedWithWakeup);
  } else {
    CLOGE("invalid state: %u", state_);
    return;
  }

  std::unique_ptr<cJSON, decltype(&DeleteCjsonObj)> root_obj(cJSON_CreateObject(), &DeleteCjsonObj);
  cJSON_AddStringToObject(root_obj.get(), "type", "hello");
  cJSON_AddNumberToObject(root_obj.get(), "version", 1);
  cJSON_AddStringToObject(root_obj.get(), "transport", "websocket");

  auto const audio_params_obj = cJSON_CreateObject();
  cJSON_AddStringToObject(audio_params_obj, "format", "opus");
  cJSON_AddNumberToObject(audio_params_obj, "sample_rate", 16000);
  cJSON_AddNumberToObject(audio_params_obj, "channels", 1);
  cJSON_AddNumberToObject(audio_params_obj, "frame_duration", audio_frame_duration_);
  cJSON_AddItemToObject(root_obj.get(), "audio_params", audio_params_obj);

  std::unique_ptr<char, decltype(&CJSONFree)> text(cJSON_PrintUnformatted(root_obj.get()), &CJSONFree);
  const auto length = strlen(text.get());
  CLOGI("sending text: %.*s", static_cast<int>(length), text.get());
  esp_websocket_client_send_text(web_socket_client_, text.get(), length, pdMS_TO_TICKS(5000));
}

void EngineImpl::OnWebSocketDisconnected() {
  CLOGI();
  audio_input_engine_.reset();
  transmit_queue_.reset();
  audio_output_engine_.reset();
  esp_websocket_client_close(web_socket_client_, pdMS_TO_TICKS(5000));

#ifdef ARDUINO_ESP32S3_DEV
  //wake_net_.Start(audio_input_device_);
#endif
  ChangeState(State::kStandby);
}

void EngineImpl::OnAudioOutputDataConsumed() {
  CLOGI();
  if (state_ != State::kSpeaking) {
    CLOGD("invalid state: %u", state_);
    return;
  }
  SendIotUpdatedStates(false);
  StartListening();
}

void EngineImpl::OnTriggered() {
  CLOGI();
  if(state_ == State::kInited)
  {
    state_ = State::kStandby;
  }
  switch (state_) {
    case State::kInited: {

      LoadProtocol();

      break;
    }
    case State::kStandby: {
      if (ConnectWebSocket()) {
        ChangeState(State::kWebsocketConnecting);
      }
      break;
    }
    case State::kListening: {
      DisconnectWebSocket();
      break;
    }
    case State::kSpeaking: {
      AbortSpeaking();
      break;
    }
    default: {
      break;
    }
  }
}

void EngineImpl::OnWakeUp() {
  CLOGI();
  switch (state_) {
    case State::kStandby: {
      if (ConnectWebSocket()) {
        ChangeState(State::kWebsocketConnectingWithWakeup);
      }
      break;
    }
    case State::kSpeaking: {
      AbortSpeaking("wake_word_detected");
      break;
    }
    default: {
      break;
    }
  }
}

void EngineImpl::LoadProtocol() {
  CLOGI();
  if (state_ != State::kInited) {
    CLOG("invalid state: %u", state_);
    return;
  }

  ChangeState(State::kLoadingProtocol);

  auto config = GetConfigFromServer(ota_url_, uuid_);

  if (!config.has_value()) {
    CLOGE("GetConfigFromServer failed");
    ChangeState(State::kInited);
    return;
  }

  CLOG("mqtt endpoint: %s", config->mqtt.endpoint.c_str());
  CLOG("mqtt client_id: %s", config->mqtt.client_id.c_str());
  CLOG("mqtt username: %s", config->mqtt.username.c_str());
  CLOG("mqtt password: %s", config->mqtt.password.c_str());
  CLOG("mqtt publish_topic: %s", config->mqtt.publish_topic.c_str());
  CLOG("mqtt subscribe_topic: %s", config->mqtt.subscribe_topic.c_str());

  CLOG("activation code: %s", config->activation.code.c_str());
  CLOG("activation message: %s", config->activation.message.c_str());

  if (!config->activation.code.empty()) {
    if (observer_) {
      observer_->PushEvent(Observer::ActivationEvent{config->activation.code, config->activation.message});
    }
    ChangeState(State::kInited);
    return;
  }
#ifdef ARDUINO_ESP32S3_DEV
  //wake_net_.Start(audio_input_device_);
#endif

  ChangeState(State::kStandby);
  return;
}

void EngineImpl::StartListening() {
  if (state_ != State::kWebsocketConnected && state_ != State::kWebsocketConnectedWithWakeup && state_ != State::kSpeaking) {
    CLOG("invalid state: %u", state_);
    return;
  }

  std::unique_ptr<cJSON, decltype(&DeleteCjsonObj)> root_obj(cJSON_CreateObject(), &DeleteCjsonObj);
  cJSON_AddStringToObject(root_obj.get(), "session_id", session_id_.c_str());
  cJSON_AddStringToObject(root_obj.get(), "type", "listen");
  cJSON_AddStringToObject(root_obj.get(), "state", "start");
  cJSON_AddStringToObject(root_obj.get(), "mode", "auto");
  std::unique_ptr<char, decltype(&CJSONFree)> text(cJSON_PrintUnformatted(root_obj.get()), &CJSONFree);
  const auto length = strlen(text.get());
  CLOGI("sending text: %.*s", static_cast<int>(length), text.get());
  esp_websocket_client_send_text(web_socket_client_, text.get(), length, pdMS_TO_TICKS(5000));

  audio_output_engine_.reset();
#ifdef ARDUINO_ESP32S3_DEV
  //wake_net_.Stop();
#endif
  transmit_queue_ = std::make_unique<TaskQueue>("AiVoxTransmit", 1024 * 3, tskIDLE_PRIORITY + 2);
  audio_input_engine_ = std::make_shared<AudioInputEngine>(
      audio_input_device_,
      [this](FlexArray<uint8_t> &&data) mutable {
        if (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) == 0 && transmit_queue_->Size() > 5) {
          return;
        }
        size_t pending_tasks = transmit_queue_->Size();
        
        if (pending_tasks > 15) {
            return; 
        }
        transmit_queue_->Enqueue([this, data = std::move(data)]() mutable {
          if (esp_websocket_client_is_connected(web_socket_client_)) {
            const auto start_time = esp_timer_get_time();
            if (data.size() !=
                esp_websocket_client_send_bin(web_socket_client_, reinterpret_cast<const char *>(data.data()), data.size(), pdMS_TO_TICKS(1000))) {
              CLOGE("sending failed");
            }

            const auto elapsed_time = esp_timer_get_time() - start_time;
            if (elapsed_time > 100 * 1000) {
              CLOGW("Network latency high: %lld ms, data size: %zu bytes, poor network condition detected", elapsed_time / 1000, data.size());
            }
          }
        });
      },
      audio_frame_duration_);
  ChangeState(State::kListening);
}

void EngineImpl::AbortSpeaking() {
  if (state_ != State::kSpeaking) {
    CLOGE("invalid state: %d", state_);
    return;
  }

  std::unique_ptr<cJSON, decltype(&DeleteCjsonObj)> root_obj(cJSON_CreateObject(), &DeleteCjsonObj);
  cJSON_AddStringToObject(root_obj.get(), "session_id", session_id_.c_str());
  cJSON_AddStringToObject(root_obj.get(), "type", "abort");
  std::unique_ptr<char, decltype(&CJSONFree)> text(cJSON_PrintUnformatted(root_obj.get()), &CJSONFree);
  const auto length = strlen(text.get());
  CLOGI("sending text: %.*s", static_cast<int>(length), text.get());
  esp_websocket_client_send_text(web_socket_client_, text.get(), length, pdMS_TO_TICKS(5000));
  CLOG("OK");
}

void EngineImpl::AbortSpeaking(const std::string &reason) {
  if (state_ != State::kSpeaking) {
    CLOGE("invalid state: %d", state_);
    return;
  }

  std::unique_ptr<cJSON, decltype(&DeleteCjsonObj)> root_obj(cJSON_CreateObject(), &DeleteCjsonObj);
  cJSON_AddStringToObject(root_obj.get(), "session_id", session_id_.c_str());
  cJSON_AddStringToObject(root_obj.get(), "type", "abort");
  cJSON_AddStringToObject(root_obj.get(), "reason", reason.c_str());
  std::unique_ptr<char, decltype(&CJSONFree)> text(cJSON_PrintUnformatted(root_obj.get()), &CJSONFree);
  const auto length = strlen(text.get());
  CLOGI("sending text: %.*s", static_cast<int>(length), text.get());
  esp_websocket_client_send_text(web_socket_client_, text.get(), length, pdMS_TO_TICKS(5000));
}

bool EngineImpl::ConnectWebSocket() {
  if (state_ != State::kStandby) {
    CLOGE("invalid state: %u", state_);
    return false;
  }

  CLOGI("esp_websocket_client_start");
  const auto ret = esp_websocket_client_start(web_socket_client_);
  CLOGI("websocket client start: %d", ret);
  return ret == ESP_OK;
}

void EngineImpl::DisconnectWebSocket() {
  audio_input_engine_.reset();
  transmit_queue_.reset();
  audio_output_engine_.reset();
#ifdef ARDUINO_ESP32S3_DEV
  //wake_net_.Start(audio_input_device_);
#endif
  esp_websocket_client_close(web_socket_client_, pdMS_TO_TICKS(5000));
}

void EngineImpl::SendIotDescriptions() {
  const auto descirptions = iot_manager_.DescriptionsJson();
  for (const auto &descirption : descirptions) {
    CLOGI("sending text: %.*s", static_cast<int>(descirption.size()), descirption.c_str());
    const auto ret = esp_websocket_client_send_text(web_socket_client_, descirption.c_str(), descirption.size(), pdMS_TO_TICKS(5000));
    if (ret != descirption.size()) {
      CLOGE("sending failed");
    } else {
      CLOGD("sending ok");
    }
  }
}

void EngineImpl::SendIotUpdatedStates(const bool force) {
  CLOGD("force: %d", force);
  const auto updated_states = iot_manager_.UpdatedJson(force);
  for (const auto &updated_state : updated_states) {
    CLOGI("sending text: %.*s", static_cast<int>(updated_state.size()), updated_state.c_str());
    const auto ret = esp_websocket_client_send_text(web_socket_client_, updated_state.c_str(), updated_state.size(), pdMS_TO_TICKS(5000));
    if (ret != updated_state.size()) {
      CLOGE("sending failed");
    } else {
      CLOGD("sending ok");
    }
  }
}

void EngineImpl::ChangeState(const State new_state) {
  auto convert_state = [](const State state) {
    switch (state) {
      case State::kIdle:
        return ChatState::kIdle;
      case State::kInited:
        return ChatState::kIniting;
      case State::kLoadingProtocol:
        return ChatState::kIniting;
      case State::kWebsocketConnecting:
      case State::kWebsocketConnectingWithWakeup:
        return ChatState::kConnecting;
      case State::kWebsocketConnectedWithWakeup:
      case State::kWebsocketConnected:
        return ChatState::kConnecting;
      case State::kStandby:
        return ChatState::kStandby;
      case State::kListening:
        return ChatState::kListening;
      case State::kSpeaking:
        return ChatState::kSpeaking;
      default:
        return ChatState::kIdle;
    }
  };

  const auto new_chat_state = convert_state(new_state);

  if (new_chat_state != chat_state_ && observer_) {
    observer_->PushEvent(Observer::StateChangedEvent{chat_state_, new_chat_state});
  }

  state_ = new_state;
  chat_state_ = new_chat_state;
}

void EngineImpl::Touch() {
  task_queue_.Enqueue([this]() { OnTriggered(); });
}

void EngineImpl::Stop() {
    CLOGD("Stopping EngineImpl...");
    std::lock_guard lock(mutex_);

    if (state_ == State::kIdle) {
        CLOGI("Engine is already stopped");
        return;
    }

    if (web_socket_client_ != nullptr && state_ != State::kStandby) {
      esp_websocket_client_close(web_socket_client_, pdMS_TO_TICKS(5000)); 
    }


    CLOGI("EngineImpl stopped");

}

void EngineImpl::change_state() {
    std::lock_guard lock(mutex_);
    switch (state_) {
      case State::kWebsocketConnecting:
        //DisconnectWebSocket();
        break;

      case State::kListening:
        //DisconnectWebSocket();
        break;

      case State::kSpeaking:
        AbortSpeaking();
        //DisconnectWebSocket();
        break;

      case State::kInited:
        break;

      case State::kStandby:
        return;

      default:
        break;
    }
    ChangeState(State::kIdle);
}


void EngineImpl::CloseInputDevice() {
    if (audio_input_device_) {
        audio_input_device_->Disable();
    }
}

void EngineImpl::OpenInputDevice()
{
    if (audio_input_device_) {
        audio_input_device_->Enable();
    } 
}

void EngineImpl::DisableconnectWebSocket()
{
    std::lock_guard lock(mutex_); 
    ChangeState(State::kStandby);
    vTaskDelay(200);
    if (web_socket_client_ != nullptr) {
          esp_websocket_client_stop(web_socket_client_);
          vTaskDelay(pdMS_TO_TICKS(300));
          esp_websocket_client_destroy(web_socket_client_);
          web_socket_client_ = nullptr;
          vTaskDelay(pdMS_TO_TICKS(700));
      }
       
      audio_input_engine_.reset();
      transmit_queue_.reset();
      audio_output_engine_.reset();
      
}


}  