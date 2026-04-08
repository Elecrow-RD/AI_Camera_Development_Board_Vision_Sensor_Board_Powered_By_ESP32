#include <Arduino.h>
#include <Preferences.h>          // For storing persistent settings (not used here yet)
#include <Base64.h>               // Base64 encoding for sending images over HTTP
#include <WiFiClientSecure.h>     // HTTPS client
#include <HTTPClient.h>           // HTTP request handling
#include <WiFi.h>                 // Arduino WiFi library
#include <ArduinoJson.h>          // JSON parsing

#include "src\AW9523\elecrow_aw9523.h" 
#include "src\SD\elecrow_sd.h"

// I2C pins for AW9523 (LED/IO expander)
#define I2C_SCL 15
#define I2C_SDA 16

/* SD card SPI pins (use -1 if not using hardware NSS) */
#define SPI_NSS_PIN   -1
#define SPI_MISO_PIN  6
#define SPI_SCK_PIN   7
#define SPI_MOSI_PIN  8

// Initialize SD object with SPI pins
static ELECROW_SD my_sd(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_NSS_PIN);

// Pointer to I2C bus (Wire)
static TwoWire* wi = &Wire;

// WiFi credentials
const char* WIFI_SSID = "yanfa_software";
const char* WIFI_PASS = "yanfa-123456";

// Activation status
bool activationResult = false;

// Return the MAC address of the ESP32 in string format
String GetMacAddress()
{
    // Arduino WiFi library provides macAddress() function
    return WiFi.macAddress();
}

// Initialize WiFi in STA mode and connect
void wifi_init() {
    WiFi.mode(WIFI_STA);            // Set ESP32 as WiFi station
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("Connecting WiFi\n");
    // Wait until WiFi is connected
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("WiFi connected");
    Serial.println(WiFi.localIP()); // Print local IP
}

// Activation code and challenge code storage
String activationCode = "";
String challengeCode = "";

// Get activation code from remote server
bool GetActivationCode() {
    String deviceId = GetMacAddress();  // Device MAC as ID
    String url = "https://api.thinknode.cc/ota/";

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Device-Id", deviceId);

    // Create JSON body for POST
    DynamicJsonDocument doc(256);
    JsonObject application = doc.createNestedObject("application");
    JsonObject board = doc.createNestedObject("board");
    board["type"] = "AICamera";

    String requestBody;
    serializeJson(doc, requestBody);

    int httpCode = http.POST(requestBody);
    String response = "";

    if (httpCode > 0) {
        response = http.getString();   // Read response as string
    } else {
        http.end();
        return false;                  // HTTP request failed
    }

    http.end();

    // Parse JSON response
    DynamicJsonDocument jsonDoc(2048);
    DeserializationError error = deserializeJson(jsonDoc, response);
    if (error) {
        Serial.println("JSON parse error in activation response");
        return false;
    }

    // Check if activation code exists
    if (jsonDoc.containsKey("activation") && jsonDoc["activation"].containsKey("code")) {
        activationCode = jsonDoc["activation"]["code"].as<String>();
        challengeCode  = jsonDoc["activation"]["challenge"].as<String>();

        Serial.print("Activation Code: ");
        Serial.println(activationCode);

        return true;    // Activation succeeded
    }

    return false;       // No activation code received
}

bool result_save_flag = false;

// AI image recognition function
void AI_Recognition_func() {

    uint8_t *jpg_buf = NULL;
    size_t jpg_size = 0;

    // Read JPG image from SD card
    if (!my_sd.read_jpg_from_fs(SD, "/test.jpg", &jpg_buf, &jpg_size)) {
        Serial.println("Read JPG from SD failed");
        return;
    }

    // Encode image in Base64 for HTTP POST
    String imageBase64 = base64::encode(jpg_buf, jpg_size);

    WiFiClientSecure client;  // HTTPS client
    HTTPClient http;
    client.setInsecure();     // Ignore SSL certificate (not secure for production)

    String deviceAddr = GetMacAddress();
    deviceAddr.toLowerCase(); // Convert MAC to lowercase

    // API endpoint for image recognition
    const String url = "https://service.thinknode.cc/api/users/image-analysis-for-eps32-camera";
    const String token = "3696b69a9aa3b624cac9f691797be9e9";

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + token);

    // Build JSON request
    String jsonRequest;
    jsonRequest += "{";
    jsonRequest += "\"role\": \"You are an object recognition assistant. Only return the names of the objects in the picture, separated by commas. Do not add any explanations or extra text.\",";
    jsonRequest += "\"image\": \"data:image/jpeg;base64," + imageBase64 + "\",";
    jsonRequest += "\"mac_address\": \"" + deviceAddr + "\"";
    jsonRequest += "}";

    // Send POST request
    int httpResponseCode = http.POST(jsonRequest);
    String response = http.getString();

    if (httpResponseCode == 200) {
        // Parse JSON response
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, response);

        if (error) {
            Serial.println("JSON parse error in recognition response");
            http.end();
            free(jpg_buf);
            return;
        }

        if (doc.containsKey("code")) {
            int code = doc["code"];
            Serial.printf("Response code: %d\n", code);

            if (code == 200) {
                // Successful recognition
                if (doc.containsKey("data") && doc["data"].containsKey("response")) {
                    const char *content = doc["data"]["response"];
                    Serial.printf("Recognition result: %s\n", content);
                }
            } else if (code == 300) {
                // Device not bound
                Serial.println("Code 300: the device is not bound");
            } else if (code == 400) {
                if (doc.containsKey("msg")) {
                    const char *msg = doc["msg"];
                    Serial.printf("Code 400 message: %s\n", msg);
                }
            }
        } else {
            Serial.println("No 'code' field in recognition JSON");
        }
    } else {
        // HTTP request failed
        Serial.printf("HTTP request failed, code: %d\n", httpResponseCode);
    }

    http.end();
    free(jpg_buf); // Free memory allocated for image
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Initialize I2C bus for AW9523
    wi->setPins(I2C_SDA, I2C_SCL);
    wi->begin();
    delay(100);

    // Initialize AW9523 LED/IO expander
    aw9523.AW_init();
    aw9523.AW_set_POWER(true);

    // Connect WiFi
    wifi_init();

    // Initialize SD card
    my_sd.SD_init();
}

void loop() {

    static bool once = false;

    // Only activate and run recognition once
    if (!once) {
        activationResult = GetActivationCode();
        if (activationResult) {
            Serial.println("\nActivation success");
            AI_Recognition_func();
            once = true; // Prevent repeat
        }
    }

    delay(1000); // Avoid tight loop
}
