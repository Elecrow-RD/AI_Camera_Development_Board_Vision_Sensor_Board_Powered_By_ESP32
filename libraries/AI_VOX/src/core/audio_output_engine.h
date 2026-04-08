#pragma once

#include <driver/i2s_std.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <thread>
#include <vector>

#include "../audio_output_device.h"
#include "flex_array/flex_array.h"
#include "task_queue/task_queue.h"

class OpusDecoder;
class AudioOutputEngine {
 public:
  AudioOutputEngine(std::shared_ptr<ai_vox::AudioOutputDevice> audio_output_device, const uint32_t frame_duration);
  ~AudioOutputEngine();

  void Write(FlexArray<uint8_t>&& data);
  void NotifyDataEnd(std::function<void()>&& callback);

 private:
  static void Loop(void* self);
  void Loop();
  void ProcessData(FlexArray<uint8_t>&& data);

  std::shared_ptr<ai_vox::AudioOutputDevice> audio_output_device_;
  struct OpusDecoder* opus_decoder_ = nullptr;
  TaskQueue* task_queue_ = nullptr;
  const uint32_t samples_ = 0;
};
// #pragma once

// #include <driver/i2s_std.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>

// #include <condition_variable>
// #include <functional>
// #include <list>
// #include <memory>
// #include <thread>
// #include <vector>
// #include <mutex>

// #include "../audio_output_device.h"
// #include "flex_array/flex_array.h"
// #include "task_queue/task_queue.h"

// class OpusDecoder;
// class AudioOutputEngine {
//  public:
//   AudioOutputEngine(std::shared_ptr<ai_vox::AudioOutputDevice> audio_output_device, const uint32_t frame_duration);
//   ~AudioOutputEngine();

//   void Write(FlexArray<uint8_t>&& data);
//   void NotifyDataEnd(std::function<void()>&& callback);

//  private:
//   static void Loop(void* self);
//   void Loop();
//   void ProcessData(FlexArray<uint8_t>&& data);
//   void FlushBufferedData();  // 新增：刷新缓存数据

//   std::shared_ptr<ai_vox::AudioOutputDevice> audio_output_device_;
//   struct OpusDecoder* opus_decoder_ = nullptr;
//   TaskQueue* task_queue_ = nullptr;
//   const uint32_t samples_ = 0;

//   // 新增的缓冲相关成员
//   std::vector<FlexArray<uint8_t>> buffered_data_;      // 缓存数据包s
//   size_t total_samples_cached_ = 0;                    // 累积样本数
//   static constexpr size_t kMinRequiredSamples = 6000; // 1秒数据量 (24kHz * 1s)
//   std::mutex buffer_mutex_;                            // 保护缓冲区的互斥锁
//   bool first_play_started_ = false;                    // 标记是否已经开始第一次播放
  
// };


