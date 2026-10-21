#pragma once
#include <Arduino.h>

// Cấu hình mạng WiFi và thông số kênh 2.4GHz

// ===== wifi cafe =====
const char* const WIFI_SSID = "RUBY'S COFFEE 2";
const char* const WIFI_PASSWORD = "33337777";

// ===== wifi home ======
// const char* const WIFI_SSID = "S04";
// const char* const WIFI_PASSWORD = "phattran";

// const char* const WIFI_SSID = "hoiithunn";
// const char* const WIFI_PASSWORD = "njsjyghsjmthjk.1306";

const uint8_t WIFI_BSSID_24G[] = {0xa8, 0x25, 0xeb, 0x89, 0xf4, 0xeb};
const int WIFI_CHANNEL_24G = 11;

// Cấu hình MQTT Broker HiveMQ Cloud
const char* const MQTT_BROKER = "93d5dd6a263844b6b7386a2601c83c32.s1.eu.hivemq.cloud";
const int MQTT_PORT = 8883;
const char* const MQTT_USER = "phattran0123";
const char* const MQTT_PASS = "phattran0123";

// Cấu hình thông tin trạm tủ
const char* const LOCKER_CODE = "LK-TECCO-01";
const int BOX_NUMBER = 1;

// Định nghĩa các chân GPIO
#define RELAY_PIN 13
#define REED_SWITCH_PIN 14
#define IR_SENSOR_PIN 27

// Định nghĩa chân GPIO giao tiếp phần cứng với ESP32-CAM
#define CAM_TRIGGER_PIN 26
#define CAM_STATUS_PIN 25

// Cấu hình thời gian đếm ngược và thời gian chờ tối đa khi xác thực khuôn mặt
#define FACE_COUNTDOWN_SECONDS 3
#define FACE_VERIFY_TIMEOUT_MS 15000

// Thời gian tối đa các màn hình tạm thời được hiển thị trước khi tự quay về màn hình chính (miligiây)
#define SUCCESS_SCREEN_TIMEOUT_MS 8000
#define ERROR_SCREEN_TIMEOUT_MS 3000
#define QR_SCREEN_TIMEOUT_MS 90000
#define PAYMENT_SCREEN_TIMEOUT_MS 180000

// Thời gian hiển thị tối thiểu cho màn hình kết nối WiFi (miligiây)
#define WIFI_CONNECT_MIN_DISPLAY_MS 2800

// Địa chỉ máy chủ Backend API NestJS
// #define API_SERVER_URL "http://10.225.249.145:5005"
#define API_SERVER_URL "http://192.168.34.145:5005"

// Khóa bí mật định danh thiết bị IoT kết nối Backend
#define LOCKER_API_KEY "secret-key-lk-tecco-01"
