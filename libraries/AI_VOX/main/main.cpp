#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "ai_vox.h"
#include "clogger/clogger.h"

#define TAG "main"

namespace {
enum : EventBits_t {
  kStaStarted = 1 << 0,
  kWifiConnected = 1 << 1,
};

EventGroupHandle_t g_wifi_event_group = xEventGroupCreate();

void PrintMemInfo() {
  multi_heap_info_t info;

  // 查询内部内存信息
  heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
  CLOG("Internal Memory:");
  CLOG("Free: %zu bytes", info.total_free_bytes);
  CLOG("Allocated: %zu bytes", info.total_allocated_bytes);
  CLOG("Minimum Free: %zu bytes", info.minimum_free_bytes);

  // 查询IRAM信息
  heap_caps_get_info(&info, MALLOC_CAP_EXEC);
  CLOG("IRAM:");
  CLOG("Free: %zu bytes", info.total_free_bytes);
  CLOG("Allocated: %zu bytes", info.total_allocated_bytes);
  CLOG("Minimum Free: %zu bytes", info.minimum_free_bytes);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  switch (event_id) {
    case WIFI_EVENT_STA_START:
      CLOG("WIFI_EVENT_STA_START");
      xEventGroupSetBits(g_wifi_event_group, kStaStarted);
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      CLOG("WIFI_EVENT_STA_DISCONNECTED");
      break;
    default:
      break;
  }
  return;
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  ip_event_got_ip_t* event;
  switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
      event = (ip_event_got_ip_t*)event_data;
      CLOG("IP_EVENT_STA_GOT_IP");
      CLOG("got ip:" IPSTR, IP2STR(&event->ip_info.ip));
      xEventGroupSetBits(g_wifi_event_group, kWifiConnected);
      break;
    default:
      break;
  }

  return;
}

void WifiConnect() {
  CLOG_TRACE();
  esp_err_t r = nvs_flash_init();
  if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    CLOG("no free pages or nvs version mismatch, erase..");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }

  esp_netif_init();
  esp_event_loop_create_default();
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL));
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  wifi_config_t wifi_config = {
      .sta = {.ssid = "emakefun", .password = "501416wf"},
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  xEventGroupWaitBits(g_wifi_event_group, kStaStarted, pdFALSE, pdTRUE, portMAX_DELAY);
  ESP_ERROR_CHECK(esp_wifi_connect());
  xEventGroupWaitBits(g_wifi_event_group, kWifiConnected, pdFALSE, pdTRUE, portMAX_DELAY);
  CLOG("wifi connected");
}

EngineImpl* g_ai_vox = new EngineImpl();
}  // namespace

extern "C" void app_main(void) {
  CLOG_TRACE();
  PrintMemInfo();
  WifiConnect();
  PrintMemInfo();
  g_ai_vox->Start();
  PrintMemInfo();
}
