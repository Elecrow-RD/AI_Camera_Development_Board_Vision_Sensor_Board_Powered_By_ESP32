#include "i2s_std_audio_output_device.h"

#include <cmath>

namespace ai_vox {
I2sStdAudioOutputDevice::I2sStdAudioOutputDevice(gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout)
    : gpio_cfg_({
          .mclk = I2S_GPIO_UNUSED,
          .bclk = bclk,
          .ws = ws,
          .dout = dout,
          .din = I2S_GPIO_UNUSED,
          .invert_flags =
              {
                  .mclk_inv = false,
                  .bclk_inv = false,
                  .ws_inv = false,
              },
      }) {
}

I2sStdAudioOutputDevice::I2sStdAudioOutputDevice(const i2s_std_slot_config_t& slot_cfg, const i2s_std_gpio_config_t& gpio_cfg)
    : slot_cfg_(slot_cfg), gpio_cfg_(gpio_cfg) {
}

bool I2sStdAudioOutputDevice::Open(uint32_t sample_rate) {
  std::lock_guard<std::mutex> lock(io_mutex_);
  CloseUnlocked();

  i2s_chan_config_t tx_chan_cfg = {
      .id = I2S_NUM_1,
      .role = I2S_ROLE_MASTER,
      // Keep about 160 ms of PCM queued at 24 kHz. Together with the Opus
      // prebuffer this absorbs normal Wi-Fi and task-scheduling jitter.
      .dma_desc_num = 8,
      .dma_frame_num = 480,
      .auto_clear_after_cb = true,
      .auto_clear_before_cb = false,
      .allow_pd = false,
      .intr_priority = 0,
  };

  esp_err_t result = i2s_new_channel(&tx_chan_cfg, &i2s_tx_handle_, nullptr);
  if (result != ESP_OK) {
    i2s_tx_handle_ = nullptr;
    return false;
  }

  i2s_std_config_t tx_std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
      .slot_cfg = slot_cfg_,
      .gpio_cfg = gpio_cfg_,
  };

  result = i2s_channel_init_std_mode(i2s_tx_handle_, &tx_std_cfg);
  if (result != ESP_OK) {
    CloseUnlocked();
    return false;
  }
  result = i2s_channel_enable(i2s_tx_handle_);
  if (result != ESP_OK) {
    CloseUnlocked();
    return false;
  }
  return true;
}

void I2sStdAudioOutputDevice::Close() {
  std::lock_guard<std::mutex> lock(io_mutex_);
  CloseUnlocked();
}

void I2sStdAudioOutputDevice::CloseUnlocked() {
  if (i2s_tx_handle_ == nullptr) {
    return;
  }

  i2s_channel_disable(i2s_tx_handle_);
  i2s_del_channel(i2s_tx_handle_);
  i2s_tx_handle_ = nullptr;
}

I2sStdAudioOutputDevice::~I2sStdAudioOutputDevice() {
  Close();
}

uint16_t I2sStdAudioOutputDevice::volume() const {
  return volume_.load(std::memory_order_relaxed);
}

void I2sStdAudioOutputDevice::SetVolume(uint16_t volume) {
  if (volume > kMaxVolume) {
    volume = kMaxVolume;
  }
  volume_.store(volume, std::memory_order_relaxed);
  volume_factor_.store(
      static_cast<int32_t>(pow(double(volume) / 100.0, 2) * 65536),
      std::memory_order_relaxed);
}

size_t I2sStdAudioOutputDevice::Write(int16_t* pcm, size_t samples) {
  if (!pcm || samples == 0) {
    return 0;
  }
  std::lock_guard<std::mutex> lock(io_mutex_);
  if (i2s_tx_handle_ == nullptr) {
    return 0;
  }

  const double gain = volume_factor_.load(std::memory_order_relaxed) * 1.5;
  output_buffer_.resize(samples);

  for (size_t i = 0; i < samples; i++) {
    int64_t temp = int64_t(pcm[i]) * gain;
    if (temp > INT32_MAX) {
      output_buffer_[i] = INT32_MAX;
    } else if (temp < INT32_MIN) {
      output_buffer_[i] = INT32_MIN;
    } else {
      output_buffer_[i] = static_cast<int32_t>(temp);
    }
  }

  size_t bytes_written = 0;
  const esp_err_t result = i2s_channel_write(i2s_tx_handle_, output_buffer_.data(),
                                              output_buffer_.size() * sizeof(int32_t),
                                              &bytes_written, 1000);
  if (result != ESP_OK) {
    return 0;
  }
  return bytes_written / sizeof(int32_t);
}

}  // namespace ai_vox
