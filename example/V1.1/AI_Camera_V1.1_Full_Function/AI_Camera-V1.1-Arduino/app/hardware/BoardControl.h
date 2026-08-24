#pragma once

// AI Camera V1.1 AW9523 pin mapping (P1_x maps to 8 + x).
constexpr uint8_t kAwTouchResetPin = 12;  // P1_4 / TPRST
constexpr uint8_t kAwAudioCtrlPin = 13;   // P1_5 / NS_CTRL

void BoardControl_Init();
void BoardControl_SetAudioMuted(bool muted);
