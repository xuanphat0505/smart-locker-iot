#include "MqttManager.h"
#include "../config/Config.h"
#include "../display/DisplayManager.h"

extern DisplayManager displayManager;

static MqttManager* instance = nullptr;

// Chuyển tiếp callback tĩnh từ thư viện PubSubClient vào phương thức của đối tượng
static void staticCallback(char* topic, byte* payload, unsigned int length) {
  if (instance) {
    instance->handleMessage(topic, payload, length);
  }
}

// Khởi tạo đối tượng quản lý giao thức MQTT kết nối với các module liên quan
MqttManager::MqttManager(BoxController& boxes)
  : mqttClient(secureClient), boxController(boxes) {
  instance = this;
  controlTopic = String("smartlocker/") + LOCKER_CODE + "/control";
  statusTopic = String("smartlocker/") + LOCKER_CODE + "/status";
}

// Thiết lập chứng thực bảo mật TLS và thông tin broker HiveMQ Cloud
void MqttManager::begin() {
  secureClient.setInsecure();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(staticCallback);
  mqttClient.setBufferSize(1536);
}

// Xử lý bản tin điều khiển nhận được từ Broker
void MqttManager::handleMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("[MQTT_IN] Nhận bản tin từ: ");
  Serial.println(topic);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.println("[JSON] Lỗi phân tích cú pháp dữ liệu");
    return;
  }

  const char* command = doc["command"];
  int targetBox = doc["boxNumber"] | 0;
  unsigned long duration = doc["durationMs"] | 5000;

  if (command && (strcmp(command, "UNLOCK") == 0 || strcmp(command, "OPEN_DOOR") == 0)) {
    Serial.printf("[CMD] Nhận lệnh mở Ngăn #%d -> Kích hoạt Rơ-le mở chốt Solenoid\n", targetBox);
    if (!boxController.unlock(targetBox, duration)) {
      Serial.printf("[CMD] Không mở được Ngăn #%d (số ngăn không hợp lệ hoặc ngăn khác đang mở)\n", targetBox);
      return;
    }
    displayManager.showSuccess(targetBox);
    publishStatus("DOOR_UNLOCKED", targetBox);
  } else if (command && strcmp(command, "PAYMENT_REQUIRED") == 0) {
    if (!boxController.hasBox(targetBox)) {
      Serial.printf("[CMD] Bỏ qua yêu cầu thanh toán: Ngăn #%d không hợp lệ\n", targetBox);
      return;
    }
    int feeAmount = doc["feeAmount"] | 0;
    long orderCode = doc["orderCode"] | 0;
    const char* qrPayload = doc["qrPayload"] | doc["paymentUrl"] | "";
    const char* message = doc["message"] | "Vui long thanh toan phi mo tu";

    Serial.printf("[CMD] Nhận yêu cầu thanh toán Ngăn #%d - Cước phí: %d VNĐ - Order #%ld\n",
                  targetBox, feeAmount, orderCode);
    displayManager.showPaymentQr(targetBox, feeAmount, orderCode, qrPayload, message);
  }
}

// Báo cáo trạng thái lên Broker, bản tin gắn số ngăn mang trạng thái của đúng ngăn đó, bản tin chung số ngăn bằng 0 kèm ảnh chụp cả các ngăn
void MqttManager::publishStatus(const char* eventType, int boxNumber) {
  JsonDocument doc;
  doc["device"] = LOCKER_CODE;
  doc["boxNumber"] = boxNumber;
  doc["event"] = eventType;
  doc["uptimeSec"] = millis() / 1000;

  if (boxNumber > 0) {
    doc["doorStatus"] = boxController.isDoorClosed(boxNumber) ? "CLOSED" : "OPEN";
    doc["hasItem"] = boxController.hasItem(boxNumber);
  } else {
    JsonArray boxes = doc["boxes"].to<JsonArray>();
    for (size_t i = 0; i < boxController.count(); i++) {
      int box = boxController.boxNumberAt(i);
      JsonObject item = boxes.add<JsonObject>();
      item["boxNumber"] = box;
      item["doorStatus"] = boxController.isDoorClosed(box) ? "CLOSED" : "OPEN";
      item["hasItem"] = boxController.hasItem(box);
    }
  }

  char buffer[512];
  serializeJson(doc, buffer);
  mqttClient.publish(statusTopic.c_str(), buffer);
  Serial.print("[MQTT_PUB] Đã gửi: ");
  Serial.println(buffer);
}

// Tự động kết nối và đăng ký lắng nghe topic điều khiển khi mất kết nối
void MqttManager::reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Đang kết nối tới HiveMQ Cloud...");
    String clientId = String("ESP32_Locker_") + String(random(0xffff), HEX);

    // Chuẩn bị thông điệp trăn trối LWT khi thiết bị mất nguồn hoặc ngắt kết nối đột ngột
    String lwtPayload = String("{\"device\":\"") + LOCKER_CODE + String("\",\"event\":\"OFFLINE\",\"status\":\"OFFLINE\"}");

    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS, statusTopic.c_str(), 1, false, lwtPayload.c_str())) {
      Serial.println(" Thành công!");
      mqttClient.subscribe(controlTopic.c_str());
      publishStatus("ONLINE");
    } else {
      Serial.print(" Thất bại, mã lỗi rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Thử lại sau 5 giây...");
      delay(5000);
    }
  }
}

// Duy trì vòng lặp lắng nghe và xử lý bản tin MQTT kèm nhịp tim định kỳ
void MqttManager::update() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  // Định kỳ gửi nhịp tim Heartbeat mỗi 30 giây để Backend giám sát kết nối phần cứng
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat >= 30000) {
    lastHeartbeat = millis();
    publishStatus("HEARTBEAT");
  }
}
