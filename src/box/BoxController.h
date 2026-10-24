#pragma once
#include <Arduino.h>
#include <functional>
#include "../config/Config.h"
#include "../lock/LockManager.h"
#include "../sensor/SensorManager.h"

// Các sự kiện cảm biến của một ngăn tủ
enum BoxEvent {
  BOX_DOOR_OPENED,
  BOX_DOOR_CLOSED,
  BOX_ITEM_DETECTED,
  BOX_ITEM_REMOVED
};

// Điều khiển nhiều ngăn tủ: tra cứu khóa và cảm biến theo số ngăn thay vì theo chỉ số mảng
class BoxController {
private:
  // Bộ khóa và cảm biến của một ngăn tủ
  struct BoxUnit {
    uint8_t boxNumber;
    LockManager lock;
    SensorManager sensor;

    BoxUnit(const BoxHardware& hw)
      : boxNumber(hw.boxNumber), lock(hw.relayPin), sensor(hw.reedPin, hw.irPin) {}
  };

  BoxUnit* units[BOX_COUNT];

  BoxUnit* findUnit(int boxNumber);
  bool isAnyOtherRelayActive(const BoxUnit* current) const;

public:
  BoxController();
  void begin();
  bool hasBox(int boxNumber);
  bool unlock(int boxNumber, unsigned long durationMs);
  bool isDoorClosed(int boxNumber);
  bool hasItem(int boxNumber);
  void update();
  void pollSensors(std::function<void(int boxNumber, BoxEvent event)> onEvent);
  size_t count() const { return BOX_COUNT; }
  int boxNumberAt(size_t index) const;

#ifdef BOX_SELFTEST
  void runRelaySelfTest();
  void printSensorSnapshot();
#endif
};
