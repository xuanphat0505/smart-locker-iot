#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <functional>

// Định nghĩa các trạng thái màn hình của trạm tủ thông minh
enum DisplayState {
  STATE_WIFI_CONNECTING,
  STATE_WIFI_FAILED,
  STATE_IDLE,            // Màn hình Home Menu với 3 lựa chọn (Nhập PIN, Quét QR, Face ID)
  STATE_QR,              // Màn hình hiển thị mã QR toàn màn hình khổng lồ
  STATE_KEYPAD,          // Bàn phím số cảm ứng để nhập mã PIN
  STATE_FACE_COUNTDOWN,  // Màn hình đếm ngược 3s chuẩn bị chụp khuôn mặt
  STATE_FACE_PROCESSING, // Màn hình chờ kết quả xác thực AI
  STATE_PAYMENT_QR,      // Màn hình hiển thị mã VietQR thanh toán cước phí mở tủ
  STATE_SUCCESS,
  STATE_ERROR
};

// Định nghĩa trạng thái vòng đời của mã QR động hiển thị trên LCD
enum QrStatus {
  QR_STATUS_ACTIVE,
  QR_STATUS_EXPIRED
};

// Quản lý toàn bộ giao diện tương tác Kiosk trung tâm, bàn phím cảm ứng và mã QR động
class DisplayManager {
private:
  TFT_eSPI tft;
  DisplayState currentState;
  String enteredPin;
  unsigned long stateStartTime;
  int currentUnlockedBox;
  String currentErrorMessage;
  String currentSsid;
  int dotFrame;
  unsigned long lastDotTime;
  bool wasTouched;
  unsigned long lastTouchTime;
  std::function<void(const String&)> otpSubmitCallback;

  // Quản lý thông tin thanh toán cước phí Kiosk
  int paymentBoxNumber;
  int paymentFeeAmount;
  long paymentOrderCode;
  String paymentQrPayload;
  String paymentMessage;

  // Quản lý trạng thái đếm ngược và kích hoạt chụp ảnh khuôn mặt
  int faceCountdownSec;
  unsigned long lastCountdownTick;
  std::function<void()> faceTriggerCallback;

  // Quản lý vòng đời mã QR động
  QrStatus qrStatus;
  unsigned long qrStartTime;
  String currentQrToken;
  String currentQrPayload;
  unsigned long lastQrProgressUpdate;
  const unsigned long QR_TTL_MS = 60000;

  // Sinh mã token động 6 ký tự hex từ API_KEY và thời gian thực NTP
  String generateDynamicToken(time_t epoch);

  // Vẽ mã QR toàn màn hình đang hoạt động kèm thanh đếm ngược tiến trình
  void drawActiveQr();

  // Vẽ lớp phủ che mờ khi mã QR hết hạn kèm nút chạm làm mới
  void drawExpiredQrOverlay();

  // Làm mới mã QR và đặt lại đồng hồ đếm ngược 60s
  void refreshQrCode();

  // Cập nhật thanh tiến trình đếm ngược của mã QR
  void updateIdleQrAnimation();

  // Vẽ màn hình Home Menu chính với 3 lựa chọn lớn: Nhập PIN, Quét QR và Face ID
  void drawIdleScreen();

  // Vẽ màn hình hiển thị mã QR toàn màn hình cực lớn
  void drawQrScreen();

  // Vẽ màn hình bàn phím số cảm ứng để nhập mã OTP
  void drawKeypadScreen();

  // Cập nhật hiển thị 6 ô mã OTP trên giao diện bàn phím
  void drawOtpBoxes();

  // Vẽ từng phím bấm riêng biệt trên bàn phím số
  void drawKeypadButton(int col, int row, const char* label, uint32_t bg, uint32_t fg);

  // Xử lý sự kiện chạm tại màn hình Home Menu (3 nút lựa chọn)
  void handleIdleTouch(uint16_t x, uint16_t y);

  // Xử lý sự kiện chạm tại màn hình QR toàn màn hình
  void handleQrTouch(uint16_t x, uint16_t y);

  // Xử lý sự kiện chạm bàn phím số tại màn hình nhập mã OTP
  void handleKeypadTouch(uint16_t x, uint16_t y);

  // Vẽ màn hình đếm ngược chuẩn bị chụp ảnh kèm khung ngắm và hướng dẫn
  void drawFaceCountdownScreen();

  // Cập nhật số giây đếm ngược và kích hoạt chụp ảnh khi về 0
  void updateFaceCountdownAnimation();

  // Xử lý sự kiện chạm tại màn hình đếm ngược khuôn mặt
  void handleFaceCountdownTouch(uint16_t x, uint16_t y);

  // Vẽ màn hình đang xử lý nhận diện AI sau khi chụp ảnh
  void drawFaceProcessingScreen();

  // Cập nhật animation loading khi đang chờ kết quả AI
  void updateFaceProcessingAnimation();

  // Tạo hiệu ứng chớp sáng trắng toàn màn hình mô phỏng màn trập máy ảnh
  void triggerScreenFlashEffect();

  // Vẽ giao diện thông báo mở tủ thành công
  void drawSuccessScreen();

  // Vẽ giao diện thông báo lỗi xác thực
  void drawErrorScreen();

  // Vẽ giao diện thanh toán cước phí với mã VietQR trên LCD
  void drawPaymentQrScreen();

  // Xử lý sự kiện chạm tại màn hình thanh toán cước phí
  void handlePaymentQrTouch(uint16_t x, uint16_t y);

  // Vẽ giao diện màn hình đang kết nối WiFi
  void drawWifiConnectingScreen();

  // Vẽ giao diện màn hình kết nối WiFi thất bại
  void drawWifiFailedScreen();

  // Cập nhật animation chấm loading và đếm ngược cho màn hình WiFi
  void updateWifiAnimation();

public:
  DisplayManager();

  // Khởi tạo phần cứng màn hình LCD và cấu hình cảm ứng
  void begin();

  // Vòng lặp cập nhật sự kiện chạm và đếm thời gian chuyển trạng thái
  void update();

  // Chuyển đổi trạng thái giao diện màn hình
  void setState(DisplayState newState);

  // Hiển thị màn hình đang kết nối WiFi kèm SSID đang thử kết nối
  void showWifiConnecting(const char* ssid);

  // Hiển thị màn hình kết nối WiFi thất bại kèm đếm ngược thử lại
  void showWifiFailed(const char* ssid);

  // Hiển thị thông báo mở tủ thành công kèm số ngăn mở
  void showSuccess(int boxNumber = 1);

  // Hiển thị mã QR thanh toán cước phí trên màn hình LCD kèm thông tin ngăn và số tiền
  void showPaymentQr(int boxNumber, int feeAmount, long orderCode, const char* qrPayload, const char* message = nullptr);

  // Hiển thị thông báo lỗi khi mã OTP không đúng
  void showError(const char* errorMessage = nullptr);

  // Quay trở về màn hình Home Menu ban đầu
  void showIdle();

  // Chuyển sang màn hình hiển thị mã QR toàn màn hình
  void showQrScreen();

  // Bắt đầu đếm ngược 3 giây để chụp ảnh khuôn mặt
  void startFaceCountdown();

  // Chuyển sang màn hình đang xử lý nhận diện AI
  void showFaceProcessing();

  // Đăng ký hàm xử lý khi người dùng nhập đủ 6 chữ số OTP
  void setOnOtpSubmit(std::function<void(const String&)> callback);

  // Đăng ký hàm callback khi kết thúc đếm ngược để kích hoạt xung chụp ảnh
  void setOnFaceTrigger(std::function<void()> callback);

  // Lấy trạng thái hiển thị hiện tại của màn hình
  DisplayState getCurrentState() const { return currentState; }
  int getCurrentUnlockedBox() const { return currentUnlockedBox; }
};
