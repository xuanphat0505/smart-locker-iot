#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <functional>

class NetworkManager {
public:
  // Thiết lập kết nối WiFi kèm hàm callback cập nhật giao diện định kỳ
  static void connectWifi(std::function<void()> onTick = nullptr);

  // Tự động kiểm tra và kết nối lại WiFi khi mất sóng
  static void maintainConnection();
};
