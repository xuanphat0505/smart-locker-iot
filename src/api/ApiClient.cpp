#include "ApiClient.h"
#include "../config/Config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Gửi mã OTP và mã trạm tủ lên máy chủ Backend để xác thực mở khóa bưu kiện
OtpVerifyResult ApiClient::verifyPickupOtp(const char* lockerCode, const char* pinCode) {
  OtpVerifyResult result = { false, 0, "NETWORK ERROR", false, 0, 0, "", "" };

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API] WiFi chưa được kết nối");
    result.errorMessage = "NO WIFI CONNECTION";
    return result;
  }

  String url = String(API_SERVER_URL) + "/packages/pickup/otp";
  Serial.print("[API] Đang gửi yêu cầu xác thực tới: ");
  Serial.println(url);

  HTTPClient http;
  WiFiClient client;
  WiFiClientSecure secureClient;

  if (url.startsWith("https://")) {
    secureClient.setInsecure();
    http.begin(secureClient, url);
  } else {
    http.begin(client, url);
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-api-key", LOCKER_API_KEY);
  http.addHeader("ngrok-skip-browser-warning", "true");
  http.setTimeout(4000);

  // Chuẩn bị payload JSON gửi lên máy chủ
  JsonDocument reqDoc;
  reqDoc["lockerCode"] = lockerCode;
  reqDoc["pinCode"] = pinCode;
  String requestBody;
  serializeJson(reqDoc, requestBody);

  int httpCode = http.POST(requestBody);
  Serial.printf("[API] Mã phản hồi HTTP: %d\n", httpCode);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.print("[API] Dữ liệu phản hồi: ");
    Serial.println(response);

    JsonDocument resDoc;
    DeserializationError err = deserializeJson(resDoc, response);

    if (httpCode == 200 || httpCode == 201) {
      if (resDoc["requiresPayment"] | false) {
        result.success = false;
        result.requiresPayment = true;
        result.boxNumber = resDoc["boxNumber"] | BOX_NUMBER;
        result.feeAmount = resDoc["feeAmount"] | 0;
        result.orderCode = resDoc["orderCode"] | 0;
        result.qrPayload = resDoc["qrPayload"].as<String>();
        result.paymentUrl = resDoc["paymentUrl"].as<String>();
        result.errorMessage = resDoc["message"] | "PAYMENT REQUIRED";
      } else {
        result.success = true;
        result.requiresPayment = false;
        result.boxNumber = resDoc["boxNumber"] | resDoc["package"]["boxNumber"] | BOX_NUMBER;
        result.errorMessage = "";
      }
    } else {
      result.success = false;
      String serverMsg = "";
      if (err == DeserializationError::Ok) {
        if (resDoc["message"].is<const char*>()) {
          serverMsg = resDoc["message"].as<String>();
        } else if (resDoc["error"].is<const char*>()) {
          serverMsg = resDoc["error"].as<String>();
        }
      }

      // Chuẩn hóa thông điệp lỗi ngắn gọn hiển thị vừa vặn trên màn hình LCD
      if (serverMsg.indexOf("tạm khóa") >= 0 || serverMsg.indexOf("locked") >= 0) {
        result.errorMessage = "BOX LOCKED";
      } else if (serverMsg.indexOf("đã được lấy") >= 0 || serverMsg.indexOf("already") >= 0) {
        result.errorMessage = "ALREADY PICKED";
      } else if (serverMsg.indexOf("không tồn tại") >= 0 || serverMsg.indexOf("not found") >= 0) {
        result.errorMessage = "LOCKER NOT FOUND";
      } else if (serverMsg.indexOf("48") >= 0 || serverMsg.indexOf("thu hồi") >= 0 || serverMsg.indexOf("retrieval") >= 0) {
        result.errorMessage = "OVERDUE > 48H";
      } else {
        result.errorMessage = "INVALID PIN CODE";
      }
    }
  } else {
    Serial.printf("[API] Lỗi kết nối HTTP: %s\n", http.errorToString(httpCode).c_str());
    result.errorMessage = "SERVER UNREACHABLE";
  }

  http.end();
  return result;
}
