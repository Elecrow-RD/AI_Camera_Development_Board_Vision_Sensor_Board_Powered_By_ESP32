#include "i2s_pdm_audio_input_device.h"
#include <Arduino.h>
namespace ai_vox {

I2sPdmAudioInputDevice::I2sPdmAudioInputDevice(gpio_num_t clk, gpio_num_t din) {
  gpio_cfg_ = {
      .clk = clk,
      .din = din,
      .invert_flags = {
          .clk_inv = false,
      },
  };
}

I2sPdmAudioInputDevice::I2sPdmAudioInputDevice(const i2s_pdm_rx_slot_config_t& slot_cfg,
                                               const i2s_pdm_rx_gpio_config_t& gpio_cfg)
    : slot_cfg_(slot_cfg), gpio_cfg_(gpio_cfg) {}

I2sPdmAudioInputDevice::~I2sPdmAudioInputDevice() {
  Close();
}

bool I2sPdmAudioInputDevice::Open(uint32_t sample_rate) {
  Close();

  i2s_chan_config_t rx_chan_cfg = {
      .id = I2S_NUM_0,
      .role = I2S_ROLE_MASTER,
      .dma_desc_num = 4,
      .dma_frame_num = 320,
      .auto_clear_after_cb = true,
      .auto_clear_before_cb = false,
      .allow_pd = false,
      .intr_priority = 0,

  };

  ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, nullptr, &i2s_rx_handle_));

  i2s_pdm_rx_config_t rx_pdm_cfg = {
      .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(sample_rate),
      .slot_cfg = slot_cfg_,
      .gpio_cfg = gpio_cfg_,
  };

  ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(i2s_rx_handle_, &rx_pdm_cfg));
  ESP_ERROR_CHECK(i2s_channel_enable(i2s_rx_handle_));
  
  return true;
}

void I2sPdmAudioInputDevice::Close() {
  if (i2s_rx_handle_ == nullptr) {
    return;
  }
  i2s_channel_disable(i2s_rx_handle_);
  i2s_del_channel(i2s_rx_handle_);                                                
  i2s_rx_handle_ = nullptr;
}

bool I2sPdmAudioInputDevice::Enable() {
    if (i2s_rx_handle_ == nullptr) {
        return false;  // 没有打开，不能enable
    }
    esp_err_t err = i2s_channel_enable(i2s_rx_handle_);
    return (err == ESP_OK);
}

bool I2sPdmAudioInputDevice::Disable() {
    if (i2s_rx_handle_ == nullptr) {
        return false;  // 没有打开，不能disable
    }
    esp_err_t err = i2s_channel_disable(i2s_rx_handle_);
    return (err == ESP_OK);
}

size_t I2sPdmAudioInputDevice::Read(int16_t* buffer, uint32_t samples) {
    auto raw_16bit_samples = new int16_t[samples];
    size_t bytes_read = 0;
    i2s_channel_read(i2s_rx_handle_, raw_16bit_samples, samples * sizeof(raw_16bit_samples[0]), &bytes_read, 1000);

    for (int i = 0; i < samples; i++) {
        int32_t value = raw_16bit_samples[i];
        buffer[i] = (value > INT16_MAX) ? INT16_MAX : (value < INT16_MIN) ? INT16_MIN : (int16_t)value;
    }
    
    delete[] raw_16bit_samples;
    
    return samples;
}

}  // namespace ai_vox
