#include <Arduino.h>
#include <lvgl.h>
#include "src\GFX\LovyanGFX_Driver.h"       // LovyanGFX driver for display
#include "src\AW9523\elecrow_aw9523.h"      // AW9523 I2C LED/IO expander
#include "src\Camera\elecrow_camera.h"      // Camera abstraction
#include "src\SD\elecrow_sd.h"              // SD card abstraction

// I2C pins for AW9523
#define I2C_SCL 15
#define I2C_SDA 16

/* Camera pin definitions */
#define CAMERA_PWDN_PIN -1
#define CAMERA_RESET_PIN -1
#define CAMERA_XCLK_PIN 39 
#define CAMERA_SCL_PIN 40   
#define CAMERA_SDA_PIN 41 
#define CAMERA_D0_PIN 20
#define CAMERA_D1_PIN 18
#define CAMERA_D2_PIN 17
#define CAMERA_D3_PIN 19
#define CAMERA_D4_PIN 21
#define CAMERA_D5_PIN 47
#define CAMERA_D6_PIN 38
#define CAMERA_D7_PIN 46
#define CAMERA_VSYNC_PIN 42
#define CAMERA_HREF_PIN 45   
#define CAMERA_PCLK_PIN 48 

/* SD card SPI pins */
#define SPI_NSS_PIN   -1
#define SPI_MISO_PIN  6
#define SPI_SCK_PIN   7
#define SPI_MOSI_PIN  8

// Display resolution
#define LCD_H   284
#define LCD_W   240

// Task handles
static TaskHandle_t camera_task_handle = NULL;

// LVGL buffers and drivers
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv; 
static lv_indev_drv_t indev_drv;    

// Camera frame buffer
static uint16_t *g_camera_frame = NULL;
static volatile bool g_camera_frame_ready = false;  // Flag to indicate new frame available
static lv_timer_t *camera_timer = NULL; 
static lv_img_dsc_t img_dsc;  // LVGL image descriptor

lv_obj_t *camera_output = NULL;  // LVGL image object for display

// SD and I2C camera objects
static ELECROW_SD my_sd(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_NSS_PIN);
static TwoWire* wi = &Wire;
static ELECROW_CAMERA my_camera;

// Camera configuration
camera_config_t config = {
  .pin_pwdn = CAMERA_PWDN_PIN,
  .pin_reset = CAMERA_RESET_PIN,
  .pin_xclk = CAMERA_XCLK_PIN,
  .pin_sccb_sda = CAMERA_SDA_PIN,
  .pin_sscb_scl = CAMERA_SCL_PIN,
  .pin_d7 = CAMERA_D7_PIN,
  .pin_d6 = CAMERA_D6_PIN,
  .pin_d5 = CAMERA_D5_PIN,
  .pin_d4 = CAMERA_D4_PIN,
  .pin_d3 = CAMERA_D3_PIN,
  .pin_d2 = CAMERA_D2_PIN,
  .pin_d1 = CAMERA_D1_PIN,
  .pin_d0 = CAMERA_D0_PIN,
  .pin_vsync = CAMERA_VSYNC_PIN,
  .pin_href = CAMERA_HREF_PIN,
  .pin_pclk = CAMERA_PCLK_PIN,
  .xclk_freq_hz = 10000000,           // XCLK frequency 10 MHz
  .ledc_timer = LEDC_TIMER_0,         // LEDC timer for XCLK
  .ledc_channel = LEDC_CHANNEL_0,     // LEDC channel
  .pixel_format = PIXFORMAT_RGB565,   // Output RGB565 for display
  .frame_size = FRAMESIZE_CIF,        // Camera frame size
  .jpeg_quality = 12,
  .fb_count = 2,                       // Double buffering
  .fb_location = CAMERA_FB_IN_PSRAM,   // Store framebuffer in PSRAM
  .grab_mode = CAMERA_GRAB_LATEST,     // Grab latest frame mode
};

/* ================= LVGL display flush callback ================= */
// This function is called by LVGL to flush a portion of the buffer to the screen
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  if (gfx.getStartCount() > 0) gfx.endWrite();
  gfx.pushImageDMA(area->x1, area->y1,
                   area->x2 - area->x1 + 1,
                   area->y2 - area->y1 + 1,
                   (lgfx::rgb565_t *)&color_p->full);
  lv_disp_flush_ready(disp); // Tell LVGL we're done flushing
}

/* ================= Touch input read callback ================= */
uint16_t touchX, touchY;
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  data->state = LV_INDEV_STATE_REL; // default released
  if (gfx.getTouch(&touchX, &touchY)) { // read touch coordinates
    data->state = LV_INDEV_STATE_PR;   // pressed
    data->point.x = 240 - touchX;      // flip X if needed
    data->point.y = touchY;
  }
}

/* ================= Camera pixel format switch ================= */
void camera_switch_to_jpeg()
{
    sensor_t *s = esp_camera_sensor_get();
    s->set_pixformat(s, PIXFORMAT_JPEG);
}

void camera_switch_to_rgb()
{
    sensor_t *s = esp_camera_sensor_get();
    s->set_pixformat(s, PIXFORMAT_RGB565);
}

/* ================= Take photo and save to SD card ================= */
bool take_photo_and_save()
{
    camera_fb_t *fb = esp_camera_fb_get();  // Get current camera frame
    if (!fb) return false;

    // Fixed filename
    char filePath[32];                                
    strcpy(filePath, "/test.bmp");

    // Save RGB565 buffer to BMP on SD card
    ELECROW_SD::SD_Save_Result res = my_sd.save_rgb565_to_bmp(SD, filePath, (uint16_t *)g_camera_frame, 240, 280);

    return true;
}

/* ================= LVGL button click event ================= */
static void btn_take_photo_event(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    vTaskSuspend(camera_task_handle);  // Pause camera task to avoid frame conflicts

    camera_switch_to_jpeg();           // Switch camera to JPEG mode
    take_photo_and_save();             // Take photo and save
    camera_switch_to_rgb();            // Switch back to RGB mode for live preview

    vTaskResume(camera_task_handle);   // Resume camera task
}

/* ================= Create LVGL button ================= */
void create_take_photo_button()
{
    lv_obj_t *btn = lv_btn_create(lv_scr_act());       // Create button on active screen
    lv_obj_set_size(btn, 100, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn, btn_take_photo_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "photograph");
    lv_obj_center(label);
}

/* ================= LVGL timer callback to update camera image ================= */
void lvgl_camera_update_cb(lv_timer_t *timer) {
  if (!g_camera_frame_ready) return;  // Only update when new frame is ready

  img_dsc.header.always_zero = 0;
  img_dsc.header.w = 240;
  img_dsc.header.h = 284;
  img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;          // RGB565
  img_dsc.data_size = 240 * 284 * 2;                 // buffer size in bytes
  img_dsc.data = (uint8_t *)g_camera_frame;          // point to camera frame

  lv_img_set_src(camera_output, &img_dsc);          // Update LVGL image object

  g_camera_frame_ready = false;                     // Reset ready flag
}

/* ================= Camera task ================= */
void camera_task(void *pvParameters) {
  // Allocate PSRAM buffer for cropped camera frames
  static uint16_t *cropped_buf = (uint16_t *)heap_caps_malloc(LCD_H * LCD_W * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
  if (!cropped_buf) {
    vTaskDelete(NULL); // If allocation fails, delete task
  }

  while (1) {
    camera_fb_t *fb = esp_camera_fb_get();  // Get camera framebuffer
    if (!fb) continue;

    if (fb->format == PIXFORMAT_RGB565) {
      uint16_t *src = (uint16_t *)fb->buf;
      int x_offset = (400 - 240) / 2;        // Center crop horizontally
      float y_scale = 284.0f / 296.0f;       // Scale vertically

      // Crop and scale the image to LCD size
      for (int y = 0; y < 284; y++) {
        int src_y = (int)(y / y_scale);
        for (int x = 0; x < 240; x++) {
          int src_x = x + x_offset;
          uint16_t pixel = src[src_y * 400 + src_x];
          cropped_buf[y * 240 + x] = (pixel << 8) | (pixel >> 8); // RGB565 byte swap
        }
      }

      g_camera_frame = cropped_buf;      // Update global frame pointer
      g_camera_frame_ready = true;       // Set ready flag
    }

    esp_camera_fb_return(fb);            // Return framebuffer
    vTaskDelay(pdMS_TO_TICKS(30));       // ~30 FPS
  }
}

/* ================= LVGL task ================= */
void lvgl_task(void *pvParameters) {
  while (1) {
    lv_timer_handler();                  // Handle LVGL tasks
    vTaskDelay(pdMS_TO_TICKS(5));        // 5ms delay to yield CPU
  }
}

/* ================= Arduino setup ================= */
void setup() {
  Serial.begin(115200);

  // I2C initialization for AW9523
  wi->setPins(I2C_SDA, I2C_SCL);
  wi->begin();
  delay(100);

  // AW9523 init
  aw9523.AW_init();
  aw9523.AW_set_POWER(true);
  aw9523.AW_set_lcd_blight(100);  // Set LCD backlight brightness

  // LovyanGFX init
  gfx.init();
  gfx.initDMA();
  gfx.fillScreen(TFT_BLACK);

  // Camera init
  my_camera.camera_init(&config);
  my_camera.camera_sensor_init();

  // SD card init
  my_sd.SD_init();

  // LVGL init
  lv_init();
  size_t buffer_size = sizeof(lv_color_t) * LCD_W * LCD_H;
  static lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  static lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_W * LCD_H);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_W;
  disp_drv.ver_res = LCD_H;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // LVGL image object for camera preview
  camera_output = lv_img_create(lv_scr_act());
  lv_obj_set_width(camera_output, 240);
  lv_obj_set_height(camera_output, 284);
  lv_obj_set_align(camera_output, LV_ALIGN_CENTER);
  lv_obj_add_flag(camera_output, LV_OBJ_FLAG_ADV_HITTEST);     
  lv_obj_clear_flag(camera_output, LV_OBJ_FLAG_SCROLLABLE);      

  // Create timer to update LVGL image
  camera_timer = lv_timer_create(lvgl_camera_update_cb, 30, NULL);

  // Create "take photo" button
  create_take_photo_button();

  // Create FreeRTOS tasks for camera and LVGL
  xTaskCreatePinnedToCore(camera_task, "camera_task", 4096, NULL, 5, &camera_task_handle, 1);
  xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 8192, NULL, 8, NULL, 1);
}

/* ================= Arduino loop ================= */
void loop() {
  delay(5); // Main loop does nothing; LVGL and camera run in tasks
}
