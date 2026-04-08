#pragma once

#ifndef _AI_VOX_ENGINE_IMPL_H_
#define _AI_VOX_ENGINE_IMPL_H_

#include <esp_event_base.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <condition_variable>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "ai_vox_engine.h"
#include "espressif_esp_websocket_client/esp_websocket_client.h"
#include "flex_array/flex_array.h"
#include "iot/iot_manager.h"
#include "task_queue/task_queue.h"
#include "wake_net/wake_net.h"

struct button_dev_t;
class AudioInputEngine;
class AudioOutputEngine;

namespace ai_vox {

class EngineImpl : public Engine {
 public:
  static EngineImpl &GetInstance();
  EngineImpl();
  ~EngineImpl();
  void SetObserver(std::shared_ptr<Observer> observer) override;
  void SetTrigger(const gpio_num_t gpio) override;
  void SetOtaUrl(const std::string url) override;
  void ConfigWebsocket(const std::string url, const std::map<std::string, std::string> headers) override;
  void RegisterIotEntity(std::shared_ptr<iot::Entity> entity) override;
  void Start(std::shared_ptr<AudioInputDevice> audio_input_device, std::shared_ptr<AudioOutputDevice> audio_output_device) override;
  void Stop() override;
  void CloseInputDevice() override;
  void OpenInputDevice() override;
  void Touch();
  void change_state() override;
  void DisableconnectWebSocket() override;
 private:
  enum class State {
    kIdle,
    kInited,
    kLoadingProtocol,
    kWebsocketConnecting,
    kWebsocketConnectingWithWakeup,
    kWebsocketConnected,
    kWebsocketConnectedWithWakeup,
    kStandby,
    kListening,
    kSpeaking,
  };

  static void OnButtonClick(void *button_handle, void *usr_data);
  static void OnWebsocketEvent(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

  void OnButtonClick();
  void OnWebsocketEvent(esp_event_base_t base, int32_t event_id, void *event_data);
  void OnAudioFrame(FlexArray<uint8_t> &&data);
  void OnJsonData(FlexArray<uint8_t> &&data);
  void OnWebSocketConnected();
  void OnWebSocketDisconnected();
  void OnAudioOutputDataConsumed();
  void OnTriggered();
  void OnWakeUp();

  void LoadProtocol();
  void StartListening();
  void AbortSpeaking();
  void AbortSpeaking(const std::string &reason);
  bool ConnectWebSocket();
  void DisconnectWebSocket();
  void SendIotDescriptions();
  void SendIotUpdatedStates(const bool force);
  void ChangeState(const State new_state);

  mutable std::mutex mutex_;
  State state_ = State::kIdle;
  ChatState chat_state_ = ChatState::kIdle;
  button_dev_t *button_handle_ = nullptr;
  gpio_num_t trigger_pin_ = GPIO_NUM_0;
  std::shared_ptr<AudioInputDevice> audio_input_device_;
  std::shared_ptr<AudioOutputDevice> audio_output_device_;
  std::shared_ptr<Observer> observer_;
  ai_vox::iot::Manager iot_manager_;
  esp_websocket_client_handle_t web_socket_client_ = nullptr;
  std::string uuid_;
  std::string session_id_;
  std::shared_ptr<AudioInputEngine> audio_input_engine_;
  std::shared_ptr<AudioOutputEngine> audio_output_engine_;
  std::string ota_url_;
  std::string websocket_url_;
  std::map<std::string, std::string> websocket_headers_;
#ifdef ARDUINO_ESP32S3_DEV
  //WakeNet wake_net_;
#endif
  TaskQueue task_queue_;
  std::unique_ptr<TaskQueue> transmit_queue_;
  const uint32_t audio_frame_duration_ = 60;
};
}  // namespace ai_vox

#endif