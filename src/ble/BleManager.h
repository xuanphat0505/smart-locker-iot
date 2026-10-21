#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>

// Định nghĩa Service UUID chuẩn phục vụ định danh trạm tủ thông minh
#define LOCKER_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

class BleManager {
private:
  String lockerCode;
  bool isAdvertising;
  BLEServer* pServer;
  BLEAdvertising* pAdvertising;

public:
  BleManager();
  void begin(const char* code);
  void startAdvertising();
  void stopAdvertising();
  bool isRunning() const;
};
