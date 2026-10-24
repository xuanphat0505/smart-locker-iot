#include "BoxController.h"

// Dựng bộ khóa và cảm biến cho từng ngăn theo bảng cấu hình chân
BoxController::BoxController() {
  for (size_t i = 0; i < BOX_COUNT; i++) {
    units[i] = new BoxUnit(BOXES[i]);
  }
}

// Cấu hình chân relay và cảm biến của tất cả các ngăn
void BoxController::begin() {
  for (size_t i = 0; i < BOX_COUNT; i++) {
    units[i]->lock.begin();
    units[i]->sensor.begin();
    Serial.printf("[BOX] Ngăn #%d: relay GPIO %d, reed GPIO %d, IR GPIO %d\n",
                  BOXES[i].boxNumber, BOXES[i].relayPin, BOXES[i].reedPin, BOXES[i].irPin);
  }
}

// Tìm bộ khóa và cảm biến theo số ngăn, trả về null nếu số ngăn không có trong bảng cấu hình
BoxController::BoxUnit* BoxController::findUnit(int boxNumber) {
  for (size_t i = 0; i < BOX_COUNT; i++) {
    if (units[i]->boxNumber == boxNumber) {
      return units[i];
    }
  }
  return nullptr;
}

// Kiểm tra có relay của ngăn khác đang cấp điện hay không để không bao giờ mở hai khóa cùng lúc
bool BoxController::isAnyOtherRelayActive(const BoxUnit* current) const {
  for (size_t i = 0; i < BOX_COUNT; i++) {
    if (units[i] != current && units[i]->lock.isActive()) {
      return true;
    }
  }
  return false;
}

// Kiểm tra số ngăn có tồn tại trong bảng cấu hình hay không
bool BoxController::hasBox(int boxNumber) {
  return findUnit(boxNumber) != nullptr;
}

// Mở khóa ngăn theo số ngăn, từ chối khi số ngăn không hợp lệ hoặc khi ngăn khác đang được cấp điện
bool BoxController::unlock(int boxNumber, unsigned long durationMs) {
  BoxUnit* unit = findUnit(boxNumber);
  if (unit == nullptr) {
    Serial.printf("[BOX] Từ chối mở: Ngăn #%d không có trong bảng cấu hình\n", boxNumber);
    return false;
  }
  if (isAnyOtherRelayActive(unit)) {
    Serial.printf("[BOX] Từ chối mở Ngăn #%d: một ngăn khác đang được cấp điện\n", boxNumber);
    return false;
  }
  unit->lock.unlock(durationMs);
  return true;
}

// Đọc trạng thái đóng cửa của một ngăn, số ngăn không hợp lệ được coi là cửa đóng
bool BoxController::isDoorClosed(int boxNumber) {
  BoxUnit* unit = findUnit(boxNumber);
  return unit != nullptr ? unit->sensor.isDoorClosed() : true;
}

// Đọc trạng thái có hàng của một ngăn, số ngăn không hợp lệ được coi là ngăn trống
bool BoxController::hasItem(int boxNumber) {
  BoxUnit* unit = findUnit(boxNumber);
  return unit != nullptr ? unit->sensor.hasItem() : false;
}

// Cập nhật bộ đếm thời gian của mọi relay để tự ngắt điện đúng hạn
void BoxController::update() {
  for (size_t i = 0; i < BOX_COUNT; i++) {
    units[i]->lock.update();
  }
}

// Quét cảm biến của mọi ngăn và gọi callback kèm số ngăn khi trạng thái thay đổi
void BoxController::pollSensors(std::function<void(int boxNumber, BoxEvent event)> onEvent) {
  for (size_t i = 0; i < BOX_COUNT; i++) {
    int box = units[i]->boxNumber;

    bool doorClosed = false;
    if (units[i]->sensor.checkDoorStateChanged(doorClosed)) {
      onEvent(box, doorClosed ? BOX_DOOR_CLOSED : BOX_DOOR_OPENED);
    }

    bool item = false;
    if (units[i]->sensor.checkItemStateChanged(item)) {
      onEvent(box, item ? BOX_ITEM_DETECTED : BOX_ITEM_REMOVED);
    }
  }
}

// Lấy số ngăn ở vị trí chỉ định trong bảng cấu hình, trả về 0 khi vượt phạm vi
int BoxController::boxNumberAt(size_t index) const {
  return index < BOX_COUNT ? units[index]->boxNumber : 0;
}

#ifdef BOX_SELFTEST
// Cấp điện lần lượt từng relay trong thời gian ngắn để kiểm tra dây nối trên bàn thử
void BoxController::runRelaySelfTest() {
  for (size_t i = 0; i < BOX_COUNT; i++) {
    Serial.printf("[SELFTEST] Ngăn #%d relay GPIO %d\n", BOXES[i].boxNumber, BOXES[i].relayPin);
    units[i]->lock.unlock(300);
    unsigned long start = millis();
    while (millis() - start < 1000) {
      units[i]->lock.update();
      delay(10);
    }
  }
}

// In trạng thái reed và IR của mọi ngăn mỗi giây để kiểm tra cảm biến trên bàn thử
void BoxController::printSensorSnapshot() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint < 1000) {
    return;
  }
  lastPrint = millis();
  for (size_t i = 0; i < BOX_COUNT; i++) {
    Serial.printf("[SELFTEST] Ngăn #%d cửa=%s hàng=%s\n", units[i]->boxNumber,
                  units[i]->sensor.isDoorClosed() ? "ĐÓNG" : "MỞ",
                  units[i]->sensor.hasItem() ? "CÓ" : "TRỐNG");
  }
}
#endif
