void BoardControl_Init() {
  // Keep CST816S out of reset. The old MUTE implementation accidentally
  // drove this pin because it used P1_4 instead of P1_5.
  aw9523.AW_pinMode(kAwTouchResetPin, OUTPUT);
  aw9523.AW_digitalWrite(kAwTouchResetPin, true);

  // NS4168 power is switched by a P-channel MOSFET: low enables audio.
  BoardControl_SetAudioMuted(true);
}

void BoardControl_SetAudioMuted(bool muted) {
  aw9523.AW_pinMode(kAwAudioCtrlPin, OUTPUT);
  aw9523.AW_digitalWrite(kAwAudioCtrlPin, muted);
}
