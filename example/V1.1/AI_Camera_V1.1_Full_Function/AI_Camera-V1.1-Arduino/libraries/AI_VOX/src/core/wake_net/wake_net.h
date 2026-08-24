#pragma once

#ifndef _WAKE_NET_H_
#define _WAKE_NET_H_

#include <functional>
#include <memory>

#include "../task_queue/task_queue.h"
#include "audio_input_device.h"

struct esp_afe_sr_data_t;

class WakeNet {
 public:
  WakeNet(std::function<void()>&& handler);
  ~WakeNet();

  void Start(std::shared_ptr<ai_vox::AudioInputDevice> audio_input_device);
  void Stop();

 private:
  void FeedData(std::shared_ptr<ai_vox::AudioInputDevice>&& audio_input_device, const uint32_t afe_chunksize, const uint32_t channels);
  void DetectWakeWord();

  std::function<void()> handler_;
  TaskQueue* detect_task_ = nullptr;
  TaskQueue* feed_task_ = nullptr;
  esp_afe_sr_data_t* afe_data_;
};

#endif  // _WAKE_NET_H_