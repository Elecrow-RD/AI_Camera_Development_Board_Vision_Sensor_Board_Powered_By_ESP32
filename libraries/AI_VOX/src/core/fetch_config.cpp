#include "fetch_config.h"

#include <cJSON.h>
#include <esp_app_desc.h>
#include <esp_chip_info.h>
#include <esp_crt_bundle.h>
#include <esp_flash.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_ota_ops.h>

#include <string>
#include <vector>

#ifndef CLOGGER_SEVERITY
#define CLOGGER_SEVERITY CLOGGER_SEVERITY_WARN
#endif
#include "clogger/clogger.h"

#define CONFIG_OTA_VERSION_URL "https://api.tenclass.net/xiaozhi/ota/"

namespace {

std::string GetMacAddress() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return std::string(mac_str);
}

std::string Uuid() {
  // UUID v4 需要 16 字节的随机数据
  uint8_t uuid[16];

  // 使用 ESP32 的硬件随机数生成器
  esp_fill_random(uuid, sizeof(uuid));

  // 设置版本 (版本 4) 和变体位
  uuid[6] = (uuid[6] & 0x0F) | 0x40;  // 版本 4
  uuid[8] = (uuid[8] & 0x3F) | 0x80;  // 变体 1

  // 将字节转换为标准的 UUID 字符串格式
  char uuid_str[37];
  snprintf(uuid_str,
           sizeof(uuid_str),
           "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
           uuid[0],
           uuid[1],
           uuid[2],
           uuid[3],
           uuid[4],
           uuid[5],
           uuid[6],
           uuid[7],
           uuid[8],
           uuid[9],
           uuid[10],
           uuid[11],
           uuid[12],
           uuid[13],
           uuid[14],
           uuid[15]);

  return std::string(uuid_str);
}

size_t GetFlashSize() {
  uint32_t flash_size;
  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
    CLOG("Failed to get flash size");
    return 0;
  }
  return (size_t)flash_size;
}

std::string Json2() {
  cJSON* root = cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "version", 2);
  cJSON_AddNumberToObject(root, "flash_size", GetFlashSize());
  cJSON_AddNumberToObject(root, "minimum_free_heap_size", esp_get_minimum_free_heap_size());
  cJSON_AddStringToObject(root, "mac_address", GetMacAddress().c_str());
  cJSON_AddStringToObject(root, "uuid", Uuid().c_str());
  cJSON_AddStringToObject(root, "chip_model_name", "esp32");

#if 1
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  cJSON* chip_info_json = cJSON_CreateObject();
  cJSON_AddNumberToObject(chip_info_json, "model", chip_info.model);
  cJSON_AddNumberToObject(chip_info_json, "cores", chip_info.cores);
  cJSON_AddNumberToObject(chip_info_json, "revision", chip_info.revision);
  cJSON_AddNumberToObject(chip_info_json, "features", chip_info.features);
  cJSON_AddItemToObject(root, "chip_info", chip_info_json);
#endif
#if 1
  auto app_desc = esp_app_get_description();
  cJSON* application_json = cJSON_CreateObject();
  cJSON_AddStringToObject(application_json, "name", app_desc->project_name);
  cJSON_AddStringToObject(application_json, "version", app_desc->version);
  cJSON_AddStringToObject(application_json, "compile_time", (std::string(app_desc->date) + "T" + app_desc->time + "Z").c_str());
  cJSON_AddStringToObject(application_json, "idf_version", app_desc->idf_ver);
  char sha256_str[65];
  for (int i = 0; i < 32; i++) {
    snprintf(sha256_str + i * 2, sizeof(sha256_str) - i * 2, "%02x", app_desc->app_elf_sha256[i]);
  }
  cJSON_AddStringToObject(application_json, "elf_sha256", sha256_str);
  cJSON_AddItemToObject(root, "application", application_json);
#endif

#if 1
  cJSON* partition_table_json = cJSON_CreateArray();
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  while (it) {
    const esp_partition_t* partition = esp_partition_get(it);
    cJSON* partition_json = cJSON_CreateObject();
    cJSON_AddStringToObject(partition_json, "label", partition->label);
    cJSON_AddNumberToObject(partition_json, "type", partition->type);
    cJSON_AddNumberToObject(partition_json, "subtype", partition->subtype);
    cJSON_AddNumberToObject(partition_json, "address", partition->address);
    cJSON_AddNumberToObject(partition_json, "size", partition->size);
    cJSON_AddItemToArray(partition_table_json, partition_json);
    it = esp_partition_next(it);
  }
  cJSON_AddItemToObject(root, "partition_table", partition_table_json);
#endif
#if 1
  auto ota_partition = esp_ota_get_running_partition();
  cJSON* ota_json = cJSON_CreateObject();
  cJSON_AddStringToObject(ota_json, "label", ota_partition->label);
  cJSON_AddItemToObject(root, "ota", ota_json);
#endif

#if 1
  cJSON* board_json = cJSON_CreateObject();
  cJSON_AddStringToObject(board_json, "type", "AICamera");
  cJSON_AddStringToObject(board_json, "ssid", "");
  cJSON_AddNumberToObject(board_json, "rssi", 0);
  cJSON_AddNumberToObject(board_json, "channel", 0);
  cJSON_AddStringToObject(board_json, "ip", "");
  cJSON_AddStringToObject(board_json, "mac", GetMacAddress().c_str());
  cJSON_AddItemToObject(root, "board", board_json);
#endif
  auto text = cJSON_PrintUnformatted(root);
  std::string json(text);
  cJSON_free(text);
  cJSON_Delete(root);
  return json;
}

}  // namespace

std::optional<Config> GetConfigFromServer(const std::string& url, const std::string& uuid) {
  CLOGI();
  Config config;
  esp_http_client_config_t http_client_config;
  memset(&http_client_config, 0, sizeof(http_client_config));
  http_client_config.url = url.c_str();
  http_client_config.crt_bundle_attach = esp_crt_bundle_attach;
  
  const auto post_json = Json2();
  CLOGD("json: %s", post_json.c_str());
  auto client = esp_http_client_init(&http_client_config);
  if (client == nullptr) {
    CLOGE("esp_http_client_init failed.");
    return std::nullopt;
  }
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "Device-Id", GetMacAddress().c_str());
  esp_http_client_set_header(client, "Client-Id", uuid.c_str());
  esp_http_client_set_header(client, "Content-Type", "application/json");

  auto err = esp_http_client_open(client, post_json.length());

  if (err != ESP_OK) {
    CLOGE("esp_http_client_open failed. Error: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return std::nullopt;
  }

  auto wlen = esp_http_client_write(client, post_json.data(), post_json.length());
  if (wlen < 0) {
    CLOGE("esp_http_client_write failed.");
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return std::nullopt;
  }

  auto content_length = esp_http_client_fetch_headers(client);
  if (content_length < 0) {
    CLOGE("esp_http_client_fetch_headers failed.");
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return std::nullopt;
  }

  std::vector<uint8_t> response(content_length);
  esp_http_client_read_response(client, reinterpret_cast<char*>(response.data()), content_length);
  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  CLOGI("response:%.*s", static_cast<int>(response.size()), response.data());

  auto* const root = cJSON_ParseWithLength(reinterpret_cast<const char*>(response.data()), response.size());
  if (!cJSON_IsObject(root)) {
    cJSON_Delete(root);
    return std::nullopt;
  }

  auto* mqtt_json = cJSON_GetObjectItem(root, "mqtt");
  if (cJSON_IsObject(mqtt_json)) {
    auto* endpoint = cJSON_GetObjectItem(mqtt_json, "endpoint");
    if (cJSON_IsString(endpoint)) {
      config.mqtt.endpoint = endpoint->valuestring;
    }

    auto* client_id = cJSON_GetObjectItem(mqtt_json, "client_id");
    if (cJSON_IsString(client_id)) {
      config.mqtt.client_id = client_id->valuestring;
    }

    auto* username = cJSON_GetObjectItem(mqtt_json, "username");
    if (cJSON_IsString(username)) {
      config.mqtt.username = username->valuestring;
    }

    auto* password = cJSON_GetObjectItem(mqtt_json, "password");
    if (cJSON_IsString(password)) {
      config.mqtt.password = password->valuestring;
    }

    auto* publish_topic = cJSON_GetObjectItem(mqtt_json, "publish_topic");
    if (cJSON_IsString(publish_topic)) {
      config.mqtt.publish_topic = publish_topic->valuestring;
    }

    auto* subscribe_topic = cJSON_GetObjectItem(mqtt_json, "subscribe_topic");
    if (cJSON_IsString(subscribe_topic)) {
      config.mqtt.subscribe_topic = subscribe_topic->valuestring;
    }
  }

  auto* activation_json = cJSON_GetObjectItem(root, "activation");
  if (cJSON_IsObject(activation_json)) {
    auto* code = cJSON_GetObjectItem(activation_json, "code");
    if (cJSON_IsString(code)) {
      config.activation.code = code->valuestring;
    }

    auto* message = cJSON_GetObjectItem(activation_json, "message");
    if (cJSON_IsString(message)) {
      config.activation.message = message->valuestring;
    }
  }

  cJSON_Delete(root);
  return config;
}