#pragma once

#ifndef _I2S_STD_AUDIO_INPUT_DEVICE_H_
#define _I2S_STD_AUDIO_INPUT_DEVICE_H_

#include <driver/i2s_std.h>

#include "audio_input_device.h"

namespace ai_vox {
class I2sStdAudioInputDevice : public AudioInputDevice {
 public:
  I2sStdAudioInputDevice(gpio_num_t bclk, gpio_num_t ws, gpio_num_t din);
  I2sStdAudioInputDevice(const i2s_std_slot_config_t& slot_cfg, const i2s_std_gpio_config_t& gpio_cfg);
  ~I2sStdAudioInputDevice();

 private:
  bool Open(uint32_t sample_rate) override;
  void Close() override;
  size_t Read(int16_t* buffer, uint32_t samples) override;

  i2s_chan_handle_t i2s_rx_handle_ = nullptr;
  i2s_std_slot_config_t slot_cfg_ = {
      .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
      .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
      .slot_mode = I2S_SLOT_MODE_MONO,
      .slot_mask = I2S_STD_SLOT_LEFT,
      .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
      .ws_pol = false,
      .bit_shift = true,
#if SOC_I2S_HW_VERSION_1
      .msb_right = false,
#else
      .left_align = true,
      .big_endian = false,
      .bit_order_lsb = false,
#endif
  };
  i2s_std_gpio_config_t gpio_cfg_;
};
}  // namespace ai_vox
#endif