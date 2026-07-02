#include "esp_wifi.h"
#include "WiFi.h"
#include "elecrow_wifi.h"

#define DEBUG_
#define WIFI_DHCP_TIMEOUT  15000 
#define WIFI_TIMEOUT       15000

#define WIFI_SCAN_INTERVAL 7500

static uint32_t lastScanTime = 0;
static bool scan_in_progress = false;

const int RESET_BUTTON_PIN = 0;   

bool WiFi_Connect_State = false;
String wifiScanResultHtml = "";

char wifi_ssid[32] = {0};  
bool scan_done = false;

static bool wifi_initialized = false;
static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif = NULL;
wifi_ap_record_t ap_records[30]; // 最多显示20个

String ssidstr;

void handleRoot();
void handleSave();
void handleGetWiFiList();
void handleNotFound();
IPAddress getApIP();
// ================= WiFi Scan Function =================
void updateWiFiScanList() {
    if (scan_in_progress) return;
    scan_in_progress = true;

    wifi_scan_config_t scanConf = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {.active = {.min = 120, .max = 250}}
    };

    esp_err_t ret = esp_wifi_scan_start(&scanConf, false);
    if (ret != ESP_OK) {
        wifiScanResultHtml = "<option>Scan failed</option>";
        scan_in_progress = false;
        return;
    }
    scan_done = true;
    if(WiFi_Connect_State) {
        vTaskDelete(NULL);
    }
}

static void wifi_scan_done_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    uint16_t number = 0;
    esp_wifi_scan_get_ap_num(&number);

    if (number == 0) {
        wifiScanResultHtml = "<option>No networks found</option>";
        scan_in_progress = false;
        return;
    }

    if (number > 30) number = 30;

    wifi_ap_record_t *records =
        (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * number);

    if (!records) {
        wifiScanResultHtml = "<option>Memory error</option>";
        scan_in_progress = false;
        return;
    }

    esp_wifi_scan_get_ap_records(&number, records);

    wifiScanResultHtml = "";
    String seen = "|";

    for (int i = 0; i < number; i++) {
        String ssid = String((char *)records[i].ssid);
        if (ssid.length() == 0) continue;
        if (seen.indexOf("|" + ssid + "|") >= 0) continue;
        seen += ssid + "|";

        ssid.replace("&", "&amp;");
        ssid.replace("<", "&lt;");
        ssid.replace(">", "&gt;");

        wifiScanResultHtml +=
            "<option value=\"" + ssid + "\">" + ssid + "</option>";
    }

    free(records);
    scan_in_progress = false;
}


// ================= page Function =================
void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>WiFi Connect</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html, body {
      overscroll-behavior-y: none;
      -webkit-overflow-scrolling: touch;
    }

    body {
      font-family: Arial;
      text-align: center;
      margin-top: 20px;
      padding: 20px;
    }

    form {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 12px;
    }

    select,
    input[type="text"],
    input[type="password"] {
      width: 80%;
      max-width: 300px;
      padding: 10px;
      font-size: 16px;
      box-sizing: border-box;
      border: 1px solid #ccc;
      border-radius: 4px;
      height: 44px;
      background-color: #fff;
    }

    .password-container {
      position: relative;
      width: 80%;
      max-width: 300px;
    }

    .password-container input {
      width: 100%;
      padding-right: 40px;
    }

    .toggle-password {
      position: absolute;
      right: 10px;
      top: 50%;
      transform: translateY(-50%);
      cursor: pointer;
      width: 24px;
      height: 24px;
      stroke: #666;
      fill: none;
      stroke-width: 2;
    }

    .toggle-password:hover {
      stroke: #333;
    }

    /* ===== 按钮行 ===== */
    .button-row {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-top: 8px;
    }

    button {
      padding: 12px 24px;
      background: #4CAF50;
      color: white;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 16px;
      height: 44px;
    }

    button:hover {
      background: #45a049;
    }

    /* ===== 刷新按钮 ===== */
    .refresh-btn {
      width: 44px;
      height: 44px;
      border: 1px solid #ccc;
      border-radius: 4px;
      background: #f9f9f9;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
    }

    .refresh-btn:hover {
      background: #eee;
    }

    .refresh-icon {
      width: 22px;
      height: 22px;
      stroke: #333;
      fill: none;
      stroke-width: 2;
    }
  </style>
</head>

<body>
  <h2>Select or Enter WiFi</h2>

  <form method="POST" action="/save">

    <select id="ssidSelect">
      <option value="">-- Select WiFi Network --</option>
      %ssid_list%
      <option value="__manual__">Other (Enter manually)</option>
    </select>

    <input type="text" id="ssidInput" name="ssid" placeholder="Enter SSID" required>

    <div class="password-container">
      <input type="password" id="password" name="password" placeholder="WiFi Password" required>
      <svg id="togglePassword" class="toggle-password" viewBox="0 0 24 24">
        <path d="M17.94 17.94A10.94 10.94 0 0 1 12 20C6.5 20 2 12 2 12s1.7-3.1 5.06-5.94" />
        <path d="M14.12 14.12A3 3 0 0 1 9.88 9.88" />
        <line x1="1" y1="1" x2="23" y2="23" />
      </svg>
    </div>

    <div class="button-row">
      <button type="submit">Save Configuration</button>

      <div class="refresh-btn" id="refreshWifi" title="Refresh WiFi list">
        <svg class="refresh-icon" viewBox="0 0 24 24">
          <polyline points="23 4 23 10 17 10"/>
          <polyline points="1 20 1 14 7 14"/>
          <path d="M3.5 9a9 9 0 0114.13-3.36L23 10"/>
          <path d="M20.5 15a9 9 0 01-14.13 3.36L1 14"/>
        </svg>
      </div>
    </div>

  </form>

<script>
const ssidSelect = document.getElementById('ssidSelect');
const ssidInput  = document.getElementById('ssidInput');
const refreshBtn = document.getElementById('refreshWifi');

function updateWifiList() {
  refreshBtn.style.opacity = '0.5';

  fetch('/get_wifi_list')
    .then(res => res.text())
    .then(html => {
      const current = ssidSelect.value;

      ssidSelect.innerHTML =
        '<option value="">-- Select WiFi Network --</option>' +
        html +
        '<option value="__manual__">Other (Enter manually)</option>';

      for (let i = 0; i < ssidSelect.options.length; i++) {
        if (ssidSelect.options[i].value === current) {
          ssidSelect.selectedIndex = i;
          break;
        }
      }
    })
    .finally(() => {
      refreshBtn.style.opacity = '1';
    });
}

// 手动刷新
refreshBtn.addEventListener('click', updateWifiList);

// 选择逻辑
ssidSelect.addEventListener('change', function () {
  if (this.value === '__manual__' || this.value === '') {
    ssidInput.value = '';
    ssidInput.readOnly = false;
    ssidInput.readOnly = false;
    ssidInput.placeholder = 'Enter SSID manually';
    ssidInput.focus();
  } else {
    ssidInput.value = this.value;
    ssidInput.readOnly = true;
  }
  document.getElementById('password').focus();
});

    const togglePassword = document.getElementById('togglePassword');
    const passwordInput = document.getElementById('password');
    let isVisible = false;

    const eyeOpen = `
      <path d="M1 12s4-7 11-7 11 7 11 7-4 7-11 7S1 12 1 12z" />
      <circle cx="12" cy="12" r="3" />
    `;
    const eyeClosed = `
      <path d="M17.94 17.94A10.94 10.94 0 0 1 12 20C6.5 20 2 12 2 12s1.7-3.1 5.06-5.94" />
      <path d="M14.12 14.12A3 3 0 0 1 9.88 9.88" />
      <line x1="1" y1="1" x2="23" y2="23" />
    `;

    togglePassword.addEventListener('click', () => {
      isVisible = !isVisible;
      passwordInput.type = isVisible ? 'text' : 'password';
      togglePassword.innerHTML = isVisible ? eyeOpen : eyeClosed;
    });

    document.addEventListener('touchstart', function(e) {
      if (e.target.tagName === 'SELECT' || e.target.tagName === 'INPUT') {
        e.stopPropagation();
      }
    }, true);
</script>

</body>
</html>
)rawliteral";

  page.replace("%ssid_list%", wifiScanResultHtml);
  server.send(200, "text/html", page);
}


String GetDeviceName() {
    uint8_t mac[6];

    esp_wifi_get_mac(WIFI_IF_STA, mac);

    // 取后两字节，也就是后四位16进制字符
    char mac_suffix[5];  // 4位 + '\0'
    sprintf(mac_suffix, "%02X%02X", mac[4], mac[5]);

    // 拼接字符串
    String deviceName = "AI_Camera_";
    deviceName += mac_suffix;

    return deviceName;
}

void handleGetWiFiList() {
    server.send(200, "text/html", wifiScanResultHtml);
}


uint8_t RssiLev;
void wifi_rssi_level()
{

    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
    int8_t rssi = ap_info.rssi; // (dBm)


    if(rssi >= -65) RssiLev = 5;
    else if(rssi >= -75) RssiLev = 4;
    //else if(rssi >= -85) RssiLev = 3;
    else if(rssi >= -85) RssiLev = 2;
    else RssiLev = 1;


}

void exitAPModeAndConnect(String ssid, String password) {
  
  // Stop DNS and web servers
  dnsServer.stop();
  server.stop();
  // 2. Shut down the AP
  esp_wifi_stop();  
  if (ap_netif) {
      esp_netif_destroy(ap_netif);
      ap_netif = nullptr; 
  }

  // 3.Update status indicator
  isAPMode = false; 
  esp_wifi_set_mode(WIFI_MODE_STA); 
  vTaskDelay(pdMS_TO_TICKS(200));
  ESP.restart();
}

void handleNotFound() {
  if (isAPMode) {
    server.sendHeader("Location", "http://" + server.client().localIP().toString() + "/", true); // 20251125 add + "/"
    server.send(302, "text/plain", "");
  } else {
    server.send(404, "text/plain", "404: Not found");
  }
}

void handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  
  if(ssid.length() == 0 || password.length() < 8) {
    server.send(400, "text/html", "<h2>error</h2><p>The SSID cannot be empty and the password must be at least 8 characters long.</p><a href='/'>return</a>");
    return;
  }
  
  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
  
  String response = "<h2>Configuration saved!</h2>";
  response += "<p>Trying to connect to: " + ssid + "</p>";
  response += "<p>The device will automatically reboot...</p>";
  response += "<script>setTimeout(function(){location.href='/';}, 5000);</script>";
  server.send(200, "text/html", response);
  
  vTaskDelay(200 / portTICK_PERIOD_MS);
  exitAPModeAndConnect(ssid, password);
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
      if(WiFi_Connect_State == false)   esp_wifi_connect();
         
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) { 
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect(); 
            s_retry_num++;

        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void start_ap_mode() {

  WiFi_Disconnect();

  if (ap_netif) {
      esp_netif_destroy(ap_netif);
      ap_netif = NULL;
  }
  if (sta_netif) {
      esp_netif_destroy(sta_netif);
      sta_netif = NULL;
  }

  ap_netif  = esp_netif_create_default_wifi_ap();
  sta_netif = esp_netif_create_default_wifi_sta();

  esp_netif_set_default_netif(ap_netif);

  ssidstr = GetDeviceName();
  ssidstr_c = ssidstr.c_str();
  
  wifi_config_t ap_config = {0};
  strncpy((char*)ap_config.ap.ssid, ssidstr.c_str(),
          sizeof(ap_config.ap.ssid) - 1);
  ap_config.ap.ssid_len = ssidstr.length();
  ap_config.ap.password[0] = '\0';
  ap_config.ap.channel = 6;
  ap_config.ap.authmode = WIFI_AUTH_OPEN;
  ap_config.ap.ssid_hidden = 0;
  ap_config.ap.max_connection = 4;
  ap_config.ap.beacon_interval = 100;

  esp_wifi_set_mode(WIFI_MODE_APSTA);
  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();

 
  lastScanTime = 0;
  updateWiFiScanList();

  IPAddress apIP = getApIP();

  dnsServer.start(53, "*", apIP);

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/get_wifi_list", HTTP_GET, handleGetWiFiList);
  server.onNotFound(handleNotFound);
  server.begin();


}

void wifi_init_sta(const char * ssid, const char * password)
{

    if (!s_wifi_event_group) {
        s_wifi_event_group = xEventGroupCreate();
    }
    if( sta_netif == NULL)
    {
        sta_netif = esp_netif_create_default_wifi_sta();
    }

    static bool events_registered = false;
    if (!events_registered) {

        esp_event_handler_instance_register(WIFI_EVENT, 
                                           WIFI_EVENT_SCAN_DONE, 
                                           &wifi_scan_done_handler, NULL, NULL);
        

        esp_event_handler_instance_register(WIFI_EVENT,
                                           ESP_EVENT_ANY_ID,
                                           &event_handler,
                                           NULL,
                                           NULL);
        esp_event_handler_instance_register(IP_EVENT,
                                           IP_EVENT_STA_GOT_IP,
                                           &event_handler,
                                           NULL,
                                           NULL);
        events_registered = true;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold =  {
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };

    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0'; // 确保 null 终止


    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0'; // 确保 null 终止
    esp_wifi_stop();
    esp_wifi_set_mode(WIFI_MODE_STA);

    ssidstr = GetDeviceName();

    ssidstr_c = ssidstr.c_str();

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);  
    esp_wifi_start();

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
             pdMS_TO_TICKS(WIFI_DHCP_TIMEOUT));

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        WiFi_Connect_State = true;
        current_wifi_connected = true;
    } else if (bits & WIFI_FAIL_BIT) {

    } else {
        startAPMode();

    }
}

IPAddress getApIP()
{
    esp_netif_ip_info_t ip_info;
    if (ap_netif) {
        esp_netif_get_ip_info(ap_netif, &ip_info);
        return IPAddress(ip_info.ip.addr);
    }
    return IPAddress((uint32_t)0);
}

void startAPMode() {

  isAPMode = true;
  //esp_wifi_disconnect();

  start_ap_mode();

}

void connectToWiFi() {
  WiFi_Disconnect();
  vTaskDelay(pdMS_TO_TICKS(200));
  if (!wifi_initialized) {
      esp_netif_init();
      esp_event_loop_create_default();
      esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &wifi_scan_done_handler, NULL, NULL);
      wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
      esp_wifi_init(&cfg);

      wifi_initialized = true;
  }
  



  if(sta_netif == NULL)
    sta_netif = esp_netif_create_default_wifi_sta();
  
  if(ap_netif == NULL)
    ap_netif =  esp_netif_create_default_wifi_ap();

  preferences.begin("wifi-config", true);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();

  if(ssid == "") {
    startAPMode();
    return;
  }

  strncpy(wifi_ssid, ssid.c_str(), sizeof(wifi_ssid) - 1);
  wifi_ssid[sizeof(wifi_ssid) - 1] = '\0'; 
  
  wifi_init_sta(ssid.c_str(), password.c_str());

  wifiConnectStartTime = millis();
}


void checkWiFiConnection() {

  if (isAPMode) {

    dnsServer.processNextRequest();
    server.handleClient();

    // 每 5 秒扫描一次
    if (millis() - lastScanTime > WIFI_SCAN_INTERVAL) {
        lastScanTime = millis();
        updateWiFiScanList();
    }
    return;
  }

  if (WiFi_Connect_State) {
    wifi_rssi_level();
    return;
  }

  if (millis() - wifiConnectStartTime > WIFI_TIMEOUT) {
    startAPMode();
  }
}

void WiFi_Disconnect()
{

  esp_wifi_disconnect();
  esp_wifi_stop();
}