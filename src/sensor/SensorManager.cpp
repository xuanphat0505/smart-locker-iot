#include "SensorManager.h"

// Khởi tạo cảm biến với các chân GPIO tương ứng
SensorManager::SensorManager(uint8_t reed, uint8_t ir)
  : reedPin(reed), irPin(ir), lastDoorClosed(true), lastHasItem(false), lastCheckTime(0), lastIrCheckTime(0) {}

// Cấu hình chân công tắc từ có điện trở kéo lên, chân hồng ngoại để đầu vào thường vì module tự kéo mức
void SensorManager::begin() {
  pinMode(reedPin, INPUT_PULLUP);
  pinMode(irPin, INPUT);
  lastDoorClosed = isDoorClosed();
  lastHasItem = hasItem();
}

// Kiểm tra trạng thái đóng cửa qua công tắc từ
bool SensorManager::isDoorClosed() {
  return digitalRead(reedPin) == LOW;
}

// Kiểm tra trạng thái có vật thể trong ngăn qua cảm biến hồng ngoại
bool SensorManager::hasItem() {
  return digitalRead(irPin) == LOW;
}

// Phát hiện khoảnh khắc cửa vừa được đóng kín lại
bool SensorManager::checkDoorJustClosed() {
  bool currentClosed = false;
  if (checkDoorStateChanged(currentClosed)) {
    return currentClosed;
  }
  return false;
}

// Phát hiện thay đổi trạng thái đóng mở của cánh cửa ngăn tủ
bool SensorManager::checkDoorStateChanged(bool& currentClosed) {
  if (millis() - lastCheckTime < 300) {
    return false;
  }
  lastCheckTime = millis();
  bool closed = isDoorClosed();
  bool changed = (closed != lastDoorClosed);
  if (changed) {
    lastDoorClosed = closed;
    currentClosed = closed;
  }
  return changed;
}

// Phát hiện thay đổi trạng thái có hàng hay trống của ngăn tủ
bool SensorManager::checkItemStateChanged(bool& currentItem) {
  if (millis() - lastIrCheckTime < 500) {
    return false;
  }
  lastIrCheckTime = millis();
  bool item = hasItem();
  bool changed = (item != lastHasItem);
  if (changed) {
    lastHasItem = item;
    currentItem = item;
  }
  return changed;
}
