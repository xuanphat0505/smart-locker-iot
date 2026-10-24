#pragma once
#include <Arduino.h>

class SensorManager {
private:
  uint8_t reedPin;
  uint8_t irPin;
  bool lastDoorClosed;
  bool lastHasItem;
  unsigned long lastCheckTime;
  unsigned long lastIrCheckTime;

public:
  SensorManager(uint8_t reed, uint8_t ir);
  void begin();
  bool isDoorClosed();
  bool hasItem();
  bool checkDoorJustClosed();
  bool checkDoorStateChanged(bool& currentClosed);
  bool checkItemStateChanged(bool& currentItem);
};
