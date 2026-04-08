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

#define WIFI_INIT 0
#define WIFI_CONNECTED 1
#define WIFI_ONLINE_AI 2

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
#define ACTIVATE_START 1
#define ACTIVATE_IDLE 0
#define ACTIVATE_END 2
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
void InitIot() {

  auto& ai_vox_engine = ai_vox::Engine::GetInstance();

  // Speaker
  // 1.Define the properties for the speaker entity
    std::vector<ai_vox::iot::Property> speaker_properties({
        {
            "volume",                         
            "Current volume level",          
            ai_vox::iot::ValueType::kNumber   
        },
    });

    // 2. Define the speaker's function
    std::vector<ai_vox::iot::Function> speaker_functions({
        {"SetVolume",                                     
        "Set the volume of the device",                
        {
            {
                "volume",                                 
                "Integer value between 0 and 100",       
                ai_vox::iot::ValueType::kNumber,          
                true                                      
            },

        }},
    });

    // 3. Create speaker entity object
    g_speaker_iot_entity = std::make_shared<ai_vox::iot::Entity>(
        "Speaker",                          
        "A smart speaker device",           
        std::move(speaker_properties),      
        std::move(speaker_functions)        
    );

    // 4. Initialize default state values
    g_speaker_iot_entity->UpdateState("volume", g_audio_output_device->volume());

    // 5. Register to AI Vox Engine
    ai_vox_engine.RegisterIotEntity(g_speaker_iot_entity);

  // LED
  // 1.Define the properties for the LED entity
    std::vector<ai_vox::iot::Property> rgb_properties({
        {
            "state",                          // attribute name
            "RGB light switch state (true=on, false=off)", // describe
            ai_vox::iot::ValueType::kBool     // type (on/off)
        },
    });

    // 2. Define the properties for RGB Light functions
    std::vector<ai_vox::iot::Function> rgb_functions({
        {"TurnOn",                         
        "Turn on the RGB light",             
        {
            {
                "delay",                     
                "Delay time in seconds (optional)", 
                ai_vox::iot::ValueType::kNumber, 
                false                        
            }
        }},
        {"TurnOff",                          
        "Turn off the RGB light",         
        {
            {
                "delay",                  
                "Delay time in seconds (optional)", 
                ai_vox::iot::ValueType::kNumber,
                false
            }
        }}
    });

    g_rgb_iot_entity = std::make_shared<ai_vox::iot::Entity>(
        "RGB",                              
        "RGB lighting controller",          
        std::move(rgb_properties),          
        std::move(rgb_functions)            
    );


    g_rgb_iot_entity->UpdateState("state", false);


    ai_vox_engine.RegisterIotEntity(g_rgb_iot_entity);

  // Screen
  // 1.Define the properties for the screen entity
    std::vector<ai_vox::iot::Property> screen_properties({
        {
            "power",                           // 
            "Screen power status (on/off)",    // 
            ai_vox::iot::ValueType::kBool      // 
        },
        {
            "brightness",                      //
            "Current brightness percentage",   //
            ai_vox::iot::ValueType::kNumber    // 
        }
    });


    std::vector<ai_vox::iot::Function> screen_functions({
        {"TurnOn",
        "Turn on the screen display",
        {{
            "delay",
            "Optional delay in seconds before turning on",
            ai_vox::iot::ValueType::kNumber,
            false
        }}},
        {"TurnOff",
        "Turn off the screen display",
        {{
            "delay",
            "Optional delay in seconds before turning off",
            ai_vox::iot::ValueType::kNumber,
            false
        }}},
        {"SetBrightness",
        "Adjust screen brightness level",
        {{
            "brightness",
            "Integer from 1 to 100 representing brightness percent",
            ai_vox::iot::ValueType::kNumber,
            true
        }}}
    });


    g_screen_iot_entity = std::make_shared<ai_vox::iot::Entity>(
        "Screen",
        "Smart screen that supports power on/off and adjustable brightness",
        std::move(screen_properties),
        std::move(screen_functions)
    );


    g_screen_iot_entity->UpdateState("power", true); 
    g_screen_iot_entity->UpdateState("brightness", brightness_value);


    ai_vox_engine.RegisterIotEntity(g_screen_iot_entity);
    // LED

    std::vector<ai_vox::iot::Property> led_properties({
        {
            "state",                           // 
            "light switch state (on/off)", // 
            ai_vox::iot::ValueType::kBool
        }
    });

    // 2. 
    std::vector<ai_vox::iot::Function> led_functions({
        {"TurnOn",
        "Turn the  light on",
        {{
            "delay",
            "Optional delay in seconds",
            ai_vox::iot::ValueType::kNumber,
            false
        }}},
        {"TurnOff",
        "Turn the light off",
        {{
            "delay",
            "Optional delay in seconds",
            ai_vox::iot::ValueType::kNumber,
            false
        }}}
    });

    // 3. 
    g_led_iot_entity = std::make_shared<ai_vox::iot::Entity>(
        "Led",
        "General LED control interface",
        std::move(led_properties),
        std::move(led_functions)
    );

    // 4.
    g_led_iot_entity->UpdateState("state", false);

    // 5. 
    ai_vox_engine.RegisterIotEntity(g_led_iot_entity);
}
}

String getSerialNumber() {
  uint64_t chipid = ESP.getEfuseMac();
  char serial[32];
  sprintf(serial, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  return String(serial);
}

String GetMacAddress() {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);

  char mac_str[18];
  sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return String(mac_str);
}

String GetActivationPayload() {
  if (serial_number.length() == 0) {
    return "{}";
  }

  StaticJsonDocument<256> doc;
  doc["algorithm"] = "hmac-sha256";
  doc["serial_number"] = serial_number.c_str();
  doc["challenge"] = activation_challenge.c_str();
  doc["hmac"] = "";

  String json_str;
  serializeJson(doc, json_str);
  return json_str;
}

String GenerateUUIDSimple()
{
    uint8_t mac[6];
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) != ESP_OK) {
        return "00000000-0000-0000-0000-000000000000";
    }

    char uuid[37]; 

    snprintf(uuid, sizeof(uuid),
             "%02X%02X%02X%02X-0000-1000-8000-%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3],
             mac[4], mac[5],
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return String(uuid);
}

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


void setup(void) {
  Serial.begin(115200);
  while (!Serial);

  pinMode(0, INPUT_PULLUP);
  /*I2C Init*/
  //wi->setPins(I2C_SDA, I2C_SCL);
  wi->begin(I2C_SDA, I2C_SCL,100000U);
  delay(200);
  /* extern chip init*/
  aw9523.AW_init();
  aw9523.AW_set_POWER(true);
  delay(100);
  aw9523.AW_set_MUTE(true);
  delay(100);
  aw9523.AW_set_MUTE(false);
  delay(100);
  aw9523.AW_set_MUTE(true);
  delay(100);
  aw9523.AW_set_lcd_blight(0);
  lvgl_mutex = xSemaphoreCreateMutex();
  if (lvgl_mutex == NULL) {

  }
  /*LCD Init*/
  gfx.init();
  gfx.initDMA();
  gfx.startWrite();
  gfx.fillScreen(TFT_BLACK);

  // my_camera.camera_init(&config);
  // my_camera.camera_sensor_init();

  lv_init();
  size_t buffer_size = sizeof(lv_color_t) * screenWidth * screenHeight;
  static lv_color_t *buf = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  static lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  lv_disp_draw_buf_init(&draw_buf, buf, buf1, screenWidth * screenHeight);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  loadPreferences(g_cfg);

  if (g_cfg.ssid.isEmpty()) {
    restartFlag = true;
  } else {
    restartFlag = false;
  }

  camera_save_buf = (uint8_t *)heap_caps_malloc(236800, MALLOC_CAP_SPIRAM);

  /* cmaera SemaphoreHandle */
  camera_mutex = xSemaphoreCreateMutex();
  /* AI Recogition Result sem */
  sem_recognition = xSemaphoreCreateBinary();
  /* SD Card Task mutex */
  sd_mutex = xSemaphoreCreateMutex(); 
  /* LVGL & Camera timer*/
  camera_timer = lv_timer_create(lvgl_camera_update_cb, 30, NULL);

  InitIot();
  auto& ai_vox_engine = ai_vox::Engine::GetInstance();
  ai_vox_engine.SetObserver(g_observer);
  ai_vox_engine.SetTrigger(kTriggerPin);
  ai_vox_engine.SetOtaUrl("https://api.thinknode.cc/ota/");
  ai_vox_engine.ConfigWebsocket("wss://otc.thinknode.cc/v1/",
                                {
                                    {"Authorization", "Bearer test-token"},
                                });

  xTaskCreatePinnedToCore(RGB_Light_task, "RGB_Light_handle", 1024, NULL, 1, &RGB_Light_task_handle, 1);
  //xTaskCreatePinnedToCore(video_task, "video_stream", 4096, NULL, 6, &video_stream_task_handle, 1);
  xTaskCreatePinnedToCore(audio_task, "audio_task_handle", 2048, NULL, 4, &audio_task_handle, 1);
  xTaskCreatePinnedToCore(SD_Card_task, "SD_Card_handle", 2048, NULL, 3, &SD_Card_task_handle, 1);
  xTaskCreatePinnedToCore(WiFi_task, "wifi_handle", 8192, NULL, 1, &WiFi_task_handle, 1);
  xTaskCreatePinnedToCore(aichat_task, "aichat_task_handle", 8192, NULL, 6, &aichat_task_handle, 1);
  ui_init();

  g_cfg.brightness = brightness_value;

  g_audio_output_device->SetVolume(speaker_value);
  g_speaker_iot_entity->UpdateState("volume", speaker_value);
  is_mute = true;

  
  aw9523.AW_set_lcd_blight(g_cfg.brightness);
}

String activationCode = "";
String challengeCode = "";
String GetActivationCode() {
  String deviceId = GetMacAddress();

  String url = "https://api.thinknode.cc/ota/";

  HTTPClient http;
  http.begin(url);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Device-Id", deviceId);

  DynamicJsonDocument doc(256);
  JsonObject application = doc.createNestedObject("application");
  JsonObject board = doc.createNestedObject("board");
  //board["type"] = "AICamera";
  String requestBody;
  serializeJson(doc, requestBody);

  int httpCode = http.POST(requestBody);
  String response = "";

  if (httpCode > 0) {
    response = http.getString();
  } else {
    http.end();
    return "";
  }

  http.end();

  DynamicJsonDocument jsonDoc(2048);  
  DeserializationError error = deserializeJson(jsonDoc, response);

  if (error) {
    return "";
  }

  if (jsonDoc.containsKey("activation") && jsonDoc["activation"].containsKey("code")) {

    activationCode = jsonDoc["activation"]["code"].as<String>();
    challengeCode = jsonDoc["activation"]["challenge"].as<String>();

    get_active_code  = true;
    return activationCode;
  }

  return "";
}


bool IsDeviceActivated() {
  if (device_active) {

    return true;
  }

  serial_number = getSerialNumber();

  String device_id = GetMacAddress();
  String Client_id = GenerateUUIDSimple();
  static char url[256]; 
  if(current_active_screen == 1)
  {
      strncpy(url, "https://api.thinknode.cc/ota/activate", sizeof(url) - 1);
  }
  else if(current_active_screen == 3)
  {
      strncpy(url, "https://api.thinknode.cc/ota/activate", sizeof(url) - 1);
  }
  else if(current_active_screen == 2)
  {
      strncpy(url, "https://api.tenclass.net/xiaozhi/ota/activate", sizeof(url) - 1);
                    
  }
  url[sizeof(url) - 1] = '\0';

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, url);

  String payload = GetActivationPayload();

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Device-Id", device_id);
  http.addHeader("Client-Id", Client_id);

  int status_code = http.POST("{}");
  String resp = http.getString();

  if (status_code == 202) {
    if (device_active_steup == ACTIVATE_IDLE) {
      device_active_steup = ACTIVATE_START;
    }

    http.end();
    return false;
  } else if (status_code == 200) {
    device_active = true;

    http.end();
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    return true;
  }

  http.end();
  return false;
}

uint8_t *save_camera_frame(const camera_fb_t *src_fb) {
  if (!src_fb || !src_fb->buf || src_fb->len == 0) {

    return NULL;
  }

  memcpy(camera_save_buf, src_fb->buf, src_fb->len);
  saved_buf_len = src_fb->len;
  return camera_save_buf;
}

void aichat_task(void *pvParameters)
{
    vTaskSuspend(aichat_task_handle);
    while(1)
    {
      switch(aichat_steup)
      {
        
          case 1:
          {
              auto& ai_vox_engine = ai_vox::Engine::GetInstance();
              if(current_active_screen == 2)
              {
                  ai_vox_engine.SetOtaUrl("https://api.tenclass.net/xiaozhi/ota/");
                  ai_vox_engine.ConfigWebsocket("wss://api.tenclass.net/xiaozhi/v1/",
                                                {
                                                    {"Authorization", "Bearer test-token"},
                                                });
              }
              else if(current_active_screen == 1)
              {
                  ai_vox_engine.SetOtaUrl("https://api.thinknode.cc/ota/");
                  ai_vox_engine.ConfigWebsocket("wss://otc.thinknode.cc/v1/",
                                                {
                                                    {"Authorization", "Bearer test-token"},
                                                });
              }
              ai_vox_engine.Start(audio_input_device, g_audio_output_device);
              aichat_steup = 2;
              break;
          }

          case 2:
          {
              const auto events = g_observer->PopEvents();
                for (auto& event : events) {
                  if (auto activation_event = std::get_if<ai_vox::Observer::ActivationEvent>(&event)) {
                    activationCode = activation_event->code.c_str();
                    get_active_code  = true;
                    printf("activation code: %s, message: %s\n", activation_event->code.c_str(), activation_event->message.c_str());
                  } else if (auto state_changed_event = std::get_if<ai_vox::Observer::StateChangedEvent>(&event)) {
                    //printf("state changed from %" PRIu8 " to %" PRIu8 "\n",
                          // static_cast<uint8_t>(state_changed_event->old_state),
                          // static_cast<uint8_t>(state_changed_event->new_state));
                    switch (state_changed_event->new_state) {
                      case ai_vox::ChatState::kIdle: {
                        printf("Idle\n");
                        aichat_status = AIChat_Idle;
                        break;
                      }
                      case ai_vox::ChatState::kIniting: {
                        printf("Initing...\n");
                        aichat_status = AIChat_Initing;
                        break;
                      }
                      case ai_vox::ChatState::kStandby: {
                        printf("Standby\n");
                        device_active = true;
                        aichat_status = AIChat_Standby;
                        break;
                      }
                      case ai_vox::ChatState::kConnecting: {
                        printf("Connecting...\n");
                        aichat_status = AIChat_Connecting;
                        break;
                      }
                      case ai_vox::ChatState::kListening: {
                        printf("Listening...\n");
                        aichat_status = AIChat_Listening;
                        break;
                      }
                      case ai_vox::ChatState::kSpeaking: {
                        printf("Speaking...\n");

                        aichat_status = AIChat_Speaking;
                        break;
                      }
                      default: {
                        break;
                      }
                    }
                  } else if (auto emotion_event = std::get_if<ai_vox::Observer::EmotionEvent>(&event)) {
                    printf("emotion: %s\n", emotion_event->emotion.c_str());
                    std::string emotion_name = emotion_event->emotion;
                    emotion_index = get_emotion_index(emotion_name); 
                  } else if (auto chat_message_event = std::get_if<ai_vox::Observer::ChatMessageEvent>(&event)) {
                    switch (chat_message_event->role) {
                      case ai_vox::ChatRole::kAssistant: {
                        printf("role: assistant, content: %s\n", chat_message_event->content.c_str());
                        on_chat_message(chat_message_event->content); 
                        break;
                      }
                      case ai_vox::ChatRole::kUser: {
                        printf("role: user, content: %s\n", chat_message_event->content.c_str());
                        on_chat_message(chat_message_event->content); 
                        break;
                      }
                    }
                  } else if (auto iot_message_event = std::get_if<ai_vox::Observer::IotMessageEvent>(&event)) {
                    printf("IOT message: %s, function: %s\n", iot_message_event->name.c_str(), iot_message_event->function.c_str());
                    for (const auto& [key, value] : iot_message_event->parameters) {
                      if (std::get_if<bool>(&value)) {
                        printf("key: %s, value: %s\n", key.c_str(), std::get<bool>(value) ? "true" : "false");
                      } else if (std::get_if<std::string>(&value)) {
                        printf("key: %s, value: %s\n", key.c_str(), std::get<std::string>(value).c_str());
                      } else if (std::get_if<int64_t>(&value)) {
                        printf("key: %s, value: %lld\n", key.c_str(), std::get<int64_t>(value));
                      }
                    }

                    if (iot_message_event->name == "Led") {
                      if (iot_message_event->function == "TurnOn") {
                        led_status = true;
                        aichat_led_open = true;
                        g_rgb_iot_entity->UpdateState("state", true);  // Note: Must UpdateState after change the device state
                      } else if (iot_message_event->function == "TurnOff") {
                        led_status = false;
                        aichat_led_close = true;
                        g_rgb_iot_entity->UpdateState("state", false);  // Note: Must UpdateState after change the device state
                      }
                    }
                    else if (iot_message_event->name == "RGB") {
                      if (iot_message_event->function == "TurnOn") {
                        aichat_RGB_Light_Open = true;
                        g_rgb_iot_entity->UpdateState("state", true);  // Note: Must UpdateState after change the device state
                      } else if (iot_message_event->function == "TurnOff") {
                        aichat_RGB_Light_Close = true;

                        g_rgb_iot_entity->UpdateState("state", false);  // Note: Must UpdateState after change the device state
                      }
                    }
                    else if (iot_message_event->name == "Speaker") {
                      if (iot_message_event->function == "SetVolume") {
                        if (const auto it = iot_message_event->parameters.find("volume"); it != iot_message_event->parameters.end()) {
                          auto volume = it->second;
                          if (std::get_if<int64_t>(&volume)) {

                            g_audio_output_device->SetVolume(std::get<int64_t>(volume));

                            g_speaker_iot_entity->UpdateState("volume", std::get<int64_t>(volume));  // Note: Must UpdateState after change the device state
                          }
                        }
                      }
                    }
                    else if (iot_message_event->name == "Screen") {
                      if (iot_message_event->function == "TurnOn") {
                        screen_power = true;
                        aw9523.AW_set_lcd_blight(g_cfg.brightness);
                        g_screen_iot_entity->UpdateState("power", true);  // Note: Must UpdateState after change the device state
                      } else if (iot_message_event->function == "TurnOff") {
                        screen_power = false;
                        aw9523.AW_set_lcd_blight(0);
                        g_screen_iot_entity->UpdateState("power", false);  // Note: Must UpdateState after change the device state
                      }else if (iot_message_event->function == "SetBrightness") {
                        if (const auto it = iot_message_event->parameters.find("brightness"); it != iot_message_event->parameters.end()) {
                          auto brightness = it->second;
                          if (std::get_if<int64_t>(&brightness)) {
                            uint8_t temp_value = std::get<int64_t>(brightness);
                            brightness_value = (temp_value < 1) ? 1 : (temp_value > 100 ? 100 : temp_value);
                            g_screen_iot_entity->UpdateState("brightness", std::get<int64_t>(brightness));  // Note: Must UpdateState after change the device state
                          }
                        }
                      }
                    }
                    
                    
                  }
                }
              IsDeviceActivated();
              break;
          }
      }
      
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    
}

bool result_save_flag = false;
void AI_Recognition_func() {
  camera_fb_t fake_fb;
  uint8_t *jpg_buf = NULL;
  size_t jpg_size = 0;

  fake_fb.buf = camera_save_buf;
  fake_fb.len = saved_buf_len;
  fake_fb.width = 400;
  fake_fb.height = 296;
  fake_fb.format = PIXFORMAT_RGB565;

  vTaskSuspend(video_stream_task_handle);

  if (!frame2jpg(&fake_fb, 80, &jpg_buf, &jpg_size)) {

    return;
  }
  String imageBase64 = base64::encode(jpg_buf, jpg_size);
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure();

  String deviceAddr = GetMacAddress();
  deviceAddr.toLowerCase();

  const String url = "https://service.thinknode.cc/api/users/image-analysis-for-eps32-camera";
  const String token = "3696b69a9aa3b624cac9f691797be9e9";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + token);

  String jsonRequest;
  jsonRequest += "{";
  jsonRequest += "\"role\": \"You are an object recognition assistant. Only return the names of the objects in the picture, separated by commas. Do not add any explanations or extra text.\",";
  jsonRequest += "\"image\": \"data:image/jpeg;base64," + imageBase64 + "\",";
  jsonRequest += "\"mac_address\": \"" + deviceAddr + "\"";
  jsonRequest += "}";

  int httpResponseCode = http.POST(jsonRequest);

  String response = http.getString();

  if (httpResponseCode == 200) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, response);

    if (error) {

      return;
    }

    if (doc.containsKey("code")) {
      int code = doc["code"];

      if (code == 200) {

        if (doc.containsKey("data") && doc["data"].containsKey("response")) {
          const char *content = doc["data"]["response"];

          strncpy(g_recognition_text,
                  content,
                  TEXT_MAX_LEN - 1);

          g_recognition_text[TEXT_MAX_LEN - 1] = '\0';

          if(lv_scr_act() == ui_AIRecognition)
          {
              g_show_recognition = true;
              if (sem_recognition != NULL) {
                xSemaphoreGive(sem_recognition);
              }
          }
        }

      } else if (code == 300) {
          strncpy(g_recognition_text, "the device is not bound", TEXT_MAX_LEN - 1);
          g_recognition_text[TEXT_MAX_LEN - 1] = '\0';
          g_show_recognition = true;
          if (sem_recognition != NULL) {
              xSemaphoreGive(sem_recognition);
          }
      } else if (code == 400) {
        if (doc.containsKey("msg")) {
          const char *msg = doc["msg"];
          g_show_recognition = true;
          strncpy(g_recognition_text,
                  msg,
                  TEXT_MAX_LEN - 1);
          if (sem_recognition != NULL) {
            xSemaphoreGive(sem_recognition);
          }
        }
      }
    } else {
    }
  } else {
    String msg = "HTTP Error: " + String(httpResponseCode);

    snprintf(g_recognition_text, TEXT_MAX_LEN, "The network is poor", httpResponseCode);

    g_recognition_text[TEXT_MAX_LEN - 1] = '\0';

    if(lv_scr_act() == ui_AIRecognition)
    {
        g_show_recognition = true;
        if (sem_recognition != NULL) {
          xSemaphoreGive(sem_recognition);
        }
    }
  }

  http.end();
  free(jpg_buf);
}

void RGB_Light_task(void *pvParameters) {
  static uint8_t RGB_index;
  vTaskSuspend(RGB_Light_task_handle);
  while (1) {
    switch (RGB_index) {
      case 0: RGB_Light_index = 1; break;
      case 1: RGB_Light_index = 2; break;
      case 2: RGB_Light_index = 3; break;
    }
    RGB_index = (RGB_index + 1) % 3;  
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

  void video_task(void *pvParameters) {
    camera_fb_t *fb = NULL;

    static uint16_t *cropped_buf = (uint16_t *)heap_caps_malloc(screenWidth * screenHeight * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!cropped_buf) {
      vTaskDelete(NULL);
    }

    vTaskSuspend(video_stream_task_handle);

    while (1) {
      if (xSemaphoreTake(camera_mutex, portMAX_DELAY)) {
        fb = esp_camera_fb_get();

        if (!fb) continue;

        camera_save_buf = save_camera_frame(fb);

        if (fb->format == PIXFORMAT_RGB565) {
          uint16_t *src = (uint16_t *)fb->buf;
          int x_offset = (400 - 240) / 2;
          float y_scale = 284.0f / 296.0f;

          for (int y = 0; y < 284; y++) {
            int src_y = (int)(y / y_scale);
            for (int x = 0; x < 240; x++) {
              int src_x = x + x_offset;
              uint16_t pixel = src[src_y * 400 + src_x];
              cropped_buf[y * 240 + x] = (pixel << 8) | (pixel >> 8);
            }
          }

          g_camera_frame = cropped_buf;
          g_camera_frame_ready = true;
        }
        esp_camera_fb_return(fb);
        xSemaphoreGive(camera_mutex);
      }
      vTaskDelay(pdMS_TO_TICKS(30));
    }
  }

void audio_task(void *pvParameters) {
  while(1)
  {
    
    if(is_mute)
    {
      is_mute = false;
      vTaskDelay(50);
      aw9523.AW_set_MUTE(false);
      vTaskDelay(100);
    }

    size_t size = sizeof(Audio) / sizeof(Audio[0]);
    uint8_t *wav_buffer = (uint8_t*)heap_caps_malloc(size-44, MALLOC_CAP_SPIRAM);
    if (!wav_buffer) {
        return ;
    }

    memcpy(wav_buffer, Audio + 44, size - 44);

    size_t sample_count = (size - 44) / 2; 
    std::vector<int16_t> i2s_buffer(sample_count * 2); 

    for (size_t i = 0; i < sample_count; i++) {
        int16_t sample = (wav_buffer[i*2+1] << 8) | wav_buffer[i*2]; 
        sample = static_cast<int16_t>(sample * 0.1); 

        i2s_buffer[i] = sample;
    }

    free(wav_buffer);

    if(g_audio_output_device->Open(16000))
    {
      g_audio_output_device->Write(i2s_buffer.data(), i2s_buffer.size());
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    i2s_buffer.clear();         
    g_audio_output_device->SetVolume(speaker_value);
    vTaskSuspend(NULL);
  }
}

void SD_Card_task(void *pvParameters) {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      

    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(0))) {
        change_screen_sd_card = true;
        my_sd.SD_End();
        bool sd_init_result = my_sd.SD_init();
        if (sd_init_result) {
          
          vTaskDelay(pdMS_TO_TICKS(200));
          my_sd.SD_test();
          Percentages = String(percentage, 0) + "% Remaining";
          strncpy(g_sdcard_text,
                  Percentages.c_str(),
                  TEXT_MAX_LEN - 1);

          g_sdcard_text[TEXT_MAX_LEN - 1] = '\0';
          SD_Card_info_updata = true;

        } else {
          strncpy(g_sdcard_text,
                  "No Memory Card",
                  TEXT_MAX_LEN - 1);
          g_sdcard_text[TEXT_MAX_LEN - 1] = '\0';
          SD_Card_info_updata = true;
        }

      xSemaphoreGive(sd_mutex);
    }

  }
}

void WiFi_task(void *pvParameters) {

  while (1) {
    if (AIRecognition_flag == true) {
      AIRecognition_flag = false;
      state = WIFI_ONLINE_AI;
    }
    switch (state) {
      case WIFI_INIT:
        current_wifi_connected = false;
        connectToWiFi();
        state = WIFI_CONNECTED;
        setting_ap = false;
        break;

      case WIFI_CONNECTED:
        if (WiFi_Long_Press_Reset) {
          WiFi_Long_Press_Reset = false;
          current_wifi_connected = false;
          setting_ap = true;
        }

        if (!setting_ap) {
          checkWiFiConnection();
        }
        break;
      case WIFI_ONLINE_AI:
          load_anim_flag = true;
          AI_Recognition_func();

          state = WIFI_CONNECTED;
        break;
      default:
        state = WIFI_INIT;
        break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
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

    if(g_cfg.volume >= 66)
    {
        lv_obj_add_flag(ui_AIChatVolumImg0, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_add_flag(ui_AIChatVolumImg1, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_add_flag(ui_AIChatVolumImg2, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_clear_flag(ui_AIChatVolumImg3, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
    }
    else if (g_cfg.volume <= 66 && g_cfg.volume >= 33)
    {
        lv_obj_add_flag(ui_AIChatVolumImg0, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_add_flag(ui_AIChatVolumImg1, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_add_flag(ui_AIChatVolumImg3, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_clear_flag(ui_AIChatVolumImg2, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
    }
    else if (g_cfg.volume > 0 && g_cfg.volume < 33)
    {
        lv_obj_add_flag(ui_AIChatVolumImg0, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_add_flag(ui_AIChatVolumImg2, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_add_flag(ui_AIChatVolumImg3, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_clear_flag(ui_AIChatVolumImg1, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
    }
    else
    {
        lv_obj_add_flag(ui_AIChatVolumImg1, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_add_flag(ui_AIChatVolumImg2, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_add_flag(ui_AIChatVolumImg3, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
        lv_obj_clear_flag(ui_AIChatVolumImg0, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST); 
    }

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

    if (ui_SpeakerSlider != nullptr) {
        lv_bar_set_value(ui_SpeakerSlider, speaker_val, LV_ANIM_ON);

        lv_label_set_text_fmt(ui_speakerLab, "%d%%", speaker_val);

    }
  }

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
      aw9523.AW_set_MUTE(true);
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
void loop() {

  lv_timer_handler(); 
    

  lv_wifi_rssi_show();
  system_set();
  AIChat_Status_lvgl();
  device_active_check_steup_handle();

  if (In_AIRecognition_Save_Src) {
    if (g_show_recognition) {
      g_show_recognition = false;
      lv_obj_add_flag(ui_AIRecognitionSpinner, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_arc_opa(ui_AIRecognitionSpinner, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
      lv_obj_set_style_arc_opa(ui_AIRecognitionSpinner, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
      _ui_screen_change(&ui_AIRecognitionSave, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_AIRecognitionSave_screen_init);
      camera_output = ui_AIRecognitionSaveImg;
      lv_img_set_src(camera_output, &img_dsc);
      if (xSemaphoreTake(sem_recognition, 0) == pdTRUE) {

        lv_label_set_text(ui_AIRecognitionResultLab, g_recognition_text);
        lv_obj_clear_flag(ui_AIRecognitionResultLab, LV_OBJ_FLAG_HIDDEN);
        vTaskResume(audio_task_handle);
      }
    }
    if (img_save_to_sd_flag) {
      img_save_to_sd_flag = false;
      my_sd.SD_End();
      bool sd_init_result = my_sd.SD_init();
      lv_timer_t * timer = lv_timer_create(hide_sd_label_cb, 2100, ui_AIRecognitionSaveSDLab);
      lv_timer_set_repeat_count(timer, 1);
      if (sd_init_result == false) {
        lv_label_set_text(ui_AIRecognitionSaveSDLab, "SD card does not exist");
        lv_obj_clear_flag(ui_AIRecognitionSaveSDLab, LV_OBJ_FLAG_HIDDEN);
        return;
      }
      static int file_idx = 0;
      char filePath[32];                                 // 声明一个字符数组
      sprintf(filePath, "/capture_%d.bmp", file_idx++);  // 填充内容
      ELECROW_SD::SD_Save_Result res = my_sd.save_rgb565_to_bmp(SD, filePath, (uint16_t *)g_camera_frame, 240, 280);

      if (res != ELECROW_SD::SAVE_OK) {
        
      } else {
        // 成功提示
        lv_label_set_text(ui_AIRecognitionSaveSDLab, "Image saved to SD card");
        lv_obj_clear_flag(ui_AIRecognitionSaveSDLab, LV_OBJ_FLAG_HIDDEN);
      }

    }
  }

  if(AIChat_Device_Bound_Check == 1)
  {
      vTaskDelay(pdMS_TO_TICKS(300));
      if(ui_ServerConnectSrc != NULL)
      {
        lv_obj_add_flag(ui_Label4, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_ServerConnectSpinner, LV_OBJ_FLAG_HIDDEN);
      }
      _ui_screen_change(&ui_ServerConnectSrc, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_ServerConnectSrc_screen_init);
      //lv_label_set_text(ui_Label4, "Opening HTTP Connection\nto \"https://api.thinknode.cc/\nota/\"");
      device_active_check = true;
      device_active = false;
      current_active_screen = 1;
      aichat_steup = 1;
      if(is_mute == true)
      {
        aw9523.AW_set_MUTE(false);
        is_mute = false;
      }
      AIChat_Device_Bound_Check = 0;

  }
  else if (AIChat_Device_Bound_Check == 2)
  {
      vTaskDelay(pdMS_TO_TICKS(300));
      if(ui_ServerConnectSrc != NULL)
      {
        lv_obj_add_flag(ui_Label4, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_ServerConnectSpinner, LV_OBJ_FLAG_HIDDEN);
      }
      _ui_screen_change(&ui_ServerConnectSrc, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_ServerConnectSrc_screen_init);
      //lv_label_set_text(ui_Label4, "Opening HTTP Connection\nto \"https://xiaozhi.me/\nota/\"");
      device_active_check = true;
      device_active = false;
      current_active_screen = 2;
      aichat_steup = 1;
      if(is_mute == true)
      {
        aw9523.AW_set_MUTE(false);
        is_mute = false;
      }
      AIChat_Device_Bound_Check = 0;
  }
  else if (AIChat_Device_Bound_Check == 3)
  {   
      if(camera_is_no_init)
      {
          vTaskDelay(pdMS_TO_TICKS(100));
          camera_is_no_init = false;
          my_camera.camera_init(&config);
          my_camera.camera_sensor_init();
          xTaskCreatePinnedToCore(video_task, "video_stream", 4096, NULL, 6, &video_stream_task_handle, 1);
      }
      if(ui_AIRecognitionSpinner != NULL)
      {
          lv_obj_add_flag(ui_AIRecognitionSpinner, LV_OBJ_FLAG_HIDDEN);
      }
      //
      vTaskResume(video_stream_task_handle);
      camera_output = NULL;
      In_AIRecognition_Save_Src = false;
      lv_obj_add_flag(ui_AIRecognitionSaveSDLab, LV_OBJ_FLAG_HIDDEN);
      vTaskDelay(pdMS_TO_TICKS(300));
      if(ui_ServerConnectSrc != NULL)
      {
        lv_obj_add_flag(ui_Label4, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_ServerConnectSpinner, LV_OBJ_FLAG_HIDDEN);
      }

      _ui_screen_change(&ui_ServerConnectSrc, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_ServerConnectSrc_screen_init);
      //lv_label_set_text(ui_Label4, "Opening HTTP Connection\nto \"https://api.thinknode.cc/\nota/\"");
      device_active_check = true;
      device_active = false;
      aichat_steup = 1;
      current_active_screen = 3;
      if(is_mute == true)
      {
        aw9523.AW_set_MUTE(false);
        is_mute = false;
      }
      AIChat_Device_Bound_Check = 0;
    }

    if(aichat_exit)
    {
      aichat_exit = false;
      lv_label_set_text(ui_Label2, "Closing...");
      auto& ai_vox_engine = ai_vox::Engine::GetInstance();
      ai_vox_engine.DisableconnectWebSocket();
      vTaskSuspend(aichat_task_handle);
      if(current_active_screen != 3)
      {
        vTaskDelay(pdMS_TO_TICKS(200));
        
        _ui_screen_change(&ui_AIChatExitScr, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_AIChatExitScr_screen_init);
      }
      
    }

    if(current_fun_is_Recognition)
    {
      lv_obj_t* currentPage = lv_scr_act();

      if(currentPage == ui_AIRecognition) {
          int buttonState = digitalRead(0);

          if (lastButtonState == HIGH && buttonState == LOW) {
              buttonPressed = true;
          }
          lastButtonState = buttonState;  

          if (buttonPressed) {
              buttonPressed = false;  
              AI_Recognition_flag = true;  
          }
      } 
      else if(currentPage == ui_AIRecognitionSave) {
          int buttonState = digitalRead(0);

          if (lastButtonState == HIGH && buttonState == LOW) {
              buttonPressed = true;
          }

          lastButtonState = buttonState; 

          if (buttonPressed) {
              buttonPressed = false;  
              AI_Recognition_exit_flag = true;  
          }
      }
    }

    if(Touch)
    {
      Touch = 0;
      auto& ai_vox_engine = ai_vox::Engine::GetInstance();
      ai_vox_engine.Touch();
    }

    if(load_anim_flag)
    {
        load_anim_flag = false;
        lv_obj_clear_flag(ui_AIRecognitionSpinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_arc_opa(ui_AIRecognitionSpinner, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }

    if(AI_Recognition_flag)
    {

      AI_Recognition_flag = false;
      vTaskSuspend(video_stream_task_handle);

      camera_output = NULL;
      lv_obj_add_flag(ui_AIRecognitionTitleCont, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_AIRecognitionIdentifyBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ui_press_lab, LV_OBJ_FLAG_HIDDEN);
      //AIRecognition_start_flag = true;
      AIRecognition_flag = true;
      In_AIRecognition_Save_Src = true;

    }
    else if(AI_Recognition_exit_flag)
    {
      AI_Recognition_exit_flag = false;
      lv_obj_clear_flag(ui_AIRecognitionTitleCont, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(ui_AIRecognitionIdentifyBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(ui_press_lab, LV_OBJ_FLAG_HIDDEN);
      vTaskResume(video_stream_task_handle);
      camera_output = NULL;
      In_AIRecognition_Save_Src = false;
      lv_obj_add_flag(ui_AIRecognitionSaveSDLab, LV_OBJ_FLAG_HIDDEN);
      _ui_screen_change(&ui_AIRecognition, LV_SCR_LOAD_ANIM_FADE_ON, 0, 0, &ui_AIRecognition_screen_init);
    }
  
  vTaskDelay(pdMS_TO_TICKS(5));
}