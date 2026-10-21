#pragma once
#include <Arduino.h>

// Kết quả xác thực mã OTP nhận hàng từ máy chủ
struct OtpVerifyResult {
  bool success;
  int boxNumber;
  String errorMessage;
  bool requiresPayment;
  int feeAmount;
  long orderCode;
  String qrPayload;
  String paymentUrl;
};

// Quản lý các yêu cầu gọi REST API tới máy chủ Backend
class ApiClient {
public:
  // Gửi mã OTP và mã trạm tủ lên máy chủ Backend để xác thực mở khóa bưu kiện
  static OtpVerifyResult verifyPickupOtp(const char* lockerCode, const char* pinCode);
};
