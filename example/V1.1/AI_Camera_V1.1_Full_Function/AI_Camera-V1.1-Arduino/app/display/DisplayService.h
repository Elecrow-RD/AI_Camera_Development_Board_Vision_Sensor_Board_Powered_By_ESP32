#pragma once

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void loadPreferences(DeviceConfig &cfg);
void save_volume_callback(TimerHandle_t xTimer);
void saveBrightness();
void lvgl_camera_update_cb(lv_timer_t *timer);
void hide_sd_label_cb(lv_timer_t *timer);
void on_chat_message(const std::string &text);
int get_emotion_index(const std::string &emotion_name);
