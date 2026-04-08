

#include "ST7789.h"

#define LCD_HOST    SPI2_HOST

#define PIN_NUM_MOSI    13
#define PIN_NUM_CLK     14
#define PIN_NUM_CS      10
#define PIN_NUM_DC      9
#define PIN_NUM_RST     -1

#define LCD_H_RES 240
#define LCD_V_RES 284

static const char * TAG = "lcd_pannel";

esp_lcd_panel_handle_t panel;

static lv_display_t *my_lvgl_disp = NULL;


static void lcd_st7789_custom_init(esp_lcd_panel_io_handle_t io)
{
    esp_lcd_panel_io_tx_param(io, 0x01, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(120));

    esp_lcd_panel_io_tx_param(io, 0x11, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(120));

    uint8_t data = 0x55;
    esp_lcd_panel_io_tx_param(io, 0x3A, &data, 1);

    data = 0x08;
    esp_lcd_panel_io_tx_param(io, 0x36, &data, 1);

    uint8_t proch[5] = {0x00C, 0x0C, 0x00, 0x33, 0x33};
    esp_lcd_panel_io_tx_param(io, 0xB2, proch, 5);

    data = 0x05;
    esp_lcd_panel_io_tx_param(io, 0xB7, &data, 1);

    data = 0x21;
    esp_lcd_panel_io_tx_param(io, 0xBB, &data, 1);

    data = 0x2C;
    esp_lcd_panel_io_tx_param(io, 0xC0, &data, 1);

    data = 0x01;
    esp_lcd_panel_io_tx_param(io, 0xC2, &data, 1);

    data = 0x15;
    esp_lcd_panel_io_tx_param(io, 0xC3, &data, 1);

    data = 0x0F;
    esp_lcd_panel_io_tx_param(io, 0xC6, &data, 1);

    data = 0xD0;
    esp_lcd_panel_io_tx_param(io, 0xD0, &data, 1);

    uint8_t d0[] = {0xA4, 0xA1};
    esp_lcd_panel_io_tx_param(io, 0xD0, d0, 2);

    data = 0xA1;
    esp_lcd_panel_io_tx_param(io, 0xD6, &data, 1);

    uint8_t gamma_pos[14] = {
        0xF0, 0x05, 0x0E, 0x08, 0x0A, 0x17, 0x39,
        0x54, 0x4E, 0x37, 0x12, 0x12, 0x21, 0x37
    };
    esp_lcd_panel_io_tx_param(io, 0xE0, gamma_pos, 14);
    
    uint8_t gamma_neg[14] = {
        0xF0, 0x10, 0x14, 0x0D, 0x0B, 0x05, 0x39,
        0x44, 0x4D, 0x38, 0x14, 0x14, 0x2E, 0x35
    };
    esp_lcd_panel_io_tx_param(io, 0xE1, gamma_neg, 14);

    uint8_t e4[] = {0x23, 0x00, 0x00};
    esp_lcd_panel_io_tx_param(io, 0xE4, e4, 3);

    esp_lcd_panel_io_tx_param(io, 0x21, NULL, 0);

    esp_lcd_panel_io_tx_param(io, 0x29, NULL, 0);

    esp_lcd_panel_io_tx_param(io, 0x2C, NULL, 0);

    ESP_LOGI(TAG, "ST7789 initialization sequence sent");
}

void screen_fill() {
    int w = 240;
    int h = 284;
    uint16_t *test_buf = heap_caps_malloc(w * 10 * 2, MALLOC_CAP_DMA); 
    if (test_buf) {
        for (int i = 0; i < w * 10; i++) test_buf[i] = 0x0000; 
        for (int j = 0; j < h; j += 10) {
            esp_lcd_panel_draw_bitmap(panel, 0, j, w, j + 10, test_buf);
        }
        free(test_buf);
    }
}


/**
 * @brief Initialize the LVGL graphics library and register display/input drivers.
 * @note Task is pinned to Core 1 to avoid interference with Core 0 networking.
 * @return
 * - ESP_OK: LVGL system ready.
 * - ESP_FAIL: Failed to add display or touch interface to LVGL.
 */
static esp_err_t lvgl_init()
{
    esp_err_t err = ESP_OK;
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 3,       /* LVGL task priority */
        .task_stack = 8192,       /* LVGL task stack size */
        .task_affinity = 1,       /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500, /* Maximum sleep in LVGL task */
        .timer_period_ms = 20,    /* LVGL timer tick period in ms */
    };
    err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK)
    {
        DISPLAY_ERROR("LVGL port initialization failed");
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .panel_handle = panel,
        .control_handle = panel,
        .buffer_size = BSP_H_SIZE * 40,
        .double_buffer = false,
        .hres = BSP_H_SIZE,
        .vres = BSP_V_SIZE,
        .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
            .sw_rotate = false,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,
#endif
#if BSP_DISPLAY_LVGL_FULL_REFRESH
            .full_refresh = true,
#else
            .full_refresh = false,
#endif
#if BSP_DISPLAY_LVGL_DIRECT_MODE
            .direct_mode = true,
#else
            .direct_mode = false,
#endif
        },
    };
    const lvgl_port_display_rgb_cfg_t lvgl_rgb_cfg = {
        .flags = {
#if BSP_DISPLAY_LVGL_BOUNCE_BUFFER_MODE
            .bb_mode = true,
#else
            .bb_mode = false,
#endif
#if BSP_DISPLAY_LVGL_AVOID_TEAR
            .avoid_tearing = true,
#else
            .avoid_tearing = false,
#endif
        }};

    my_lvgl_disp = lvgl_port_add_disp_rgb(&disp_cfg, &lvgl_rgb_cfg);
    if (my_lvgl_disp == NULL)
    {
        err = ESP_FAIL;
        DISPLAY_ERROR("LVGL rgb port add fail");
    }
#ifdef BSP_TOUCH_ENABLED
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = my_lvgl_disp,
        .handle = touch_handle,
    };
    my_touch_indev = lvgl_port_add_touch(&touch_cfg);
    if (my_touch_indev == NULL)
    {
        err = ESP_FAIL;
        DISPLAY_ERROR("LVGL touch port add fail");
    }
#endif
    if (lvgl_port_lock(0))
    {
        lv_obj_t *screen = lv_scr_act();
        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
        lv_obj_t *label = lv_label_create(screen);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
        lv_label_set_text(label, "System Ready...");
        lv_obj_center(label);
        lvgl_port_unlock();
    }
    return err;
}

void screen_init()
{
    spi_bus_config_t bus_config = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 20 * 2
    };

    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 20 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 5
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io));

    lcd_st7789_custom_init(io);

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &panel_config, &panel));


    esp_lcd_panel_set_gap(panel, 0, 0);
    /* 这里使用自定义参数 */
    //ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    // screen_fill();

    ESP_LOGI(TAG, "ST7789 panel initialized and display turned on");

    lvgl_init();

    ESP_LOGI(TAG, "LVGL v8 initialized");
}