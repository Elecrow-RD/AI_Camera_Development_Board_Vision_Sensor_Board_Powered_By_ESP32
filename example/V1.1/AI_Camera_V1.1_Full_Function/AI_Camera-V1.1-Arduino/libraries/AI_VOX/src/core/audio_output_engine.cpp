#include "audio_output_engine.h"

#include <algorithm>

#ifndef CLOGGER_SEVERITY
#define CLOGGER_SEVERITY CLOGGER_SEVERITY_WARN
#endif
#include "clogger/clogger.h"

#ifdef ARDUINO
#include "libopus/opus.h"
#else
#include "opus.h"
#endif

namespace {
constexpr uint32_t kDefaultSampleRate = 24000;
constexpr uint32_t kDefaultChannels = 1;
constexpr uint32_t kDefaultDurationMs = 40;  // Duration in milliseconds
constexpr uint32_t kDefaultFrameSize = kDefaultSampleRate / 1000 * kDefaultChannels * kDefaultDurationMs;
constexpr uint32_t kPrebufferDurationMs = 240;
constexpr uint32_t kOutputDrainDurationMs = 180;
}  // namespace

AudioOutputEngine::AudioOutputEngine(std::shared_ptr<ai_vox::AudioOutputDevice> audio_output_device, const uint32_t frame_duration)
    : audio_output_device_(std::move(audio_output_device)),
      samples_(kDefaultSampleRate / 1000 * kDefaultChannels * frame_duration),
      prebuffer_frames_(std::max<size_t>(2, (kPrebufferDurationMs + frame_duration - 1) / frame_duration)) {
  int error = -1;
  opus_decoder_ = opus_decoder_create(kDefaultSampleRate, kDefaultChannels, &error);
  assert(opus_decoder_ != nullptr);
  audio_output_device_->Open(kDefaultSampleRate);
  pcm_buffer_.resize(samples_);

  uint32_t stack_size = 9 << 10;
  prebuffer_.reserve(prebuffer_frames_);

  // Decoding and I2S feeding must pre-empt UI, observer and background work.
  // The task blocks while DMA is full, so the higher priority does not spin.
  task_queue_ = new TaskQueue("AudioOutput", stack_size, tskIDLE_PRIORITY + 7);
  CLOGI("OK");
}

AudioOutputEngine::~AudioOutputEngine() {
  delete task_queue_;
  audio_output_device_->Close();
  opus_decoder_destroy(opus_decoder_);
}

void AudioOutputEngine::Write(FlexArray<uint8_t>&& data) {
  std::lock_guard<std::mutex> lock(prebuffer_mutex_);
  if (!playback_started_) {
    prebuffer_.emplace_back(std::move(data));
    if (prebuffer_.size() < prebuffer_frames_) {
      return;
    }
    playback_started_ = true;
    FlushPrebufferLocked();
    return;
  }

  task_queue_->Enqueue([this, data = std::move(data)]() mutable { ProcessData(std::move(data)); });
}

void AudioOutputEngine::NotifyDataEnd(std::function<void()>&& callback) {
  std::lock_guard<std::mutex> lock(prebuffer_mutex_);
  // Short replies may finish before reaching the normal prebuffer threshold.
  // Flush them before the completion callback so no speech is discarded.
  FlushPrebufferLocked();
  playback_started_ = true;
  task_queue_->Enqueue([callback = std::move(callback)]() mutable {
    // i2s_channel_write returns when PCM has been copied into DMA, not when
    // the speaker has played the final sample. Let the enlarged DMA queue
    // drain before the engine closes the I2S channel and resumes listening.
    vTaskDelay(pdMS_TO_TICKS(kOutputDrainDurationMs));
    callback();
  });
}

void AudioOutputEngine::FlushPrebufferLocked() {
  for (auto& frame : prebuffer_) {
    task_queue_->Enqueue([this, data = std::move(frame)]() mutable { ProcessData(std::move(data)); });
  }
  prebuffer_.clear();
}

void AudioOutputEngine::ProcessData(FlexArray<uint8_t>&& data) {
  const auto decoded_samples = opus_decode(opus_decoder_, data.data(), data.size(),
                                           pcm_buffer_.data(), samples_, 0);
  if (decoded_samples > 0) {
    // opus_decode returns the actual number of valid samples. Writing the
    // whole maximum-sized buffer played uninitialised tail data whenever the
    // server supplied a shorter frame, which sounded like intermittent skips.
    audio_output_device_->Write(pcm_buffer_.data(), static_cast<size_t>(decoded_samples));
  }
}
// #include "audio_output_engine.h"

// #ifndef CLOGGER_SEVERITY
// #define CLOGGER_SEVERITY CLOGGER_SEVERITY_WARN
// #endif
// #include "clogger/clogger.h"

// #ifdef ARDUINO
// #include "libopus/opus.h"
// #else
// #include "opus.h"
// #endif

// namespace {
// constexpr uint32_t kDefaultSampleRate = 24000;
// constexpr uint32_t kDefaultChannels = 1;
// constexpr uint32_t kDefaultDurationMs = 60;  // Duration in milliseconds
// constexpr uint32_t kDefaultFrameSize = kDefaultSampleRate / 1000 * kDefaultChannels * kDefaultDurationMs; // 480 samples
// }  // namespace

// AudioOutputEngine::AudioOutputEngine(std::shared_ptr<ai_vox::AudioOutputDevice> audio_output_device, const uint32_t frame_duration)
//     : audio_output_device_(std::move(audio_output_device)), samples_(kDefaultSampleRate / 1000 * kDefaultChannels * frame_duration) {
//   int error = -1;
//   opus_decoder_ = opus_decoder_create(kDefaultSampleRate, kDefaultChannels, &error);
//   assert(opus_decoder_ != nullptr);
//   audio_output_device_->Open(kDefaultSampleRate);

//   uint32_t stack_size = 16 << 10;
//   task_queue_ = new TaskQueue("AudioOutput", stack_size, tskIDLE_PRIORITY + 1);
//   CLOGI("OK");
// }

// AudioOutputEngine::~AudioOutputEngine() {
//   delete task_queue_;
//   audio_output_device_->Close();
//   opus_decoder_destroy(opus_decoder_);
// }

// void AudioOutputEngine::Write(FlexArray<uint8_t>&& data) {
//   // std::lock_guard<std::mutex> lock(buffer_mutex_);

//   // 将数据添加到缓冲区
//   buffered_data_.push_back(std::move(data));
  
//   // 累计样本数（假设每帧都是标准的20ms帧，即480个样本）
//   total_samples_cached_ += kDefaultFrameSize;

//   // 如果累计数据达到1秒，则刷新缓冲区开始播放
//   if (!first_play_started_) {
//     // 第一次播放逻辑：等待累计样本达到阈值
//     if (total_samples_cached_ >= kMinRequiredSamples) {
//       FlushBufferedData();
//       first_play_started_ = true;  // 标记第一次已经播放过
//     }
//   } else {
//     // 后续数据：直接播放，不等待
//     auto& last_data = buffered_data_.back();
//     task_queue_->Enqueue([this, d = std::move(last_data)]() mutable {
//       ProcessData(std::move(d));
//     });
//     buffered_data_.pop_back();
//   }
// }

// void AudioOutputEngine::NotifyDataEnd(std::function<void()>&& callback) {
//   first_play_started_ = false;
//   task_queue_->Enqueue(std::move(callback));
// }

// void AudioOutputEngine::ProcessData(FlexArray<uint8_t>&& data) {
//   auto pcm = new int16_t[samples_];
//   const auto ret = opus_decode(opus_decoder_, data.data(), data.size(), pcm, samples_, 0);
//   if (ret >= 0) {
//     audio_output_device_->Write(pcm, samples_);
//   }
//   delete[] pcm;
// }

// // 新增：刷新并播放所有缓存的数据
// void AudioOutputEngine::FlushBufferedData() {
//   // std::lock_guard<std::mutex> lock(buffer_mutex_);

//   // 将所有缓存的数据包依次加入任务队列进行处理
//   for (auto& data : buffered_data_) {
//     task_queue_->Enqueue([this, d = std::move(data)]() mutable {
//       ProcessData(std::move(d));
//     });
//   }

//   // 清空缓冲区并重置计数器
//   buffered_data_.clear();
//   total_samples_cached_ = 0;
// }
