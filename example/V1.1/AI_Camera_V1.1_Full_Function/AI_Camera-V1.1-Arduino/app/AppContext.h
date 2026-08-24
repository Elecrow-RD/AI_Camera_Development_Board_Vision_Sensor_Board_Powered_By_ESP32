#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Base64.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <string>

#include "src\audio.h"
#include "src\UI\ui.h"
#include "src\AICamera_V1.h"
#include "src\SD\elecrow_sd.h"
#include "src\WiFi\elecrow_wifi.h"
#include "src\AW9523\elecrow_aw9523.h"
#include "src\GFX\LovyanGFX_Driver.h"
#include "src\Camera\elecrow_camera.h"

#include "ai_vox_engine.h"
#include "ai_vox_observer.h"
#include "i2s_pdm_audio_input_device.h"
#include "i2s_std_audio_output_device.h"

SemaphoreHandle_t xGuiSemaphore = NULL;
SemaphoreHandle_t camera_mutex = NULL;
SemaphoreHandle_t sem_recognition = NULL;
SemaphoreHandle_t lvgl_mutex = NULL;

static TimerHandle_t brightness_timer = NULL;
static TimerHandle_t volume_timer = NULL;
/* LED Status */
bool led_status = false;
bool led_status_pre = false;

/*SD Service*/
static ELECROW_SD my_sd(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_NSS_PIN);
SemaphoreHandle_t sd_mutex;
bool current_wifi_connected = false;
bool WiFi_Long_Press_Reset = false;
uint8_t sd_state;
bool img_save_to_sd_flag = false;
bool change_screen_sd_card = false;
/* WiFi Server */

#define WIFI_INIT       0
#define WIFI_CONNECTED  1
#define WIFI_ONLINE_AI  2

uint8_t state = WIFI_INIT;
bool isAPMode = false;
bool isAudio = false;
static bool setting_ap = false;
WebServer server(80);
DNSServer dnsServer;
unsigned long wifiConnectStartTime = 0;
const unsigned long WIFI_TIMEOUT = 5000;  // 30秒超时
const char *ssidstr_c;

extern uint8_t RssiLev;

volatile bool AIRecognition_start_flag = false;  //AI Recognition start flag
bool AIRecognition_flag = false;
volatile bool g_show_recognition = false;

/*IIC Set*/
static TwoWire *wi = &Wire;
/* Task Handle */
TaskHandle_t RGB_Light_task_handle    = NULL;
TaskHandle_t video_stream_task_handle = NULL;
TaskHandle_t audio_task_handle        = NULL;
TaskHandle_t SD_Card_task_handle      = NULL;
TaskHandle_t lvgl_task_handle         = NULL;
TaskHandle_t WiFi_task_handle         = NULL;
TaskHandle_t led_task_handle          = NULL;
TaskHandle_t aichat_task_handle       = NULL;
/*Camera Service*/
static ELECROW_CAMERA my_camera;
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
  // .xclk_freq_hz=10000000,
  .xclk_freq_hz = 10000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_RGB565,
  .frame_size = FRAMESIZE_CIF,
  .jpeg_quality = 12,
  .fb_count = 2,
  .fb_location = CAMERA_FB_IN_PSRAM,
  .grab_mode = CAMERA_GRAB_LATEST,  //CAMERA_GRAB_WHEN_EMPTY,
};

static uint16_t *g_camera_frame = NULL;
static volatile bool g_camera_frame_ready = false;
uint8_t *camera_save_buf = NULL;
size_t saved_buf_len = 0;
/* LVGL */
lv_obj_t *camera_output = NULL;
static lv_img_dsc_t img_dsc;
static lv_timer_t *camera_timer = NULL;  //Camera Img Show in Screen timer
uint8_t screen_index = 0;

/* Server */
#define ACTIVATE_IDLE   0
#define ACTIVATE_START  1
#define ACTIVATE_END    2
bool device_active_check = false;
bool device_active = false;
uint8_t device_active_steup = ACTIVATE_IDLE;
uint8_t active_cont = 0;
bool device_check_end = false;
bool aichat_exit = false;
bool Touch;
bool end_active;
bool load_anim_flag = false;
String serial_number;
String activation_challenge;

#define TEXT_MAX_LEN 256
char g_recognition_text[TEXT_MAX_LEN];

/* SD Card */
bool SD_Card_info_updata = false;
String Percentages;

char g_sdcard_text[TEXT_MAX_LEN];

extern float percentage;
/* system info */
bool camera_is_no_init = true;
bool is_mute = false;
bool reset_to_factory = false;
bool reconfigure_btn = false;
Preferences preferences;
struct DeviceConfig {
  uint8_t brightness;
  uint8_t volume;
  bool restartFlag;
  String ssid;
  String password;
};
DeviceConfig g_cfg;
bool restartFlag = false;  //reconfigure WiFi

uint8_t brightness_value = 100;  //pre brightness  value
bool RGBLighCloseSem = false;

uint8_t speaker_value = 70;

uint8_t current_active_screen = 0;

uint8_t RGB_Light_index = 0;

bool Power_Off = false;

/* AI Chat */
bool aichat_led_close = false;
bool aichat_led_open = false;
bool aichat_RGB_Light_Open = false;
bool aichat_RGB_Light_Close = false;
bool screen_power = true;
uint8_t AIChat_Device_Bound_Check = 0;
uint8_t aichat_steup = 1;
bool get_active_code = false;

static std::string g_pending_text;
static volatile bool g_text_ready = false;

typedef enum {
    AIChat_Idle = 0,
    AIChat_Initing,
    AIChat_Standby,
    AIChat_Connecting,
    AIChat_Listening,
    AIChat_Speaking,
    AIChat_Other,
} AIChat_status;

struct Emotion {
    std::string name;   // AI 返回的字符串
    uint8_t id;         // 下标 / 编号
};

Emotion emotion_table[] = {
    {"neutral", 0}, {"happy", 1}, {"laughing", 2}, {"funny", 3},
    {"sad", 4}, {"angry", 5}, {"crying", 6}, {"loving", 7},
    {"embarrassed", 8}, {"surprised", 9}, {"shocked", 10}, {"thinking", 11},
    {"winking", 12}, {"cool", 13}, {"relaxed", 14}, {"delicious", 15},
    {"kissy", 16}, {"confident", 17}, {"sleepy", 18}, {"silly", 19},
    {"confused", 20}
};
constexpr size_t EMOTION_COUNT = sizeof(emotion_table)/sizeof(emotion_table[0]);
int emotion_index = 0;
int emotion_index_pre = -1;

int lastButtonState = HIGH;
volatile bool buttonPressed = false;
bool AI_Recognition_flag = false;
bool current_fun_is_Recognition = false;
bool AI_Recognition_exit_flag = false;

AIChat_status aichat_status = AIChat_Other;
namespace {
constexpr gpio_num_t kMicPdmClk = GPIO_NUM_1;
constexpr gpio_num_t kMicPdmDin = GPIO_NUM_2;

constexpr gpio_num_t kSpeakerPinBclk = GPIO_NUM_5;
constexpr gpio_num_t kSpeakerPinWs = GPIO_NUM_4;
constexpr gpio_num_t kSpeakerPinDout = GPIO_NUM_3;

constexpr gpio_num_t kTriggerPin = GPIO_NUM_0;

auto g_observer = std::make_shared<ai_vox::Observer>();
std::shared_ptr<ai_vox::iot::Entity> g_rgb_iot_entity;
std::shared_ptr<ai_vox::iot::Entity> g_screen_iot_entity;
std::shared_ptr<ai_vox::iot::Entity> g_speaker_iot_entity;
std::shared_ptr<ai_vox::iot::Entity> g_led_iot_entity;
auto g_audio_output_device = std::make_shared<ai_vox::I2sStdAudioOutputDevice>(kSpeakerPinBclk, kSpeakerPinWs, kSpeakerPinDout);
auto audio_input_device = std::make_shared<ai_vox::I2sPdmAudioInputDevice>(kMicPdmClk, kMicPdmDin);
