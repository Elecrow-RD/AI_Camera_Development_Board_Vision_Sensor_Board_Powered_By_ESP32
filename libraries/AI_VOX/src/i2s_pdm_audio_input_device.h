#pragma once

#include <driver/i2s_pdm.h>
#include "audio_input_device.h"

namespace ai_vox {

class I2sPdmAudioInputDevice : public AudioInputDevice {
 public:
  I2sPdmAudioInputDevice(gpio_num_t clk, gpio_num_t din);
  I2sPdmAudioInputDevice(const i2s_pdm_rx_slot_config_t& slot_cfg,
                         const i2s_pdm_rx_gpio_config_t& gpio_cfg);
  ~I2sPdmAudioInputDevice();

 private:
  bool Open(uint32_t sample_rate) override;
  void Close() override;
  bool Enable();  
  bool Disable(); 
  size_t Read(int16_t* buffer, uint32_t samples) override;

  i2s_chan_handle_t i2s_rx_handle_ = nullptr;
  i2s_pdm_rx_slot_config_t slot_cfg_ = {
    .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
    .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
    .slot_mode = I2S_SLOT_MODE_MONO,
    .slot_mask = I2S_PDM_SLOT_LEFT,

  };
  i2s_pdm_rx_gpio_config_t gpio_cfg_;
};

}  // namespace ai_vox
