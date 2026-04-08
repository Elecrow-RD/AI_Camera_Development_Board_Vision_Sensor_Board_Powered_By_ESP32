#include "audio_input_engine.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
constexpr size_t kMaxOpusPacketSize = 1500;
constexpr uint32_t kFrameDuration = 20;                          // ms
constexpr uint32_t kDefaultSampleRate = 16000;                   // Hz
constexpr uint32_t kDefaultChannels = 1;                         // Mono
constexpr size_t kMaxFrameSize = 16000 / 1000 * kFrameDuration;  // 16000 Hz * 20 ms
}  // namespace

AudioInputEngine::AudioInputEngine(std::shared_ptr<ai_vox::AudioInputDevice> audio_input_device,
                                   AudioInputEngine::DataHandler &&handler,
                                   const uint32_t frame_duration)
    : handler_(std::move(handler)), audio_input_device_(std::move(audio_input_device)) 
{
  int error = 0;
  opus_encoder_ = opus_encoder_create(kDefaultSampleRate, kDefaultChannels, OPUS_APPLICATION_VOIP, &error);
  assert(opus_encoder_ != nullptr);
  if (opus_encoder_ == nullptr) {
    CLOG("opus_encoder_create failed: %d", error);
    abort();
    return;
  }

  uint32_t stack_size = 32 << 10;
  opus_encoder_ctl(opus_encoder_, OPUS_SET_DTX(1));
  if (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) == 0) {
    opus_encoder_ctl(opus_encoder_, OPUS_SET_COMPLEXITY(0));
    opus_encoder_ctl(opus_encoder_, OPUS_SET_BITRATE(8000));
    stack_size = 20 << 10;
  } else {
    opus_encoder_ctl(opus_encoder_, OPUS_SET_COMPLEXITY(5));
  }

  audio_input_device_->Open(kDefaultSampleRate);
  task_queue_ = new TaskQueue("AudioInput", stack_size, tskIDLE_PRIORITY + 1);
  task_queue_->Enqueue([this, samples = 16000 / 1000 * frame_duration]() { PullData(samples); });
  CLOGI("OK");
}

AudioInputEngine::~AudioInputEngine() {
  delete task_queue_;
  audio_input_device_->Close();
  opus_encoder_destroy(opus_encoder_);
  CLOG("OK");
}

void AudioInputEngine::PullData(const uint32_t samples) {
  auto pcm = new int16_t[samples];
  audio_input_device_->Read(pcm, samples);

  FlexArray<uint8_t> data(kMaxOpusPacketSize);
  const auto ret = opus_encode(opus_encoder_, pcm, samples, data.data(), data.size());
  if (ret > 0) {
    data.Resize(ret);
    handler_(std::move(data));
  } else {
    CLOGE("opus_encode failed with: %d", ret);
  }
  delete[] pcm;

  task_queue_->Enqueue([this, samples]() { PullData(samples); });
}