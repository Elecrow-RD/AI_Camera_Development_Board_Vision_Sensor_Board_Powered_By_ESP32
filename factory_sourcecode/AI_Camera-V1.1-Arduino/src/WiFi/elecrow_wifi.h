#ifndef _WIFI_H_
#define _WIFI_H_

#include "Arduino.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <WiFi.h>
#include "esp_wifi.h"
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>


#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK


/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define EXAMPLE_ESP_MAXIMUM_RETRY  10
#define CONFIG_ESP_WIFI_PW_ID      ""

#define AP_SSID "AI_Camera_"
#define AP_PASSWORD "1234567890"



extern Preferences preferences;
extern WebServer server;
extern DNSServer dnsServer;
extern bool isAPMode;
extern bool isAudio;
extern unsigned long wifiConnectStartTime;
extern const byte DNS_PORT;
extern const char* apSSID;
extern const char* apPassword;
extern const unsigned long WIFI_TIMEOUT;
extern String ssidstr;
extern uint8_t RssiLev;
extern bool current_wifi_connected;
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

extern bool scan_done;
static int s_retry_num = 0;
extern const char * ssidstr_c;

extern bool restartFlag;

extern char wifi_ssid[32];  

void wifi_init_sta(const char * ssid, const char * password);

void connectToWiFi();

void checkWiFiConnection();

void startAPMode();

void updateWiFiScanList();

void WiFi_Disconnect();

#endif