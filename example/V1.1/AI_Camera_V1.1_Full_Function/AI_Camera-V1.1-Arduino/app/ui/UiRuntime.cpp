static bool volume_object_is_valid(lv_obj_t *obj)
{
  return obj != nullptr && lv_obj_is_valid(obj);
}

static void set_volume_icon_hidden(lv_obj_t *obj, bool hidden)
{
  const bool is_hidden = lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN);
  if (hidden && !is_hidden) {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  } else if (!hidden && is_hidden) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
}

static void refresh_volume_ui(uint8_t volume)
{
  if (volume_object_is_valid(ui_SpeakerSlider)) {
    if (lv_slider_get_value(ui_SpeakerSlider) != volume) {
      lv_slider_set_value(ui_SpeakerSlider, volume, LV_ANIM_OFF);
    }
  }

  if (volume_object_is_valid(ui_speakerLab)) {
    char expected_text[5];
    snprintf(expected_text, sizeof(expected_text), "%u%%", static_cast<unsigned>(volume));
    if (strcmp(lv_label_get_text(ui_speakerLab), expected_text) != 0) {
      lv_label_set_text(ui_speakerLab, expected_text);
    }
  }

  if (!volume_object_is_valid(ui_AIChatVolumImg0) ||
      !volume_object_is_valid(ui_AIChatVolumImg1) ||
      !volume_object_is_valid(ui_AIChatVolumImg2) ||
      !volume_object_is_valid(ui_AIChatVolumImg3)) {
    return;
  }

  const uint8_t visible_icon = volume == 0 ? 0 : (volume < 33 ? 1 : (volume < 66 ? 2 : 3));
  set_volume_icon_hidden(ui_AIChatVolumImg0, visible_icon != 0);
  set_volume_icon_hidden(ui_AIChatVolumImg1, visible_icon != 1);
  set_volume_icon_hidden(ui_AIChatVolumImg2, visible_icon != 2);
  set_volume_icon_hidden(ui_AIChatVolumImg3, visible_icon != 3);
}

void lv_wifi_rssi_show() {
  if (current_wifi_connected) {
    if (lv_obj_is_valid(ui_WiFiNetworName)) {
      lv_async_call([](void *) {
        lv_label_set_text_fmt(ui_WiFiNetworName, "%s", wifi_ssid);
      },
      NULL);
    }

    if (lv_scr_act() == ui_AIRecognitionScr) {
      switch (RssiLev) {
        
        case 5:
          lv_obj_clear_flag(ui_AIRecognitionRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 4:
          lv_obj_clear_flag(ui_AIRecognitionRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 3:
          lv_obj_clear_flag(ui_AIRecognitionRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 2:
          lv_obj_clear_flag(ui_AIRecognitionRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 1:
          lv_obj_clear_flag(ui_AIRecognitionRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIRecognitionRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
      }
    } else if (lv_scr_act() == ui_AIChatSrc) {
      switch (RssiLev) {
        case 5:
          lv_obj_clear_flag(ui_AIChatSrcWiFiRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 4:
          lv_obj_clear_flag(ui_AIChatSrcWiFiRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 3:
          lv_obj_clear_flag(ui_AIChatSrcWiFiRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 2:
          lv_obj_clear_flag(ui_AIChatSrcWiFiRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 1:
          lv_obj_clear_flag(ui_AIChatSrcWiFiRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatSrcWiFiRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
      }
    } else if (lv_scr_act() == ui_SettingScr) {
      switch (RssiLev) {
        case 5:
          lv_obj_clear_flag(ui_SettingSrcRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 4:
          lv_obj_clear_flag(ui_SettingSrcRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 3:
          lv_obj_clear_flag(ui_SettingSrcRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 2:
          lv_obj_clear_flag(ui_SettingSrcRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 1:
          lv_obj_clear_flag(ui_SettingSrcRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
      }
    } else if (lv_scr_act() == ui_AIChatMenu) {
      switch (RssiLev) {
        case 5:
          lv_obj_clear_flag(ui_AIChatMenuRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 4:
          lv_obj_clear_flag(ui_AIChatMenuRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 3:
          lv_obj_clear_flag(ui_AIChatMenuRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 2:
          lv_obj_clear_flag(ui_AIChatMenuRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 1:
          lv_obj_clear_flag(ui_AIChatMenuRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_AIChatMenuRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
      }
    } else if (lv_scr_act() == ui_SettingSrcMenu) {
      switch (RssiLev) {
        case 5:
          lv_obj_clear_flag(ui_SettingSrcMenuRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 4:
          lv_obj_clear_flag(ui_SettingSrcMenuRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 3:
          lv_obj_clear_flag(ui_SettingSrcMenuRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 2:
          lv_obj_clear_flag(ui_SettingSrcMenuRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 1:
          lv_obj_clear_flag(ui_SettingSrcMenuRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_SettingSrcMenuRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
      }
    } else if(lv_scr_act() == ui_ShotdownScr) {
      switch (RssiLev) {
        case 5:
          lv_obj_clear_flag(ui_PoweroffSrcRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 4:
          lv_obj_clear_flag(ui_PoweroffSrcRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 3:
          lv_obj_clear_flag(ui_PoweroffSrcRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 2:
          lv_obj_clear_flag(ui_PoweroffSrcRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
        case 1:
          lv_obj_clear_flag(ui_PoweroffSrcRssi1, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi5, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi4, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi3, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi2, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ui_PoweroffSrcRssi0, LV_OBJ_FLAG_HIDDEN);
          break;
      }
    }

  } else {
    if (lv_scr_act() == ui_AIRecognitionScr) {
      lv_obj_clear_flag(ui_AIRecognitionRssi0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIRecognitionRssi5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIRecognitionRssi4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIRecognitionRssi3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIRecognitionRssi2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIRecognitionRssi1, LV_OBJ_FLAG_HIDDEN);
    } else if (lv_scr_act() == ui_AIChatSrc) {
      lv_obj_clear_flag(ui_AIChatSrcWiFiRssi0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIChatSrcWiFiRssi5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIChatSrcWiFiRssi4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIChatSrcWiFiRssi3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIChatSrcWiFiRssi2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIChatSrcWiFiRssi1, LV_OBJ_FLAG_HIDDEN);
    } else if (lv_scr_act() == ui_SettingScr) {
      lv_obj_clear_flag(ui_SettingSrcRssi0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_SettingSrcRssi5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_SettingSrcRssi4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_SettingSrcRssi3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_SettingSrcRssi2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_SettingSrcRssi1, LV_OBJ_FLAG_HIDDEN);
    } else if (lv_scr_act() == ui_AIChatMenu) {
      lv_obj_clear_flag(ui_AIChatMenuRssi0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIChatMenuRssi5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIChatMenuRssi4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIChatMenuRssi3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIChatMenuRssi2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIChatMenuRssi1, LV_OBJ_FLAG_HIDDEN);
    } else if (lv_scr_act() == ui_SettingSrcMenu) {

      lv_obj_clear_flag(ui_SettingSrcMenuRssi0, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_SettingSrcMenuRssi5, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_SettingSrcMenuRssi4, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_SettingSrcMenuRssi3, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_SettingSrcMenuRssi2, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_SettingSrcMenuRssi1, LV_OBJ_FLAG_HIDDEN);
    }
  }
}
bool cfg_is_dirty = false;
uint32_t last_brightness_change_ms = 0;
const uint32_t SAVE_DELAY_MS = 1000;
void system_set() {
  // SquareLine's generated source still contains the V1.0 design-time text.
  // Override it at runtime so src/UI can remain generated and untouched.
  if (ui_VersionLab != NULL &&
      strcmp(lv_label_get_text(ui_VersionLab), "Version V1.1") != 0) {
    lv_label_set_text(ui_VersionLab, "Version V1.1");
  }

  /* Screen Brightness */
  if (g_cfg.brightness != brightness_value) {
    g_cfg.brightness = brightness_value;
    
    aw9523.AW_set_lcd_blight(brightness_value);
    if(lv_scr_act() == ui_ScreenBrightness)
    {
        lv_slider_set_value(ui_BrightnessSlider, brightness_value, LV_ANIM_OFF);
        g_screen_iot_entity->UpdateState("brightness", brightness_value);
    }
    cfg_is_dirty = true;
    last_brightness_change_ms = millis();
    // if (brightness_timer != NULL) {
    //   xTimerDelete(brightness_timer, 0);
    // }
    // brightness_timer = xTimerCreate("brightnesstimer", pdMS_TO_TICKS(100), pdFALSE, (void *)0, saveBrightness);
    // xTimerStart(brightness_timer, 0);
  }

  if (cfg_is_dirty) {
          // 如果距离最后一次滑动已经过去了 1000ms
          if (millis() - last_brightness_change_ms > SAVE_DELAY_MS) {
              TaskHandle_t camTaskHandle = xTaskGetHandle("cam_task");
        
          // 2. 如果任务存在，先挂起它，防止它在写 Flash 时访问 Cache
          if (camTaskHandle != NULL) {
              vTaskSuspend(camTaskHandle);
          }
          saveBrightness(); // 执行写入 Flash 的操作
          cfg_is_dirty = false; // 清除标志位
          if (camTaskHandle != NULL) {
              vTaskResume(camTaskHandle);
          }
      }
  }

  if(screen_power == false)
  {
    if (digitalRead(BOOT_PIN) == LOW)
    {
      aw9523.AW_set_lcd_blight(g_cfg.brightness);
      g_screen_iot_entity->UpdateState("power", true);
    }
  }

  if(change_screen_sd_card)
  {
    change_screen_sd_card = false;
    _ui_screen_change(&ui_SDCard, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_SDCard_screen_init);
  }
  if (SD_Card_info_updata) {
    SD_Card_info_updata = false;
    lv_label_set_text(ui_SDCardLab, g_sdcard_text);
    lv_obj_clear_flag(ui_SDCardReturnImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_SDCardReturnBtn, LV_OBJ_FLAG_HIDDEN);
  }

  if(g_cfg.volume != speaker_value)
  {
    g_cfg.volume = speaker_value;
    
    g_audio_output_device->SetVolume(speaker_value);
    g_speaker_iot_entity->UpdateState("volume", speaker_value);

    if (volume_timer != NULL) {
        xTimerDelete(volume_timer, 0);
    }
    volume_timer = xTimerCreate("VolumeTimer", pdMS_TO_TICKS(500), pdFALSE, (void*)0, save_volume_callback);
    xTimerStart(volume_timer, 0);

  }

  if (led_status != led_status_pre) {
    led_status_pre = led_status;
    if (led_status == true) {
      aw9523.AW_set_SLED(true);
    } else {
      aw9523.AW_set_SLED(false);
    }
  }

  switch(RGB_Light_index)
  {
    case 0:
      aw9523.AW_set_RGB(0x000000);
      RGB_Light_index = 4;
      break;
    case 1:
      aw9523.AW_set_RGB(0xFF0000);
      break;
    case 2:
      aw9523.AW_set_RGB(0x00FF00);
      break;
    case 3:
      aw9523.AW_set_RGB(0x0000FF);
      break;
    default:
      RGB_Light_index = 4;
      break;
  }


  if (RGBLighCloseSem) {
    RGBLighCloseSem = false;
    RGB_Light_index = 0;
  }

  uint16_t speaker_val = g_audio_output_device->volume();

  if (speaker_value != speaker_val)
  {
    speaker_value = speaker_val;
  }

  // Keep the AI command value, the real player value, the slider/label and
  // the top-right icon synchronised even when the volume screen was already
  // open before an AI SetVolume command arrived.
  refresh_volume_ui(speaker_value);

  if(aichat_led_open)
  {
    aichat_led_open = false;
    lv_obj_add_flag(ui_AIRecognitionLedOffImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_AIRecognitionLedOnImg, LV_OBJ_FLAG_HIDDEN);
  }
  if(aichat_led_close)
  {
      aichat_led_close = false;
      lv_obj_add_flag(ui_AIRecognitionLedOnImg, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(ui_AIRecognitionLedOffImg, LV_OBJ_FLAG_HIDDEN);
  }
  if(aichat_RGB_Light_Open)
  {
      aichat_RGB_Light_Open = false;
      vTaskResume(RGB_Light_task_handle);
  }

  if(aichat_RGB_Light_Close)
  {
      aichat_RGB_Light_Close = false;
      //aw9523.AW_set_RGB(0x000000);
      RGB_Light_index = 0;
      vTaskSuspend(RGB_Light_task_handle);
  }

    if(reconfigure_btn)
    {
        preferences.begin("wifi-reconfig", false); 

        preferences.putBool("restart_flag", true);  

        preferences.end();  

        preferences.begin("wifi-config", false);

        preferences.clear();

        preferences.end(); 

        delay(100);
        ESP.restart();
    }

    if(reset_to_factory)
    {
        reset_to_factory = false;
        preferences.begin("wifi-config", false);

        preferences.clear();

        preferences.end();

        preferences.begin("settings", false);

        preferences.clear();

        preferences.end();

        preferences.begin("wifi-reconfig", false); 

        preferences.putBool("restart_flag", true);  

        preferences.end();

        delay(100);
        ESP.restart();
    }

    if(Power_Off)
    {
      Power_Off = false;
      WiFi_Disconnect();
      BoardControl_SetAudioMuted(true);
      delay(100);
      aw9523.AW_set_lcd_blight(0);
      delay(100);
      aw9523.AW_set_POWER(false);
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
      esp_sleep_config_gpio_isolate();
      
      esp_deep_sleep_start();
    }
}

void device_active_check_steup_handle() {
  if (device_active_check) {
    if (device_active) {
      device_active_check = false;
      lv_obj_add_flag(ui_Label5, LV_OBJ_FLAG_HIDDEN); //hidden activation code
      if(current_active_screen == 3)
      {
        //AI Recognition
          gpio_isr_handler_remove(kTriggerPin);
          current_fun_is_Recognition = true;
          _ui_screen_change(&ui_AIRecognition, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_AIRecognition_screen_init);
          
      }
      else if (current_active_screen == 1)
      {
        //ELECROW Server
        _ui_screen_change(&ui_AIChat, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_AIChat_screen_init);
        //vTaskResume(aichat_task_handle);
        //aichat_steup = 1;
        Touch = 1;
      }
      else if (current_active_screen == 2)
      {
        //XiaoZhi Server
        _ui_screen_change(&ui_AIChat, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, &ui_AIChat_screen_init);
        // vTaskResume(aichat_task_handle);
        // aichat_steup = 1;
        Touch = 1;
      }
      
      
    } 
    else 
    {
      if(get_active_code)
      {
          get_active_code = false;
          lv_obj_add_flag(ui_ServerConnectSpinner, LV_OBJ_FLAG_HIDDEN);
          lv_label_set_text(ui_Label5, activationCode.c_str());
          lv_obj_clear_flag(ui_Label5, LV_OBJ_FLAG_HIDDEN);
          if(current_active_screen == 3)
          {
            //AI Recognition
              lv_label_set_text(ui_Label4, "Open\"https://portal.thinkno\nde.cc/\" active device");
              lv_obj_clear_flag(ui_Label4, LV_OBJ_FLAG_HIDDEN);

          }
          else if (current_active_screen == 1)
          {
            //ELECROW Server
              lv_label_set_text(ui_Label4, "Open\"https://portal.thinkno\nde.cc/\" active device");
              lv_obj_clear_flag(ui_Label4, LV_OBJ_FLAG_HIDDEN);

          }
          else if (current_active_screen == 2)
          {
            //XiaoZhi Server
              lv_label_set_text(ui_Label4, "Open\"https://xiaozhi.me/\"\n active device");
              lv_obj_clear_flag(ui_Label4, LV_OBJ_FLAG_HIDDEN);

          }
      }
      vTaskResume(aichat_task_handle);
    }

    if(end_active)
    {
      end_active = false;
      device_active_check = false;
      //aichat_exit = true;
      vTaskSuspend(aichat_task_handle);
    }
  }
}

void lv_hidden_emotion()
{
    lv_obj_add_flag(ui_AngerImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_BlinkingImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_ConfidenceImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_ConfusionImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_CryingImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DeliciousnessImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DescriptionImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_DrowsinessImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_EmbarrassmeImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_FunnyImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_HappinessImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_ThinkingImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_StupidityImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_SpeechImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_SleepyImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_ShockImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_SadnessImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_RelaxImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LoveImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_ListeningImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LaughHeartilyImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_KissImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_InterestImg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_NeutralityImg, LV_OBJ_FLAG_HIDDEN);
}

void AIChat_Status_lvgl()
{ 
    switch(emotion_index)
    {
      case 0:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_NeutralityImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 1:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_HappinessImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 2:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_LaughHeartilyImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 3:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_FunnyImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 4:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_SadnessImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 5:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_AngerImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 6:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_CryingImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 7:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_LoveImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 8:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_EmbarrassmeImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 9:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_InterestImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 10:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_ShockImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 11:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_ThinkingImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 12:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_BlinkingImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 13:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_SpeechImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 14:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_RelaxImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 15:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_DeliciousnessImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 16:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_KissImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 17:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_ConfidenceImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 18:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_SleepyImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 19:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_NeutralityImg, LV_OBJ_FLAG_HIDDEN);
        break;
      case 20:
        lv_hidden_emotion();
        lv_obj_clear_flag(ui_ConfusionImg, LV_OBJ_FLAG_HIDDEN);
        break;
      default :
        lv_hidden_emotion();
        break;
    }

    switch(aichat_status)
    {
        case AIChat_Idle:

            break;
        case AIChat_Initing:

            break;
        case AIChat_Standby:
            lv_label_set_text(ui_Label2, "Standby");
            //lv_obj_clear_flag(ui_Image6, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui_Label2, LV_OBJ_FLAG_HIDDEN);
            emotion_index = 0;
            
            aichat_status = AIChat_Other;
            break;
        case AIChat_Connecting:
            lv_label_set_text(ui_Label2, "Connecting...");
            lv_obj_add_flag(ui_Image6, LV_OBJ_FLAG_HIDDEN);
            aichat_status = AIChat_Other;
            emotion_index_pre = -1;

            break;
        case AIChat_Listening:
            lv_label_set_text(ui_Label2, "Listening...");
            lv_obj_add_flag(ui_Image6, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_clear_flag(ui_Image6, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui_Label2, LV_OBJ_FLAG_HIDDEN); 
            aichat_status = AIChat_Other;
            g_text_ready = false;
            emotion_index_pre = 1;
            //emotion_index = -1;
            break;
        case AIChat_Speaking:
            lv_obj_add_flag(ui_Image6, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Label2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui_NeutralityImg, LV_OBJ_FLAG_HIDDEN);
            aichat_status = AIChat_Other;
            break;
    }

    if (g_text_ready)
    {
        g_text_ready = false;
        lv_obj_add_flag(ui_Image6, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_Label2, g_pending_text.c_str());
        lv_obj_clear_flag(ui_Label2, LV_OBJ_FLAG_HIDDEN);
    }
}

bool In_AIRecognition_Save_Src = false;
