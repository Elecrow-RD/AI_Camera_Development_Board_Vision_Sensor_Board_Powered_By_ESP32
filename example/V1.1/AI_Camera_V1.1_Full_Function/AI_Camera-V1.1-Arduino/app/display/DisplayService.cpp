void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  if (gfx.getStartCount() > 0) {
    gfx.endWrite();
  }
  gfx.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::rgb565_t *)&color_p->full);

  lv_disp_flush_ready(disp);  //  Tell lvgl that the refresh is complete
}

uint16_t touchX, touchY;
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  data->state = LV_INDEV_STATE_REL;
  if (gfx.getTouch(&touchX, &touchY)) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = 240 - touchX;

    data->point.y = touchY + 10;

  }
}

void loadPreferences(DeviceConfig &cfg) {
  // ---------- settings ----------
  preferences.begin("settings", true);
  brightness_value = preferences.getUChar("brightness", 100);
  speaker_value = preferences.getInt("volume", 70);
  preferences.end();

  // ---------- wifi-reconfig ----------
  preferences.begin("wifi-reconfig", true);
  cfg.restartFlag = preferences.getBool("restart_flag", true);
  preferences.end();

  // ---------- wifi-config ----------
  preferences.begin("wifi-config", true);
  cfg.ssid = preferences.getString("ssid", "");
  cfg.password = preferences.getString("password", "");
  preferences.end();
}

void save_volume_callback(TimerHandle_t xTimer) {
    preferences.begin("settings", false);  
    preferences.putInt("volume", g_cfg.volume);  
    preferences.end(); 
}

void saveBrightness() {
  preferences.begin("settings", false);
  preferences.putUChar("brightness", g_cfg.brightness);
  preferences.end();
}

void lvgl_camera_update_cb(lv_timer_t *timer) {
  if (!g_camera_frame_ready) return;

  img_dsc.header.always_zero = 0;
  img_dsc.header.w = 240;
  img_dsc.header.h = 284;
  img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  img_dsc.data_size = 240 * 284 * 2;
  img_dsc.data = (uint8_t *)g_camera_frame;
  if (camera_output == NULL && g_show_recognition == true) {
    camera_output = ui_AIRecognitionSaveImg;
  } 
  else if (camera_output == NULL && AIRecognition_start_flag == false) {
    camera_output = ui_AIRecognitionDisplayImg;
  }
  lv_img_set_src(camera_output, &img_dsc);

  g_camera_frame_ready = false;
}

void hide_sd_label_cb(lv_timer_t * t) {
    lv_obj_t * label = (lv_obj_t *)t->user_data; 
    lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);  
}

void on_chat_message(const std::string& text)
{
    g_pending_text = text;   // 拷贝内容
    g_text_ready = true;     // 标记有新数据
}

int get_emotion_index(const std::string& emotion_name)
{
    for (size_t i = 0; i < EMOTION_COUNT; ++i) {
        if (emotion_table[i].name == emotion_name) {
            return emotion_table[i].id;
        }
    }
    return -1; 
}


