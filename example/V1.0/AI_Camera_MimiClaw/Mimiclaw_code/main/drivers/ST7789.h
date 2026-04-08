#ifndef _ST7789_H__
#define _ST7789_H__

#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "lvgl.h"

#define DISPLAY_TAG "DISPLAY"
#define DISPLAY_INFO(fmt, ...) ESP_LOGI(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_DEBUG(fmt, ...) ESP_LOGD(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_ERROR(fmt, ...) ESP_LOGE(DISPLAY_TAG, fmt, ##__VA_ARGS__)


#define BSP_H_SIZE 240
#define BSP_V_SIZE 284

void screen_init();

#endif