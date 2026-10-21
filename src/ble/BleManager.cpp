#include "BleManager.h"
#include <esp_bt.h>

// Khởi tạo đối tượng quản lý phát sóng BLE định danh trạm tủ
BleManager::BleManager()
  : lockerCode(""), isAdvertising(false), pServer(nullptr), pAdvertising(nullptr) {}

// Cấu hình BLE Server và kích hoạt phát sóng quảng bá beacon trạm tủ
void BleManager::begin(const char* code) {
  lockerCode = code ? code : "SMART-LOCKER";
  Serial.printf("[BLE] Đang khởi tạo Bluetooth Low Energy cho trạm: %s\n", lockerCode.c_str());

  BLEDevice::init(lockerCode.c_str());

  // Giới hạn công suất phát sóng để kiểm soát bán kính phủ sóng trong khoảng 1 đến 3 mét
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N0);

  pServer = BLEDevice::createServer();
  pAdvertising = BLEDevice::getAdvertising();

  // Đóng gói dữ liệu quảng bá bao gồm tên trạm tủ và Service UUID
  BLEAdvertisementData advertisementData;
  advertisementData.setName(lockerCode.c_str());
  advertisementData.setCompleteServices(BLEUUID(LOCKER_SERVICE_UUID));

  // Gói phản hồi quét hỗ trợ cung cấp thêm thông tin cho ứng dụng di động
  BLEAdvertisementData scanResponseData;
  scanResponseData.setName(lockerCode.c_str());

  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->setScanResponseData(scanResponseData);

  startAdvertising();
}

// Bắt đầu phát sóng quảng bá BLE beacon ra môi trường xung quanh
void BleManager::startAdvertising() {
  if (pAdvertising && !isAdvertising) {
    pAdvertising->start();
    isAdvertising = true;
    Serial.printf("[BLE] Trạm %s đã bắt đầu phát sóng quảng bá\n", lockerCode.c_str());
  }
}

// Tạm dừng phát sóng quảng bá BLE khi cần bảo trì
void BleManager::stopAdvertising() {
  if (pAdvertising && isAdvertising) {
    pAdvertising->stop();
    isAdvertising = false;
    Serial.println("[BLE] Đã tạm dừng phát sóng quảng bá BLE");
  }
}

// Kiểm tra trạng thái đang phát sóng của trạm tủ
bool BleManager::isRunning() const {
  return isAdvertising;
}
