#pragma once

#ifndef _AUDIO_INPUT_DEVICE_H_
#define _AUDIO_INPUT_DEVICE_H_

#include <cstdint>
#include <vector>

namespace ai_vox {
class AudioInputDevice {
 public:
  virtual ~AudioInputDevice() = default;
  virtual bool Open(uint32_t sample_rate) = 0;
  virtual void Close() = 0;
  virtual size_t Read(int16_t* buffer, uint32_t samples) = 0;

  virtual bool Enable() { return true; }
  virtual bool Disable() { return true; }
};
}  // namespace ai_vox

#endif