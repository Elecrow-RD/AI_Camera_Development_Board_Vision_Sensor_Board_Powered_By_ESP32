#ifdef ARDUINO_ESP32S3_DEV

#include "wake_net.h"

#include <esp_afe_config.h>
#include <esp_afe_sr_models.h>
#include <esp_wn_models.h>
#include <model_path.h>

#include <cstring>

#ifndef CLOGGER_SEVERITY
#define CLOGGER_SEVERITY CLOGGER_SEVERITY_WARN
#endif

#include "core/clogger/clogger.h"

namespace {
auto &g_afe_handle = ESP_AFE_SR_HANDLE;

constexpr uint8_t kSrmodels[] = {
#include "srmodels.bin"
};
}  // namespace

WakeNet::WakeNet(std::function<void()> &&handler) : handler_(std::move(handler)) {
  CLOGI("OK");
  srmodel_list_t *models = srmodel_load(kSrmodels);
  if (models) {
    for (int i = 0; i < models->num; i++) {
      if (strstr(models->model_name[i], ESP_WN_PREFIX) != NULL) {
        CLOGI("wakenet model in flash: %s", models->model_name[i]);
      }
    }
  }

  afe_config_t afe_config = AFE_CONFIG_DEFAULT();
  CLOGI("esp_srmodel_filter");
  afe_config.wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, nullptr);
  afe_config.aec_init = false;
  afe_config.pcm_config.total_ch_num = 1;
  afe_config.pcm_config.mic_num = 1;
  afe_config.pcm_config.ref_num = 0;
  CLOGI("load wakenet '%s'", afe_config.wakenet_model_name);

  afe_data_ = g_afe_handle.create_from_config(&afe_config);
  CLOGI("afe_data: %p", afe_data_);
}

WakeNet::~WakeNet() {
  CLOGI("OK");
}

void WakeNet::Start(std::shared_ptr<ai_vox::AudioInputDevice> audio_input_device) {
  CLOGI("audio_input_device: %p", audio_input_device.get());
  audio_input_device->Open(16000);
  feed_task_ = new TaskQueue("WakeNetFeed", 8 * 1024, tskIDLE_PRIORITY + 1);
  detect_task_ = new TaskQueue("WakeNetDetect", 4 * 1024, tskIDLE_PRIORITY + 1);

  feed_task_->Enqueue(
      [this,
       audio_input_device = std::move(audio_input_device),
       afe_chunksize = g_afe_handle.get_feed_chunksize(afe_data_),
       channels = g_afe_handle.get_total_channel_num(afe_data_)]() mutable { FeedData(std::move(audio_input_device), afe_chunksize, channels); });
  detect_task_->Enqueue([this]() { DetectWakeWord(); });
  CLOGI("OK");
}

void WakeNet::Stop() {
  if (detect_task_) {
    delete detect_task_;
    detect_task_ = nullptr;
  }
  if (feed_task_) {
    delete feed_task_;
    feed_task_ = nullptr;
  }
  CLOGI("OK");
}

void WakeNet::FeedData(std::shared_ptr<ai_vox::AudioInputDevice> &&audio_input_device, const uint32_t afe_chunksize, const uint32_t channels) {
  auto pcm = new int16_t[afe_chunksize * channels];
  audio_input_device->Read(pcm, afe_chunksize * channels);
  g_afe_handle.feed(afe_data_, pcm);
  delete[] pcm;

  feed_task_->Enqueue([this, audio_input_device = std::move(audio_input_device), afe_chunksize, channels]() mutable {
    FeedData(std::move(audio_input_device), afe_chunksize, channels);
  });
}

void WakeNet::DetectWakeWord() {
  afe_fetch_result_t *res = g_afe_handle.fetch(afe_data_);
  if (res != nullptr && res->wakeup_state == WAKENET_DETECTED) {
    CLOGI("Wake word detected");
    if (handler_) {
      handler_();
    }
  }
  taskYIELD();
  detect_task_->Enqueue([this]() { DetectWakeWord(); });
}

#endif  // ARDUINO_ESP32S3_DEV