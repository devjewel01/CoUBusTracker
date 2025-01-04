#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include "esp_camera.h"
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// WiFi credentials
const char* ssid = "RoboAI";
const char* password = "11235813";

// Server details
const char* serverUrl = "https://roboict.duckdns.org";

// GPS configuration
static const int GPS_RX = 13;
static const int GPS_TX = 12;
static const uint32_t GPS_BAUD = 9600;

// Camera configuration - AI Thinker
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Global objects
TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
WiFiClientSecure client;

// Function declarations
void setupCamera();
bool sendGPSData();
bool sendFrame();

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  // Skip SSL certificate verification
  client.setInsecure();
  
  // Initialize camera
  setupCamera();
}

void loop() {
  // Update GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Send GPS data if available
  if (gps.location.isValid()) {
    if (sendGPSData()) {
      Serial.println("GPS data sent successfully");
    } else {
      Serial.println("Failed to send GPS data");
    }
  }

  // Send camera frame
  if (sendFrame()) {
    Serial.println("Frame sent successfully");
  } else {
    Serial.println("Failed to send frame");
  }

  // Small delay to prevent overwhelming the server
  delay(1000);
}

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera initialized successfully");
}

bool sendGPSData() {
  HTTPClient http;
  String url = String(serverUrl) + "/api/gps";
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  
  String jsonData = "{\"lat\":" + String(gps.location.lat(), 6) + 
                    ",\"lng\":" + String(gps.location.lng(), 6) + 
                    ",\"alt\":" + String(gps.altitude.meters()) + 
                    ",\"speed\":" + String(gps.speed.kmph()) + 
                    ",\"satellites\":" + String(gps.satellites.value()) + "}";
  
  int httpCode = http.POST(jsonData);
  bool success = (httpCode == HTTP_CODE_OK);
  
  http.end();
  return success;
}

bool sendFrame() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }

  HTTPClient http;
  String url = String(serverUrl) + "/api/stream";
  
  http.begin(client, url);
  http.addHeader("Content-Type", "image/jpeg");
  
  int httpCode = http.POST(fb->buf, fb->len);
  bool success = (httpCode == HTTP_CODE_OK);
  
  esp_camera_fb_return(fb);
  http.end();
  return success;
}