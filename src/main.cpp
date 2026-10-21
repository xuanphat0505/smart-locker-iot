#include <Arduino.h>
#include <WiFi.h>
#include "config/Config.h"
#include "lock/LockManager.h"
#include "sensor/SensorManager.h"
#include "network/NetworkManager.h"
#include "mqtt/MqttManager.h"
#include "ble/BleManager.h"
#include "display/DisplayManager.h"
#include "api/ApiClient.h"

// Khởi tạo các module thành phần của tủ thông minh
LockManager lockManager(RELAY_PIN);
SensorManager sensorManager(REED_SWITCH_PIN, IR_SENSOR_PIN);
MqttManager mqttManager(lockManager, sensorManager);
BleManager bleManager;
DisplayManager displayManager;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== SMART LOCKER IOT CONTROLLER ===");

  displayManager.begin();
  lockManager.begin();
  sensorManager.begin();

  // Hiển thị màn hình kết nối WiFi trước khi thực sự kết nối
  displayManager.showWifiConnecting(WIFI_SSID);
  unsigned long wifiStartTime = millis();

  // Kết nối WiFi kèm cập nhật animation màn hình
  NetworkManager::connectWifi([&]() {
    displayManager.update();
  });

  // Cập nhật giao diện dựa trên kết quả kết nối WiFi
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[DISPLAY] WiFi connected -> Giữ animation hiển thị đủ thời gian tối thiểu");
    while (millis() - wifiStartTime < WIFI_CONNECT_MIN_DISPLAY_MS) {
      displayManager.update();
      delay(50);
    }
    displayManager.showIdle();
  } else {
    Serial.println("[DISPLAY] WiFi failed -> Show error screen");
    displayManager.showWifiFailed(WIFI_SSID);
  }

  mqttManager.begin();
  bleManager.begin(LOCKER_CODE);

  // Cấu hình chân GPIO điều khiển và nhận phản hồi từ module ESP32-CAM
  pinMode(CAM_TRIGGER_PIN, OUTPUT);
  digitalWrite(CAM_TRIGGER_PIN, LOW);
  pinMode(CAM_STATUS_PIN, INPUT_PULLDOWN);

  // Đăng ký callback khi đếm ngược 3s hoàn tất để gửi xung kích hoạt chụp ảnh cho ESP32-CAM
  displayManager.setOnFaceTrigger([]() {
    Serial.println("[KIOSK_FACE] Đếm ngược hoàn tất -> Phát xung kích hoạt chụp cho ESP32-CAM");
    digitalWrite(CAM_TRIGGER_PIN, HIGH);
    delay(80);
    digitalWrite(CAM_TRIGGER_PIN, LOW);
  });

  // Đăng ký callback khi người dùng nhập đủ 6 ký tự mã OTP trên màn hình Kiosk
  displayManager.setOnOtpSubmit([](const String& pin) {
    Serial.printf("[KIOSK_OTP] Người dùng nhập mã OTP: %s -> Gửi yêu cầu xác thực API\n", pin.c_str());

    // Gửi yêu cầu xác thực mã OTP lên máy chủ Backend NestJS
    OtpVerifyResult res = ApiClient::verifyPickupOtp(LOCKER_CODE, pin.c_str());

    if (res.requiresPayment) {
      int targetBox = res.boxNumber > 0 ? res.boxNumber : BOX_NUMBER;
      Serial.printf("[KIOSK_OTP] Yêu cầu thanh toán cước phí: %d VNĐ cho Ngăn #%d (Order #%ld)\n",
                    res.feeAmount, targetBox, res.orderCode);
      displayManager.showPaymentQr(targetBox, res.feeAmount, res.orderCode,
                                   res.qrPayload.c_str(), res.errorMessage.c_str());
    } else if (res.success) {
      int targetBox = res.boxNumber > 0 ? res.boxNumber : BOX_NUMBER;
      Serial.printf("[KIOSK_OTP] Xác thực thành công từ Server -> Mở Ngăn #%d\n", targetBox);
      lockManager.unlock(5000);
      displayManager.showSuccess(targetBox);
      mqttManager.publishStatus("DOOR_UNLOCKED");
    } else {
      Serial.printf("[KIOSK_OTP] Xác thực thất bại: %s -> Hiển thị lỗi\n", res.errorMessage.c_str());
      displayManager.showError(res.errorMessage.c_str());
    }
  });
}

void loop() {
  displayManager.update();
  NetworkManager::maintainConnection();
  mqttManager.update();
  lockManager.update();

  // Kiểm tra tín hiệu báo lỗi từ ESP32-CAM khi đang chờ xác thực khuôn mặt
  if (displayManager.getCurrentState() == STATE_FACE_PROCESSING) {
    if (digitalRead(CAM_STATUS_PIN) == HIGH) {
      Serial.println("[KIOSK_FACE] Nhan tin hieu loi tu ESP32-CAM -> Hien thi thong bao");
      displayManager.showError("FACE NOT RECOGNIZED");
    }
  }

  // Phát hiện thay đổi trạng thái đóng mở cửa để báo cáo MQTT
  bool doorClosed = false;
  if (sensorManager.checkDoorStateChanged(doorClosed)) {
    if (doorClosed) {
      Serial.println("[SENSOR] Cánh cửa vừa được đóng kín!");
      mqttManager.publishStatus("DOOR_CLOSED");

      // Quay về màn hình chờ ngay khi người dùng đóng kín cửa tủ
      if (displayManager.getCurrentState() == STATE_SUCCESS) {
        displayManager.showIdle();
      }
    } else {
      Serial.println("[SENSOR] Cánh cửa vừa được bật mở!");
      mqttManager.publishStatus("DOOR_OPENED");
    }
  }

  // Phát hiện thay đổi trạng thái vật thể trong ngăn tủ qua cảm biến quang IR
  bool hasItem = false;
  if (sensorManager.checkItemStateChanged(hasItem)) {
    if (hasItem) {
      Serial.println("[SENSOR] Cảm biến IR phát hiện bưu phẩm trong ngăn!");
      mqttManager.publishStatus("ITEM_DETECTED");
    } else {
      Serial.println("[SENSOR] Ngăn tủ đã trống!");
      mqttManager.publishStatus("ITEM_REMOVED");
    }
  }
}