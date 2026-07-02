#pragma once

#ifndef _AI_VOX_ENGINE_H_
#define _AI_VOX_ENGINE_H_

#include <driver/gpio.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "ai_vox_observer.h"
#include "audio_input_device.h"
#include "audio_output_device.h"
#include "iot_entity.h"

namespace ai_vox {

class Engine {
 public:
  static Engine& GetInstance();
  Engine() = default;
  virtual ~Engine() = default;
  virtual void SetObserver(std::shared_ptr<Observer> observer) = 0;
  virtual void SetTrigger(const gpio_num_t gpio) = 0;
  virtual void SetOtaUrl(const std::string url) = 0;
  virtual void ConfigWebsocket(const std::string url, const std::map<std::string, std::string> headers) = 0;
  virtual void RegisterIotEntity(std::shared_ptr<iot::Entity> entity) = 0;
  virtual void Start(std::shared_ptr<AudioInputDevice> audio_input_device, std::shared_ptr<AudioOutputDevice> audio_output_device) = 0;
  virtual void Stop() = 0;
  virtual void CloseInputDevice() = 0;
  virtual void OpenInputDevice() = 0;
  virtual void Touch() = 0;
  virtual void change_state() = 0;
  virtual void DisableconnectWebSocket() = 0;
 private:
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
};

}  // namespace ai_vox

#endif