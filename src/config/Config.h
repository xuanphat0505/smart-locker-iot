#pragma once
#include <Arduino.h>

// Cấu hình mạng WiFi và thông số kênh 2.4GHz

// ===== wifi cafe =====
// const char* const WIFI_SSID = "RUBY'S COFFEE 2";
// const char* const WIFI_PASSWORD = "33337777";

// ===== wifi home ======
const char* const WIFI_SSID = "S04";
const char* const WIFI_PASSWORD = "phattran";

// const char* const WIFI_SSID = "PTIT.HCM_SV";
// const char* const WIFI_PASSWORD = "";

const uint8_t WIFI_BSSID_24G[] = {0xa8, 0x25, 0xeb, 0x89, 0xf4, 0xeb};
const int WIFI_CHANNEL_24G = 11;

// Cấu hình MQTT Broker HiveMQ Cloud
const char* const MQTT_BROKER = "93d5dd6a263844b6b7386a2601c83c32.s1.eu.hivemq.cloud";
const int MQTT_PORT = 8883;
const char* const MQTT_USER = "phattran0123";
const char* const MQTT_PASS = "phattran0123";

// Cấu hình thông tin trạm tủ
const char* const LOCKER_CODE = "LK-TECCO-01";

// Khai báo bộ chân phần cứng của một ngăn tủ
struct BoxHardware {
  uint8_t boxNumber;
  uint8_t relayPin;
  uint8_t reedPin;
  uint8_t irPin;
};

// Bảng ánh xạ số ngăn sang chân, thứ tự S, M, L (số ngăn phải khớp dữ liệu trên server)
// Chân reed 34, 35 chỉ hỗ trợ đầu vào và không có điện trở kéo lên nội, cần điện trở kéo lên ngoài 10k
const BoxHardware BOXES[] = {
  {1, 13, 32, 26},
  {2, 14, 35, 25},
  {3, 27, 34, 33},
};
constexpr size_t BOX_COUNT = sizeof(BOXES) / sizeof(BOXES[0]);

// Định nghĩa chân GPIO giao tiếp phần cứng với ESP32-CAM, không dùng được trên module có PSRAM
#define CAM_TRIGGER_PIN 16
#define CAM_STATUS_PIN 17

// Bỏ chú thích dòng dưới để bật chế độ tự kiểm tra relay và cảm biến trên bàn thử
// #define BOX_SELFTEST

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
