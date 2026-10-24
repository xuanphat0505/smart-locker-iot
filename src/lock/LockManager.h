#pragma once
#include <Arduino.h>

class LockManager {
private:
  uint8_t relayPin;
  bool isRelayActive;
  unsigned long relayStartTime;
  unsigned long relayDurationMs;

public:
  // Thời gian cấp điện tối đa cho cuộn hút để bảo vệ solenoid dù lệnh yêu cầu lâu hơn
  static const unsigned long MAX_UNLOCK_MS = 5000;

  LockManager(uint8_t pin);
  void begin();
  void unlock(unsigned long durationMs = 5000);
  void update();
  bool isActive() const { return isRelayActive; }
};
