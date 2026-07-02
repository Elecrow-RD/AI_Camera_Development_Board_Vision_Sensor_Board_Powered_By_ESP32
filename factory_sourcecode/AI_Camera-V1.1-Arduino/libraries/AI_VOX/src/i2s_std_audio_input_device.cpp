#include "i2s_std_audio_input_device.h"

namespace ai_vox {
I2sStdAudioInputDevice::I2sStdAudioInputDevice(gpio_num_t bclk, gpio_num_t ws, gpio_num_t din)
    : gpio_cfg_({
          .mclk = I2S_GPIO_UNUSED,
          .bclk = bclk,
          .ws = ws,
          .dout = I2S_GPIO_UNUSED,
          .din = din,
          .invert_flags =
              {
                  .mclk_inv = false,
                  .bclk_inv = false,
                  .ws_inv = false,
              },
      }) {
}

I2sStdAudioInputDevice::I2sStdAudioInputDevice(const i2s_std_slot_config_t& slot_cfg, const i2s_std_gpio_config_t& gpio_cfg)
    : slot_cfg_(slot_cfg), gpio_cfg_(gpio_cfg) {
}

bool I2sStdAudioInputDevice::Open(uint32_t sample_rate) {
  Close();

  i2s_chan_config_t rx_chan_cfg = {
      .id = I2S_NUM_0,
      .role = I2S_ROLE_MASTER,
      .dma_desc_num = 2,
      .dma_frame_num = 320,
      .auto_clear_after_cb = true,
      .auto_clear_before_cb = false,
      .allow_pd = false,
      .intr_priority = 0,
  };

  ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, nullptr, &i2s_rx_handle_));

  i2s_std_config_t rx_std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
      .slot_cfg = slot_cfg_,
      .gpio_cfg = gpio_cfg_,
  };

  ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_rx_handle_, &rx_std_cfg));
  ESP_ERROR_CHECK(i2s_channel_enable(i2s_rx_handle_));
  return true;
}

void I2sStdAudioInputDevice::Close() {
  if (i2s_rx_handle_ == nullptr) {
    return;
  }

  i2s_channel_disable(i2s_rx_handle_);
  i2s_del_channel(i2s_rx_handle_);
  i2s_rx_handle_ = nullptr;
}

I2sStdAudioInputDevice::~I2sStdAudioInputDevice() {
  Close();
}

size_t I2sStdAudioInputDevice::Read(int16_t* buffer, uint32_t samples) {
  auto raw_32bit_samples = new int32_t[samples];
  size_t bytes_read = 0;
  i2s_channel_read(i2s_rx_handle_, raw_32bit_samples, samples * sizeof(raw_32bit_samples[0]), &bytes_read, 1000);

  for (int i = 0; i < samples; i++) {
    int32_t value = raw_32bit_samples[i] >> 12;
    buffer[i] = (value > INT16_MAX) ? INT16_MAX : (value < -INT16_MAX) ? -INT16_MAX : (int16_t)value;
  }
  delete[] raw_32bit_samples;
  return samples;
}
}  // namespace ai_vox