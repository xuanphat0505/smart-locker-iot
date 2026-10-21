#include "LockManager.h"

// Khởi tạo đối tượng điều khiển khóa với chân rơ-le chỉ định
LockManager::LockManager(uint8_t pin)
  : relayPin(pin), isRelayActive(false), relayStartTime(0), relayDurationMs(5000) {}

// Cấu hình chân rơ-le và mặc định ngắt điện khóa ở mức thấp
void LockManager::begin() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
}

// Kích hoạt rơ-le kéo lên mức cao để cấp điện mở chốt Solenoid
void LockManager::unlock(unsigned long durationMs) {
  Serial.println("[RELAY] Kích hoạt rơ-le mở chốt Solenoid (Cấp điện)");
  digitalWrite(relayPin, HIGH);
  isRelayActive = true;
  relayStartTime = millis();
  relayDurationMs = durationMs > 0 ? durationMs : 5000;
}

// Đếm thời gian không chặn luồng để tự động ngắt điện bảo vệ cuộn hút
void LockManager::update() {
  if (isRelayActive && (millis() - relayStartTime >= relayDurationMs)) {
    digitalWrite(relayPin, LOW);
    isRelayActive = false;
    Serial.println("[RELAY] Đã ngắt điện rơ-le bảo vệ Solenoid an toàn");
  }
}
