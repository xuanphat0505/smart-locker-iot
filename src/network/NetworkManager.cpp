#include "NetworkManager.h"
#include "../config/Config.h"

// Thiết lập kết nối WiFi với hỗ trợ cố định kênh và gọi callback định kỳ
void NetworkManager::connectWifi(std::function<void()> onTick) {
  Serial.print("\n[WIFI] Đang kết nối tới: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  // Kết nối trực tiếp vào sóng 2.4GHz để tránh router tự động gộp băng tần
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 150) {
    if (onTick) {
      onTick();
    }
    delay(100);
    if (retryCount % 5 == 0) {
      Serial.print(".");
    }
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n[WIFI] Kết nối thành công! IP: ");
    Serial.println(WiFi.localIP());
    configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("[NTP] Đã kích hoạt đồng bộ thời gian thực Internet (GMT+7)");
  } else {
    Serial.println("\n[WIFI] Thử kết nối chế độ quét tự động...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

// Tự động kiểm tra và kết nối lại WiFi khi mất sóng
void NetworkManager::maintainConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }
}
