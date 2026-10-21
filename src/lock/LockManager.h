#pragma once
#include <Arduino.h>

class LockManager {
private:
  uint8_t relayPin;
  bool isRelayActive;
  unsigned long relayStartTime;
  unsigned long relayDurationMs;

public:
  LockManager(uint8_t pin);
  void begin();
  void unlock(unsigned long durationMs = 5000);
  void update();
};
