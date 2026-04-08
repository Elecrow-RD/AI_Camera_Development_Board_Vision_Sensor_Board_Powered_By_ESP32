#pragma once

#ifndef _I2S_STD_AUDIO_OUTPUT_DEVICE_H_
#define _I2S_STD_AUDIO_OUTPUT_DEVICE_H_

#include <driver/i2s_std.h>

#include <cmath>

#include "audio_output_device.h"

namespace ai_vox {
class I2sStdAudioOutputDevice : public AudioOutputDevice {
 public:
  I2sStdAudioOutputDevice(gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout);
  I2sStdAudioOutputDevice(const i2s_std_slot_config_t& slot_cfg, const i2s_std_gpio_config_t& gpio_cfg);
  ~I2sStdAudioOutputDevice();
  uint16_t volume() const override;
  void SetVolume(uint16_t volume) override;

  bool Open(uint32_t sample_rate) override;
  void Close() override;
  size_t Write(int16_t* pcm, size_t samples) override;
 private:


  i2s_chan_handle_t i2s_tx_handle_ = nullptr;
  i2s_std_slot_config_t slot_cfg_ = {
      .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
      .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
      .slot_mode = I2S_SLOT_MODE_STEREO,
      .slot_mask = I2S_STD_SLOT_BOTH,
      .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
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
  uint16_t volume_ = 70;
  int32_t volume_factor_ = pow(double(volume_) / 100.0, 2) * 65536;
};
}  // namespace ai_vox

#endif
