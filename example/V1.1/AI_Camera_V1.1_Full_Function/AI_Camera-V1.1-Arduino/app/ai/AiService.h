#pragma once

String GetActivationCode();
bool IsDeviceActivated();
uint8_t *save_camera_frame(const camera_fb_t *src_fb);
void aichat_task(void *pvParameters);
void AI_Recognition_func();
