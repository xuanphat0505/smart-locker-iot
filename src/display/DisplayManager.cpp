#include "DisplayManager.h"
#include "../config/Config.h"
#include <mbedtls/sha256.h>
#include <time.h>
#include <qrcode.h>

// Bảng màu giao diện Kiosk hiện đại phong cách công nghiệp tối giản
#define COLOR_BG            0x08A5  // Nền chính xanh than tối #0F172A
#define COLOR_SURFACE       0x1967  // Mặt phím than chì #1E293B
#define COLOR_SURFACE_LIGHT 0x32A6  // Mặt phím phụ #334155
#define COLOR_BORDER        0x4453  // Viền phím bấm #475569
#define COLOR_ACCENT        0x0D5D  // Màu điểm nhấn Cyan Electric #0EA5E9
#define COLOR_TEXT_ACCENT   0x3DBF  // Màu chữ điểm nhấn Sky #38BDF8
#define COLOR_SUCCESS       0x15AC  // Xanh lá báo thành công #10B981
#define COLOR_DANGER        0xED88  // Đỏ cảnh báo lỗi #EF4444
#define COLOR_TEXT_MUTED    0x9536  // Màu chữ phụ Slate 400 #94A3B8
#define COLOR_TEXT_PRIMARY  0xFFFF  // Màu chữ chính Trắng tinh
#define COLOR_WARNING       0xFD20  // Màu vàng cam cảnh báo #FF6600
#define COLOR_PURPLE        0x91F9  // Tím công nghệ cho Face ID #9333EA

// Khởi tạo đối tượng quản lý màn hình cùng các trạng thái mặc định ban đầu
DisplayManager::DisplayManager()
  : tft(TFT_eSPI()),
    currentState(STATE_WIFI_CONNECTING),
    enteredPin(""),
    stateStartTime(0),
    currentUnlockedBox(0),
    currentErrorMessage("INVALID PIN CODE"),
    currentSsid(""),
    dotFrame(0),
    lastDotTime(0),
    wasTouched(false),
    lastTouchTime(0),
    otpSubmitCallback(nullptr),
    paymentBoxNumber(0),
    paymentFeeAmount(0),
    paymentOrderCode(0),
    paymentQrPayload(""),
    paymentMessage(""),
    faceCountdownSec(FACE_COUNTDOWN_SECONDS),
    lastCountdownTick(0),
    faceTriggerCallback(nullptr),
    qrStatus(QR_STATUS_ACTIVE),
    qrStartTime(0),
    currentQrToken(""),
    currentQrPayload(""),
    lastQrProgressUpdate(0) {}

// Khởi tạo phần cứng màn hình LCD và cấu hình cảm ứng nhưng không vẽ màn hình
void DisplayManager::begin() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COLOR_BG);

  // Áp dụng thông số hiệu chuẩn cảm ứng đã căn chỉnh chuẩn xác cho màn hình 2.8 inch
  uint16_t calData[5] = {275, 3620, 264, 3532, 7};
  tft.setTouch(calData);

  Serial.println("[DISPLAY] Kiosk Display Manager initialized successfully!");
}

// Chuyển đổi trạng thái hiển thị hiện tại và vẽ lại giao diện tương ứng
void DisplayManager::setState(DisplayState newState) {
  currentState = newState;
  stateStartTime = millis();

  switch (currentState) {
    case STATE_WIFI_CONNECTING:
      dotFrame = 0;
      lastDotTime = millis();
      drawWifiConnectingScreen();
      break;
    case STATE_WIFI_FAILED:
      drawWifiFailedScreen();
      break;
    case STATE_IDLE:
      enteredPin = "";
      drawIdleScreen();
      break;
    case STATE_QR:
      drawQrScreen();
      break;
    case STATE_KEYPAD:
      enteredPin = "";
      drawKeypadScreen();
      break;
    case STATE_FACE_COUNTDOWN:
      faceCountdownSec = FACE_COUNTDOWN_SECONDS;
      lastCountdownTick = millis();
      drawFaceCountdownScreen();
      break;
    case STATE_FACE_PROCESSING:
      drawFaceProcessingScreen();
      break;
    case STATE_PAYMENT_QR:
      drawPaymentQrScreen();
      break;
    case STATE_SUCCESS:
      drawSuccessScreen();
      break;
    case STATE_ERROR:
      drawErrorScreen();
      break;
  }
}

// Hiển thị màn hình đang kết nối WiFi kèm SSID đang thử kết nối
void DisplayManager::showWifiConnecting(const char* ssid) {
  currentSsid = ssid ? ssid : "";
  setState(STATE_WIFI_CONNECTING);
}

// Hiển thị màn hình kết nối WiFi thất bại kèm đếm ngược thử lại
void DisplayManager::showWifiFailed(const char* ssid) {
  currentSsid = ssid ? ssid : "";
  setState(STATE_WIFI_FAILED);
}

// Chuyển nhanh màn hình về chế độ chờ mặc định
void DisplayManager::showIdle() {
  setState(STATE_IDLE);
}

// Chuyển sang màn hình hiển thị mã QR toàn màn hình
void DisplayManager::showQrScreen() {
  setState(STATE_QR);
}

// Bắt đầu đếm ngược 3 giây để chụp ảnh khuôn mặt
void DisplayManager::startFaceCountdown() {
  setState(STATE_FACE_COUNTDOWN);
}

// Chuyển sang màn hình đang xử lý nhận diện AI
void DisplayManager::showFaceProcessing() {
  setState(STATE_FACE_PROCESSING);
}

// Lưu hàm callback kích hoạt xung chụp ảnh sau khi đếm ngược kết thúc
void DisplayManager::setOnFaceTrigger(std::function<void()> callback) {
  faceTriggerCallback = callback;
}

// Kích hoạt hiển thị màn hình mở tủ thành công cùng số ngăn tương ứng
void DisplayManager::showSuccess(int boxNumber) {
  currentUnlockedBox = boxNumber;
  setState(STATE_SUCCESS);
}

// Kích hoạt hiển thị màn hình thanh toán cước phí mở tủ kèm mã QR
void DisplayManager::showPaymentQr(int boxNumber, int feeAmount, long orderCode, const char* qrPayload, const char* message) {
  paymentBoxNumber = boxNumber > 0 ? boxNumber : 0;
  paymentFeeAmount = feeAmount;
  paymentOrderCode = orderCode;
  paymentQrPayload = qrPayload ? qrPayload : "";
  paymentMessage = message ? message : "Please pay the fee to unlock";
  setState(STATE_PAYMENT_QR);
}

// Kích hoạt hiển thị màn hình thông báo lỗi khi xác thực thất bại
void DisplayManager::showError(const char* errorMessage) {
  if (errorMessage != nullptr && strlen(errorMessage) > 0) {
    currentErrorMessage = errorMessage;
  } else {
    currentErrorMessage = "INVALID PIN CODE";
  }
  setState(STATE_ERROR);
}

// Lưu hàm callback nhận kết quả khi người dùng nhập đủ 6 ký tự mã OTP
void DisplayManager::setOnOtpSubmit(std::function<void(const String&)> callback) {
  otpSubmitCallback = callback;
}

// Vẽ giao diện màn hình đang kết nối WiFi tối giản: icon nhỏ, 2 dòng thông tin
void DisplayManager::drawWifiConnectingScreen() {
  tft.fillScreen(COLOR_BG);

  // WiFi icon gọn: 2 cung tròn + chấm trung tâm, căn giữa màn hình
  const int cx = 160;
  const int cy = 88;
  tft.fillCircle(cx, cy, 4, COLOR_ACCENT);
  tft.drawCircle(cx, cy, 14, COLOR_ACCENT);
  tft.drawCircle(cx, cy, 24, COLOR_ACCENT);

  // Che phần dưới icon để tạo hình bán nguyệt WiFi
  tft.fillRect(cx - 28, cy + 1, 56, 30, COLOR_BG);

  // Dòng chính animated - chứa trạng thái kết nối + chấm loading
  // Vùng này sẽ được xóa và vẽ lại bởi updateWifiAnimation mỗi 600ms
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
  tft.drawCentreString("Connecting WiFi .", 160, 126, 2);

  // SSID hiển thị nhỏ, mờ nhẹ bên dưới
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawCentreString(currentSsid.c_str(), 160, 152, 2);
}

// Cập nhật animation bằng cách thêm dấu chấm vào dòng chữ chính mỗi 600ms
void DisplayManager::updateWifiAnimation() {
  if (millis() - lastDotTime < 600) return;
  lastDotTime = millis();
  dotFrame = (dotFrame + 1) % 3;

  if (currentState == STATE_WIFI_CONNECTING) {
    // Xóa chỉ vùng dòng text animation để tránh flicker toàn màn hình
    tft.fillRect(0, 124, 320, 20, COLOR_BG);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);

    const char* frames[] = {"Connecting WiFi .", "Connecting WiFi ..", "Connecting WiFi ..."};
    tft.drawCentreString(frames[dotFrame], 160, 126, 2);
  } else if (currentState == STATE_WIFI_FAILED) {
    // Cập nhật dòng đếm ngược retry
    unsigned long elapsed = millis() - stateStartTime;
    int remaining = 5 - (int)(elapsed / 1000);
    if (remaining < 0) remaining = 0;

    tft.fillRect(0, 168, 320, 18, COLOR_BG);
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    char countdown[28];
    snprintf(countdown, sizeof(countdown), "Retry in %ds...", remaining);
    tft.drawCentreString(countdown, 160, 170, 2);
  }
}

// Vẽ giao diện màn hình kết nối WiFi thất bại tối giản
void DisplayManager::drawWifiFailedScreen() {
  tft.fillScreen(COLOR_BG);

  // WiFi icon mờ với dấu X chỉ lỗi, căn giữa màn hình
  const int cx = 160;
  const int cy = 88;
  tft.drawCircle(cx, cy, 14, COLOR_TEXT_MUTED);
  tft.drawCircle(cx, cy, 24, COLOR_TEXT_MUTED);
  tft.fillRect(cx - 28, cy + 1, 56, 30, COLOR_BG);

  // Chấm đỏ trung tâm và đường gạch X đơn giản
  tft.fillCircle(cx, cy, 4, COLOR_DANGER);
  tft.drawLine(cx - 18, cy - 18, cx + 18, cy + 18, COLOR_DANGER);
  tft.drawLine(cx - 17, cy - 18, cx + 19, cy + 18, COLOR_DANGER);

  // Dòng trạng thái chính
  tft.setTextColor(COLOR_DANGER, COLOR_BG);
  tft.drawCentreString("Connection failed", 160, 128, 2);

  // SSID mờ bên dưới
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawCentreString(currentSsid.c_str(), 160, 149, 2);

  // Dòng đếm ngược retry (sẽ được cập nhật bởi updateWifiAnimation)
  tft.drawCentreString("Retry in 5s...", 160, 170, 2);
}

// Sinh mã token động 6 ký tự hex từ API_KEY và chu kỳ 60 giây thời gian thực NTP
String DisplayManager::generateDynamicToken(time_t epoch) {
  unsigned long window = (unsigned long)(epoch / 60);
  String payload = String(LOCKER_API_KEY) + String(LOCKER_CODE) + String(window);

  byte shaResult[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, (const unsigned char*)payload.c_str(), payload.length());
  mbedtls_sha256_finish(&ctx, shaResult);
  mbedtls_sha256_free(&ctx);

  char hexBuf[7];
  snprintf(hexBuf, sizeof(hexBuf), "%02X%02X%02X", shaResult[0], shaResult[1], shaResult[2]);
  return String(hexBuf);
}

// Vẽ màn hình hiển thị mã QR toàn màn hình cực lớn
void DisplayManager::drawQrScreen() {
  tft.fillScreen(COLOR_BG);

  // Nút quay lại trang chủ ở góc trên bên trái
  tft.fillRoundRect(8, 4, 82, 24, 4, COLOR_SURFACE);
  tft.drawRoundRect(8, 4, 82, 24, 4, COLOR_BORDER);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_SURFACE);
  tft.drawCentreString("< BACK", 49, 8, 2);

  // Tiêu đề màn hình quét mã QR
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
  tft.drawCentreString("SCAN QR TO UNLOCK", 185, 8, 2);
  tft.drawFastHLine(0, 31, 320, COLOR_SURFACE);

  drawActiveQr();
}

// Vẽ mã QR toàn màn hình đang hoạt động kèm thanh đếm ngược tiến trình
void DisplayManager::drawActiveQr() {
  qrStatus = QR_STATUS_ACTIVE;
  qrStartTime = millis();
  lastQrProgressUpdate = millis();

  time_t now = time(nullptr);
  if (now < 100000) {
    now = 1711000000 + (millis() / 1000);
  }

  currentQrToken = generateDynamicToken(now);
  currentQrPayload = String(LOCKER_CODE) + ":" + currentQrToken;

  Serial.printf("[QR] Sinh ma QR dong moi: %s (Token: %s)\n", currentQrPayload.c_str(), currentQrToken.c_str());

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, currentQrPayload.c_str());

  const int scale = 5;
  const int margin = 18;
  const int totalSize = qrcode.size * scale + margin * 2;
  const int boxX = (320 - totalSize) / 2;
  const int boxY = 34;

  // Thẻ trắng chứa mã QR kích thước lớn
  tft.fillRoundRect(boxX - 2, boxY - 2, totalSize + 4, totalSize + 4, 6, TFT_WHITE);

  // Vẽ các module đen của mã QR
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      uint16_t color = qrcode_getModule(&qrcode, x, y) ? TFT_BLACK : TFT_WHITE;
      tft.fillRect(boxX + margin + x * scale, boxY + margin + y * scale, scale, scale, color);
    }
  }

  // Khởi tạo thanh tiến trình thời gian sống 60 giây của mã QR
  tft.fillRect(60, 218, 200, 22, COLOR_BG);
  tft.fillRoundRect(70, 218, 180, 4, 2, COLOR_SURFACE_LIGHT);
  tft.fillRoundRect(70, 218, 180, 4, 2, COLOR_SUCCESS);

  // Hiển thị thông tin mã token và gợi ý thao tác
  char tokenLabel[40];
  snprintf(tokenLabel, sizeof(tokenLabel), "CODE: %s (60s) - TAP TO REFRESH", currentQrToken.c_str());
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawCentreString(tokenLabel, 160, 225, 1);
}

// Vẽ lớp phủ che mờ khi mã QR hết hạn kèm nút chạm làm mới
void DisplayManager::drawExpiredQrOverlay() {
  qrStatus = QR_STATUS_EXPIRED;
  Serial.println("[QR] Ma QR da het han (60s) -> Hien thi overlay lam mo");

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, currentQrPayload.c_str());

  const int scale = 5;
  const int margin = 18;
  const int totalSize = qrcode.size * scale + margin * 2;
  const int boxX = (320 - totalSize) / 2;
  const int boxY = 34;

  // Vẽ nền thẻ tối màu cho mã QR bị làm mờ
  tft.fillRoundRect(boxX - 2, boxY - 2, totalSize + 4, totalSize + 4, 6, COLOR_SURFACE);
  tft.drawRoundRect(boxX - 2, boxY - 2, totalSize + 4, totalSize + 4, 6, COLOR_BORDER);

  // Vẽ các module QR ở chế độ mờ ảo tương phản thấp
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      uint16_t color = qrcode_getModule(&qrcode, x, y) ? COLOR_BORDER : COLOR_SURFACE;
      tft.fillRect(boxX + margin + x * scale, boxY + margin + y * scale, scale, scale, color);
    }
  }

  // Vẽ các đường kẻ scanline mờ tạo hiệu ứng kính mờ vô hiệu hóa camera
  for (int sy = boxY; sy < boxY + totalSize; sy += 3) {
    tft.drawFastHLine(boxX, sy, totalSize, COLOR_BG);
  }

  // Thẻ kính mờ nổi bật ở chính giữa mã QR
  const int cardW = 136;
  const int cardH = 76;
  const int cardX = 160 - cardW / 2;
  const int cardY = 124 - cardH / 2;

  tft.fillRoundRect(cardX, cardY, cardW, cardH, 6, COLOR_SURFACE);
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 6, COLOR_ACCENT);

  // Biểu tượng xoay tròn làm mới ở tâm trên của thẻ nổi
  const int icX = 160;
  const int icY = cardY + 20;
  tft.drawCircle(icX, icY, 11, COLOR_ACCENT);
  tft.drawCircle(icX, icY, 10, COLOR_ACCENT);
  tft.fillTriangle(icX + 8, icY - 5, icX + 13, icY + 3, icX + 5, icY + 1, COLOR_ACCENT);
  tft.fillRect(icX + 3, icY - 13, 11, 8, COLOR_SURFACE);

  // Dòng trạng thái hết hạn và hướng dẫn chạm làm mới
  tft.setTextColor(COLOR_WARNING, COLOR_SURFACE);
  tft.drawCentreString("QR EXPIRED", 160, cardY + 36, 2);

  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_SURFACE);
  tft.drawCentreString("TAP TO REFRESH", 160, cardY + 54, 2);

  // Cập nhật thanh tiến trình sang màu đỏ cảnh báo và hiển thị hướng dẫn chân trang
  tft.fillRect(60, 218, 200, 22, COLOR_BG);
  tft.fillRoundRect(70, 218, 180, 4, 2, COLOR_SURFACE_LIGHT);
  tft.fillRoundRect(70, 218, 180, 4, 2, COLOR_DANGER);

  tft.setTextColor(COLOR_WARNING, COLOR_BG);
  tft.drawCentreString("Code expired - Tap anywhere to refresh", 160, 225, 1);
}

// Làm mới mã QR và đặt lại đồng hồ đếm ngược 60s
void DisplayManager::refreshQrCode() {
  Serial.println("[QR] Dang lam moi ma QR...");
  drawActiveQr();
}

// Cập nhật thanh tiến trình đếm ngược của mã QR
void DisplayManager::updateIdleQrAnimation() {
  if (currentState != STATE_QR) return;

  if (qrStatus == QR_STATUS_ACTIVE) {
    unsigned long elapsed = millis() - qrStartTime;

    if (elapsed >= QR_TTL_MS) {
      drawExpiredQrOverlay();
      return;
    }

    if (millis() - lastQrProgressUpdate >= 1000) {
      lastQrProgressUpdate = millis();

      int remainingMs = QR_TTL_MS - elapsed;
      if (remainingMs < 0) remainingMs = 0;

      int barWidth = (remainingMs * 180) / QR_TTL_MS;

      tft.fillRect(70 + barWidth, 218, 180 - barWidth, 4, COLOR_SURFACE_LIGHT);
      if (barWidth > 0) {
        uint16_t barColor = (remainingMs < 15000) ? COLOR_WARNING : COLOR_SUCCESS;
        tft.fillRect(70, 218, barWidth, 4, barColor);
      }

      int remainingSec = (remainingMs + 999) / 1000;
      tft.fillRect(40, 225, 240, 15, COLOR_BG);
      char tokenLabel[40];
      snprintf(tokenLabel, sizeof(tokenLabel), "CODE: %s (%ds) - TAP TO REFRESH", currentQrToken.c_str(), remainingSec);
      tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
      tft.drawCentreString(tokenLabel, 160, 225, 1);
    }
  }
}

// Vẽ màn hình Home Menu chính với 3 lựa chọn lớn: Nhập PIN, Quét QR và Face ID
void DisplayManager::drawIdleScreen() {
  tft.fillScreen(COLOR_BG);

  // Thanh trạng thái trên cùng
  tft.fillCircle(18, 14, 4, COLOR_SUCCESS);
  tft.setTextColor(COLOR_SUCCESS, COLOR_BG);
  tft.drawString("READY", 28, 7, 2);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawString(LOCKER_CODE, 200, 7, 2);
  tft.drawFastHLine(0, 27, 320, COLOR_SURFACE_LIGHT);

  // Tiêu đề chào mừng và hướng dẫn
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
  tft.drawCentreString("SMART LOCKER", 160, 34, 4);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawCentreString("Choose pickup method", 160, 58, 2);

  // Thẻ 1: Nhập mã PIN (trái)
  tft.fillRoundRect(10, 78, 94, 152, 6, COLOR_SURFACE);
  tft.drawRoundRect(10, 78, 94, 152, 6, COLOR_BORDER);

  const int kx = 42, ky = 90;
  tft.drawRoundRect(kx, ky, 30, 24, 3, COLOR_ACCENT);
  tft.fillCircle(kx + 7, ky + 7, 2, COLOR_TEXT_ACCENT);
  tft.fillCircle(kx + 15, ky + 7, 2, COLOR_TEXT_ACCENT);
  tft.fillCircle(kx + 23, ky + 7, 2, COLOR_TEXT_ACCENT);
  tft.fillCircle(kx + 7, ky + 16, 2, COLOR_TEXT_ACCENT);
  tft.fillCircle(kx + 15, ky + 16, 2, COLOR_TEXT_ACCENT);
  tft.fillCircle(kx + 23, ky + 16, 2, COLOR_TEXT_ACCENT);

  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_SURFACE);
  tft.drawCentreString("PIN CODE", 57, 124, 2);

  tft.fillRoundRect(18, 152, 78, 24, 4, COLOR_ACCENT);
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_ACCENT);
  tft.drawCentreString("ENTER", 57, 156, 2);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_SURFACE);
  tft.drawCentreString("Keypad", 57, 186, 1);

  // Thẻ 2: Quét mã QR (giữa)
  tft.fillRoundRect(113, 78, 94, 152, 6, COLOR_SURFACE);
  tft.drawRoundRect(113, 78, 94, 152, 6, COLOR_BORDER);

  const int qx = 145, qy = 90;
  tft.drawRoundRect(qx, qy, 30, 24, 3, COLOR_ACCENT);
  tft.drawRect(qx + 4, qy + 4, 8, 8, COLOR_TEXT_ACCENT);
  tft.fillRect(qx + 6, qy + 6, 4, 4, COLOR_TEXT_ACCENT);
  tft.drawRect(qx + 18, qy + 4, 8, 8, COLOR_TEXT_ACCENT);
  tft.fillRect(qx + 20, qy + 6, 4, 4, COLOR_TEXT_ACCENT);
  tft.drawRect(qx + 4, qy + 12, 8, 8, COLOR_TEXT_ACCENT);
  tft.fillRect(qx + 6, qy + 14, 4, 4, COLOR_TEXT_ACCENT);

  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_SURFACE);
  tft.drawCentreString("SCAN QR", 160, 124, 2);

  tft.fillRoundRect(121, 152, 78, 24, 4, COLOR_ACCENT);
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_ACCENT);
  tft.drawCentreString("SHOW QR", 160, 156, 2);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_SURFACE);
  tft.drawCentreString("Mobile app", 160, 186, 1);

  // Thẻ 3: Quét khuôn mặt Face ID (phải)
  tft.fillRoundRect(216, 78, 94, 152, 6, COLOR_SURFACE);
  tft.drawRoundRect(216, 78, 94, 152, 6, COLOR_BORDER);

  const int fx = 248, fy = 90;
  tft.drawFastHLine(fx, fy, 7, COLOR_TEXT_ACCENT);
  tft.drawFastVLine(fx, fy, 7, COLOR_TEXT_ACCENT);
  tft.drawFastHLine(fx + 23, fy, 7, COLOR_TEXT_ACCENT);
  tft.drawFastVLine(fx + 29, fy, 7, COLOR_TEXT_ACCENT);
  tft.drawFastHLine(fx, fy + 23, 7, COLOR_TEXT_ACCENT);
  tft.drawFastVLine(fx, fy + 17, 7, COLOR_TEXT_ACCENT);
  tft.drawFastHLine(fx + 23, fy + 23, 7, COLOR_TEXT_ACCENT);
  tft.drawFastVLine(fx + 29, fy + 17, 7, COLOR_TEXT_ACCENT);

  tft.fillCircle(fx + 10, fy + 9, 2, COLOR_TEXT_ACCENT);
  tft.fillCircle(fx + 20, fy + 9, 2, COLOR_TEXT_ACCENT);
  tft.drawFastHLine(fx + 11, fy + 16, 8, COLOR_TEXT_ACCENT);

  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_SURFACE);
  tft.drawCentreString("FACE ID", 263, 124, 2);

  tft.fillRoundRect(224, 152, 78, 24, 4, COLOR_ACCENT);
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_ACCENT);
  tft.drawCentreString("SCAN FACE", 263, 156, 2);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_SURFACE);
  tft.drawCentreString("Facial ID", 263, 186, 1);
}

// Vẽ giao diện toàn bộ màn hình bàn phím số cảm ứng
void DisplayManager::drawKeypadScreen() {
  tft.fillScreen(COLOR_BG);

  // Thanh điều hướng phía trên gồm nút hủy thao tác và tiêu đề
  tft.fillRoundRect(8, 4, 76, 24, 4, COLOR_SURFACE);
  tft.drawRoundRect(8, 4, 76, 24, 4, COLOR_BORDER);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_SURFACE);
  tft.drawCentreString("< CANCEL", 46, 8, 2);

  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
  tft.drawCentreString("ENTER PICKUP CODE", 200, 8, 2);
  tft.drawFastHLine(0, 31, 320, COLOR_SURFACE);

  // Vẽ 6 ô hiển thị các ký tự mã PIN
  drawOtpBoxes();

  // Vẽ 12 phím bấm số và chức năng theo bố cục lưới 3x4 hiện đại
  drawKeypadButton(0, 0, "1", COLOR_SURFACE, COLOR_TEXT_PRIMARY);
  drawKeypadButton(1, 0, "2", COLOR_SURFACE, COLOR_TEXT_PRIMARY);
  drawKeypadButton(2, 0, "3", COLOR_SURFACE, COLOR_TEXT_PRIMARY);

  drawKeypadButton(0, 1, "4", COLOR_SURFACE, COLOR_TEXT_PRIMARY);
  drawKeypadButton(1, 1, "5", COLOR_SURFACE, COLOR_TEXT_PRIMARY);
  drawKeypadButton(2, 1, "6", COLOR_SURFACE, COLOR_TEXT_PRIMARY);

  drawKeypadButton(0, 2, "7", COLOR_SURFACE, COLOR_TEXT_PRIMARY);
  drawKeypadButton(1, 2, "8", COLOR_SURFACE, COLOR_TEXT_PRIMARY);
  drawKeypadButton(2, 2, "9", COLOR_SURFACE, COLOR_TEXT_PRIMARY);

  drawKeypadButton(0, 3, "C", COLOR_SURFACE_LIGHT, COLOR_TEXT_MUTED);
  drawKeypadButton(1, 3, "0", COLOR_SURFACE, COLOR_TEXT_PRIMARY);
  drawKeypadButton(2, 3, "DEL", COLOR_SURFACE_LIGHT, COLOR_TEXT_MUTED);
}

// Cập nhật lại 6 ô hiển thị mã số OTP khi người dùng bấm phím hoặc xóa
void DisplayManager::drawOtpBoxes() {
  const int boxWidth = 32;
  const int boxHeight = 28;
  const int boxGap = 12;
  const int startX = (320 - (6 * boxWidth + 5 * boxGap)) / 2;
  const int startY = 37;

  for (int i = 0; i < 6; i++) {
    int currentX = startX + i * (boxWidth + boxGap);
    uint16_t boxBg = COLOR_SURFACE;
    uint16_t boxBorder = COLOR_SURFACE_LIGHT;

    if (i < (int)enteredPin.length()) {
      boxBorder = COLOR_ACCENT;
      tft.fillRoundRect(currentX, startY, boxWidth, boxHeight, 4, boxBg);
      tft.drawRoundRect(currentX, startY, boxWidth, boxHeight, 4, boxBorder);

      char displayChar[2] = { enteredPin[i], '\0' };
      tft.setTextColor(COLOR_TEXT_PRIMARY, boxBg);
      tft.drawCentreString(displayChar, currentX + boxWidth / 2, startY + 5, 4);
    } else if (i == (int)enteredPin.length()) {
      boxBorder = COLOR_TEXT_PRIMARY;
      tft.fillRoundRect(currentX, startY, boxWidth, boxHeight, 4, boxBg);
      tft.drawRoundRect(currentX, startY, boxWidth, boxHeight, 4, boxBorder);

      tft.setTextColor(COLOR_TEXT_MUTED, boxBg);
      tft.drawCentreString("_", currentX + boxWidth / 2, startY + 2, 4);
    } else {
      tft.fillRoundRect(currentX, startY, boxWidth, boxHeight, 4, boxBg);
      tft.drawRoundRect(currentX, startY, boxWidth, boxHeight, 4, boxBorder);
    }
  }
}

// Vẽ một phím bấm riêng lẻ theo cột, hàng và màu sắc chỉ định
void DisplayManager::drawKeypadButton(int col, int row, const char* label, uint32_t bg, uint32_t fg) {
  const int startX = 36;
  const int startY = 74;
  const int keyWidth = 76;
  const int keyHeight = 36;
  const int gapX = 10;
  const int gapY = 6;

  int x = startX + col * (keyWidth + gapX);
  int y = startY + row * (keyHeight + gapY);

  tft.fillRoundRect(x, y, keyWidth, keyHeight, 6, bg);
  tft.drawRoundRect(x, y, keyWidth, keyHeight, 6, COLOR_BORDER);
  tft.setTextColor(fg, bg);

  // Phím chữ nhiều ký tự như DEL hiển thị font 2 để cân đối kích thước
  if (strlen(label) > 1) {
    tft.drawCentreString(label, x + keyWidth / 2, y + 10, 2);
  } else {
    tft.drawCentreString(label, x + keyWidth / 2, y + 8, 4);
  }
}

// Vẽ giao diện thông báo xác thực thành công và số ngăn tủ được mở
void DisplayManager::drawSuccessScreen() {
  tft.fillScreen(COLOR_BG);

  // Biểu tượng xác nhận hoàn tất
  tft.fillCircle(160, 44, 20, COLOR_SUCCESS);
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_SUCCESS);
  tft.drawCentreString("OK", 160, 34, 4);

  tft.setTextColor(COLOR_SUCCESS, COLOR_BG);
  tft.drawCentreString("ACCESS GRANTED", 160, 74, 4);

  // Thẻ thông tin ngăn tủ đang mở
  tft.fillRoundRect(26, 104, 268, 68, 6, COLOR_SURFACE);
  tft.drawRoundRect(26, 104, 268, 68, 6, COLOR_SUCCESS);

  char boxInfo[32];
  snprintf(boxInfo, sizeof(boxInfo), "BOX #%02d OPENED", currentUnlockedBox);
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_SURFACE);
  tft.drawCentreString(boxInfo, 160, 114, 4);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_SURFACE);
  tft.drawCentreString("Please take your parcel & close door", 160, 146, 2);

  // Dòng chú thích tự động kết thúc ở chân trang
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawCentreString("Locker door will auto-lock upon closing", 160, 196, 2);
}

// Vẽ giao diện thông báo lỗi khi mã PIN không hợp lệ
void DisplayManager::drawErrorScreen() {
  tft.fillScreen(COLOR_BG);

  // Biểu tượng từ chối truy cập
  tft.fillCircle(160, 44, 20, COLOR_DANGER);
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_DANGER);
  tft.drawCentreString("X", 160, 34, 4);

  tft.setTextColor(COLOR_DANGER, COLOR_BG);
  tft.drawCentreString("ACCESS DENIED", 160, 74, 4);

  // Thẻ hiển thị nguyên nhân lỗi
  tft.fillRoundRect(26, 104, 268, 68, 6, COLOR_SURFACE);
  tft.drawRoundRect(26, 104, 268, 68, 6, COLOR_DANGER);

  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_SURFACE);
  if (currentErrorMessage.length() > 16) {
    tft.drawCentreString(currentErrorMessage.c_str(), 160, 118, 2);
  } else {
    tft.drawCentreString(currentErrorMessage.c_str(), 160, 114, 4);
  }

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_SURFACE);
  if (currentErrorMessage.indexOf("48") >= 0 || currentErrorMessage.indexOf("OVERDUE") >= 0) {
    tft.drawCentreString("Please contact Management Office", 160, 146, 2);
  } else {
    tft.drawCentreString("Please check your app and try again", 160, 146, 2);
  }

  // Dòng thông báo chuẩn bị quay lại bàn phím
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawCentreString("Returning to home screen shortly...", 160, 196, 2);
}

// Xử lý sự kiện chạm tại màn hình Home Menu (3 nút lựa chọn: PIN, QR, Face ID)
void DisplayManager::handleIdleTouch(uint16_t x, uint16_t y) {
  if (y < 70) return;
  if (x < 107) {
    Serial.printf("[DISPLAY] Cham nut PIN (%d, %d) -> Mo ban phim PIN\n", x, y);
    setState(STATE_KEYPAD);
  } else if (x < 212) {
    Serial.printf("[DISPLAY] Cham nut QR (%d, %d) -> Mo man hinh QR toan man hinh\n", x, y);
    showQrScreen();
  } else {
    Serial.printf("[DISPLAY] Cham nut FACE ID (%d, %d) -> Bat dau dem nguoc chup khuon mat\n", x, y);
    startFaceCountdown();
  }
}

// Xử lý sự kiện chạm tại màn hình QR toàn màn hình
void DisplayManager::handleQrTouch(uint16_t x, uint16_t y) {
  // Nút quay lại ở góc trên bên trái
  if (x <= 95 && y <= 35) {
    Serial.println("[QR] Back pressed -> Return to Idle");
    showIdle();
    return;
  }

  // Chạm bất kỳ vị trí nào khác để làm mới mã QR
  Serial.printf("[QR] Cham vung QR (%d, %d) -> Lam moi ma QR\n", x, y);
  refreshQrCode();
}

// Xử lý sự kiện nhấn phím trên màn hình bàn phím số
void DisplayManager::handleKeypadTouch(uint16_t x, uint16_t y) {
  // Kiểm tra nút hủy ở góc trên bên trái
  if (x >= 0 && x <= 88 && y >= 0 && y <= 35) {
    Serial.println("[KEYPAD] Cancel pressed -> Return to Idle");
    showIdle();
    return;
  }

  const int startX = 36;
  const int startY = 74;
  const int keyWidth = 76;
  const int keyHeight = 36;
  const int gapX = 10;
  const int gapY = 6;

  // Xác định tọa độ ô phím người dùng vừa bấm trong lưới 3x4
  for (int row = 0; row < 4; row++) {
    int btnY = startY + row * (keyHeight + gapY);
    if (y >= btnY && y <= btnY + keyHeight) {
      for (int col = 0; col < 3; col++) {
        int btnX = startX + col * (keyWidth + gapX);
        if (x >= btnX && x <= btnX + keyWidth) {
          if (row == 3 && col == 0) {
            // Phím C: Xóa toàn bộ chuỗi ký tự đã nhập
            enteredPin = "";
            drawOtpBoxes();
            Serial.println("[KEYPAD] Cleared all digits");
          } else if (row == 3 && col == 2) {
            // Phím DEL: Xóa lùi một ký tự
            if (enteredPin.length() > 0) {
              enteredPin.remove(enteredPin.length() - 1);
              drawOtpBoxes();
              Serial.printf("[KEYPAD] Deleted 1 digit -> Remaining: %s\n", enteredPin.c_str());
            }
          } else {
            // Phím số từ 0 đến 9
            char pressedDigit = '0';
            if (row < 3) {
              pressedDigit = '1' + (row * 3 + col);
            } else if (row == 3 && col == 1) {
              pressedDigit = '0';
            }

            if (enteredPin.length() < 6) {
              enteredPin += pressedDigit;
              drawOtpBoxes();
              Serial.printf("[KEYPAD] Entered: %c -> PIN: %s\n", pressedDigit, enteredPin.c_str());

              // Khi nhập đủ 6 ký tự mã OTP sẽ tự động gửi đi xác thực
              if (enteredPin.length() == 6) {
                Serial.printf("[KEYPAD] Completed 6-digit PIN: %s -> Verifying\n", enteredPin.c_str());

                // Hiển thị trạng thái đang xác thực trên thanh tiêu đề bàn phím
                tft.fillRect(90, 4, 220, 24, COLOR_BG);
                tft.setTextColor(COLOR_ACCENT, COLOR_BG);
                tft.drawCentreString("VERIFYING PIN...", 200, 8, 2);

                if (otpSubmitCallback) {
                  String submittedPin = enteredPin;
                  otpSubmitCallback(submittedPin);
                }
              }
            }
          }
          return;
        }
      }
    }
  }
}

// Vẽ giao diện đếm ngược 3 giây kèm khung ngắm và thông báo hướng nhìn vào camera
void DisplayManager::drawFaceCountdownScreen() {
  tft.fillScreen(COLOR_BG);

  // Nút hủy thao tác ở góc trên bên trái
  tft.fillRoundRect(8, 4, 76, 24, 4, COLOR_SURFACE);
  tft.drawRoundRect(8, 4, 76, 24, 4, COLOR_BORDER);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_SURFACE);
  tft.drawCentreString("< CANCEL", 46, 8, 2);

  // Tiêu đề màn hình nhận diện khuôn mặt
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
  tft.drawCentreString("FACE RECOGNITION", 195, 8, 2);
  tft.drawFastHLine(0, 31, 320, COLOR_SURFACE);

  // Khung ngắm khuôn mặt với 4 góc điểm nhấn Neon
  const int boxX = 100, boxY = 40, boxW = 120, boxH = 120;
  tft.drawRoundRect(boxX, boxY, boxW, boxH, 8, COLOR_SURFACE_LIGHT);
  const int cl = 16;
  tft.drawFastHLine(boxX, boxY, cl, COLOR_ACCENT);
  tft.drawFastVLine(boxX, boxY, cl, COLOR_ACCENT);
  tft.drawFastHLine(boxX + boxW - cl, boxY, cl, COLOR_ACCENT);
  tft.drawFastVLine(boxX + boxW - 1, boxY, cl, COLOR_ACCENT);
  tft.drawFastHLine(boxX, boxY + boxH - 1, cl, COLOR_ACCENT);
  tft.drawFastVLine(boxX, boxY + boxH - cl, cl, COLOR_ACCENT);
  tft.drawFastHLine(boxX + boxW - cl, boxY + boxH - 1, cl, COLOR_ACCENT);
  tft.drawFastVLine(boxX + boxW - 1, boxY + boxH - cl, cl, COLOR_ACCENT);

  // Hiển thị số đếm ngược ban đầu kích thước lớn
  char countBuf[4];
  snprintf(countBuf, sizeof(countBuf), "%d", faceCountdownSec);
  tft.setTextColor(COLOR_ACCENT, COLOR_BG);
  tft.drawCentreString(countBuf, 160, 76, 7);

  // Thanh hướng dẫn người dùng hướng nhìn về camera đặt bên cạnh tủ
  tft.fillRoundRect(8, 170, 304, 26, 4, COLOR_SURFACE);
  tft.drawRoundRect(8, 170, 304, 26, 4, COLOR_WARNING);
  tft.setTextColor(COLOR_WARNING, COLOR_SURFACE);
  tft.drawCentreString("LOOK AT THE SIDE CAMERA ->", 160, 175, 2);

  // Ghi chú giữ góc nhìn thẳng
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawCentreString("Keep head straight & remove sunglasses", 160, 204, 1);
}

// Cập nhật số đếm ngược từng giây và kích hoạt chụp ảnh khi hết thời gian
void DisplayManager::updateFaceCountdownAnimation() {
  if (currentState != STATE_FACE_COUNTDOWN) return;

  if (millis() - lastCountdownTick >= 1000) {
    lastCountdownTick = millis();

    if (faceCountdownSec > 1) {
      faceCountdownSec--;
      tft.fillRect(125, 70, 70, 60, COLOR_BG);
      char countBuf[4];
      snprintf(countBuf, sizeof(countBuf), "%d", faceCountdownSec);
      tft.setTextColor((faceCountdownSec == 1) ? COLOR_WARNING : COLOR_ACCENT, COLOR_BG);
      tft.drawCentreString(countBuf, 160, 76, 7);
    } else {
      Serial.println("[FACE] Ket thuc dem nguoc -> Chop man hinh va chup anh");
      triggerScreenFlashEffect();

      if (faceTriggerCallback) {
        faceTriggerCallback();
      }

      showFaceProcessing();
    }
  }
}

// Tạo hiệu ứng chớp sáng trắng giả lập màn trập máy ảnh trên màn hình LCD
void DisplayManager::triggerScreenFlashEffect() {
  tft.fillScreen(TFT_WHITE);
  delay(80);
}

// Xử lý sự kiện nhấn nút hủy tại màn hình đếm ngược khuôn mặt
void DisplayManager::handleFaceCountdownTouch(uint16_t x, uint16_t y) {
  if (x <= 95 && y <= 35) {
    Serial.println("[FACE] Nguoi dung huy dem nguoc -> Quay ve man hinh cho");
    showIdle();
  }
}

// Vẽ giao diện thông báo đang gửi ảnh và chờ kết quả phân tích AI
void DisplayManager::drawFaceProcessingScreen() {
  tft.fillScreen(COLOR_BG);

  // Tiêu đề
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
  tft.drawCentreString("FACE VERIFICATION", 160, 8, 2);
  tft.drawFastHLine(0, 31, 320, COLOR_SURFACE);

  // Vòng tròn trung tâm thể hiện ảnh đã chụp
  tft.fillCircle(160, 68, 22, COLOR_SURFACE);
  tft.drawCircle(160, 68, 22, COLOR_SUCCESS);
  tft.fillCircle(160, 68, 7, COLOR_SUCCESS);

  tft.setTextColor(COLOR_SUCCESS, COLOR_BG);
  tft.drawCentreString("PHOTO CAPTURED!", 160, 102, 4);

  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
  tft.drawCentreString("AI Anti-Spoofing & Face Match", 160, 134, 2);

  // Dòng trạng thái chờ kết quả
  tft.fillRect(20, 166, 280, 20, COLOR_BG);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawCentreString("Please wait a moment .", 160, 168, 2);

  tft.drawCentreString("Locker door will unlock upon match", 160, 204, 1);
}

// Cập nhật hoạt ảnh chấm chờ và kiểm tra thời gian quá hạn xác thực
void DisplayManager::updateFaceProcessingAnimation() {
  if (currentState != STATE_FACE_PROCESSING) return;

  if (millis() - lastDotTime >= 500) {
    lastDotTime = millis();
    dotFrame = (dotFrame + 1) % 4;

    tft.fillRect(20, 166, 280, 20, COLOR_BG);
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    const char* frames[] = {
      "Please wait a moment",
      "Please wait a moment .",
      "Please wait a moment ..",
      "Please wait a moment ..."
    };
    tft.drawCentreString(frames[dotFrame], 160, 168, 2);
  }

  // Quá thời gian quy định mà chưa nhận được lệnh mở cửa sẽ báo lỗi
  if (millis() - stateStartTime >= FACE_VERIFY_TIMEOUT_MS) {
    Serial.println("[FACE] Qua thoi gian xac thuc -> Bao loi timeout");
    showError("VERIFICATION TIMEOUT");
  }
}

// Vẽ giao diện thanh toán cước phí mở tủ với mã VietQR trên LCD
void DisplayManager::drawPaymentQrScreen() {
  tft.fillScreen(COLOR_BG);

  // Thanh tiêu đề phía trên
  tft.fillRoundRect(8, 6, 304, 26, 4, COLOR_SURFACE);
  tft.drawRoundRect(8, 6, 304, 26, 4, COLOR_BORDER);

  char titleBuf[48];
  snprintf(titleBuf, sizeof(titleBuf), "PAY FOR BOX #%d", paymentBoxNumber);
  tft.setTextColor(COLOR_WARNING, COLOR_SURFACE);
  tft.drawCentreString(titleBuf, 160, 11, 2);

  // Khởi tạo và vẽ mã VietQR
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(10)];
  bool qrGenerated = false;

  if (paymentQrPayload.length() > 0) {
    for (int v = 6; v <= 10; v++) {
      if (qrcode_initText(&qrcode, qrcodeData, v, ECC_LOW, paymentQrPayload.c_str()) == 0) {
        qrGenerated = true;
        break;
      }
    }
  }

  // Tọa độ và kích thước khung mã QR ở nửa bên trái màn hình
  const int leftCardX = 8;
  const int leftCardY = 38;
  const int leftCardW = 160;
  const int leftCardH = 194;

  tft.fillRoundRect(leftCardX, leftCardY, leftCardW, leftCardH, 6, COLOR_SURFACE);
  tft.drawRoundRect(leftCardX, leftCardY, leftCardW, leftCardH, 6, COLOR_BORDER);

  if (qrGenerated) {
    int scale = (qrcode.size * 3 + 12 <= leftCardW - 8) ? 3 : 2;
    int margin = (scale == 3) ? 5 : 8;
    int totalSize = qrcode.size * scale + margin * 2;
    int qrX = leftCardX + (leftCardW - totalSize) / 2;
    int qrY = leftCardY + 8;

    // Nền trắng cho mã QR để camera điện thoại dễ quét
    tft.fillRoundRect(qrX - 2, qrY - 2, totalSize + 4, totalSize + 4, 4, TFT_WHITE);

    for (uint8_t y = 0; y < qrcode.size; y++) {
      for (uint8_t x = 0; x < qrcode.size; x++) {
        uint16_t color = qrcode_getModule(&qrcode, x, y) ? TFT_BLACK : TFT_WHITE;
        tft.fillRect(qrX + margin + x * scale, qrY + margin + y * scale, scale, scale, color);
      }
    }

    // Dòng gợi ý quét QR dưới mã
    tft.setTextColor(COLOR_TEXT_ACCENT, COLOR_SURFACE);
    tft.drawCentreString("SCAN TO PAY FEE", leftCardX + leftCardW / 2, qrY + totalSize + 8, 1);
  } else {
    // Trường hợp không có payload QR hoặc lỗi chuỗi
    tft.setTextColor(COLOR_DANGER, COLOR_SURFACE);
    tft.drawCentreString("QR ERROR", leftCardX + leftCardW / 2, leftCardY + 70, 2);
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_SURFACE);
    tft.drawCentreString("Please pay via", leftCardX + leftCardW / 2, leftCardY + 95, 1);
    tft.drawCentreString("the mobile app", leftCardX + leftCardW / 2, leftCardY + 110, 1);
  }

  // Khung thông tin cước phí và hướng dẫn ở nửa bên phải màn hình
  const int rightCardX = 174;
  const int rightCardY = 38;
  const int rightCardW = 138;
  const int rightCardH = 194;

  tft.fillRoundRect(rightCardX, rightCardY, rightCardW, rightCardH, 6, COLOR_SURFACE);
  tft.drawRoundRect(rightCardX, rightCardY, rightCardW, rightCardH, 6, COLOR_BORDER);

  // Hiển thị nhãn cước phí
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_SURFACE);
  tft.drawCentreString("FEE DUE", rightCardX + rightCardW / 2, rightCardY + 10, 2);

  // Hiển thị số tiền nổi bật
  char feeBuf[32];
  if (paymentFeeAmount >= 1000) {
    snprintf(feeBuf, sizeof(feeBuf), "%d,%03d", paymentFeeAmount / 1000, paymentFeeAmount % 1000);
  } else {
    snprintf(feeBuf, sizeof(feeBuf), "%d", paymentFeeAmount);
  }
  tft.setTextColor(COLOR_WARNING, COLOR_SURFACE);
  // Số tiền cỡ lớn kèm đơn vị VND cỡ nhỏ trên cùng một dòng, căn giữa theo tổng chiều rộng
  const int amountWidth = tft.textWidth(feeBuf, 4);
  const int unitWidth = tft.textWidth(" VND", 2);
  const int amountX = rightCardX + (rightCardW - amountWidth - unitWidth) / 2;
  tft.drawString(feeBuf, amountX, rightCardY + 30, 4);
  tft.drawString(" VND", amountX + amountWidth, rightCardY + 40, 2);

  // Hiển thị mã đơn hàng SePay
  if (paymentOrderCode > 0) {
    char orderBuf[32];
    snprintf(orderBuf, sizeof(orderBuf), "Order #%ld", paymentOrderCode);
    tft.setTextColor(COLOR_TEXT_ACCENT, COLOR_SURFACE);
    tft.drawCentreString(orderBuf, rightCardX + rightCardW / 2, rightCardY + 62, 1);
  }

  // Hướng dẫn ứng dụng quét
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_SURFACE);
  tft.drawCentreString("Banking app", rightCardX + rightCardW / 2, rightCardY + 84, 1);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_SURFACE);
  tft.drawCentreString("VietQR / MoMo / Zalo", rightCardX + rightCardW / 2, rightCardY + 98, 1);

  // Thông báo cửa tự mở sau khi thanh toán
  tft.setTextColor(COLOR_SUCCESS, COLOR_SURFACE);
  tft.drawCentreString("Door unlocks", rightCardX + rightCardW / 2, rightCardY + 118, 1);
  tft.drawCentreString("after payment", rightCardX + rightCardW / 2, rightCardY + 130, 1);

  // Nút HỦY / THOÁT về trang chủ
  const int btnX = rightCardX + 10;
  const int btnY = rightCardY + 148;
  const int btnW = rightCardW - 20;
  const int btnH = 34;

  tft.fillRoundRect(btnX, btnY, btnW, btnH, 4, COLOR_SURFACE_LIGHT);
  tft.drawRoundRect(btnX, btnY, btnW, btnH, 4, COLOR_DANGER);
  tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_SURFACE_LIGHT);
  tft.drawCentreString("CANCEL", btnX + btnW / 2, btnY + 9, 2);
}

// Xử lý sự kiện chạm tại màn hình thanh toán cước phí
void DisplayManager::handlePaymentQrTouch(uint16_t x, uint16_t y) {
  // Nút HỦY BO ở góc phải dưới
  if (x >= 180 && x <= 312 && y >= 180 && y <= 230) {
    Serial.println("[KIOSK_PAYMENT] Nguoi dung bam Huy thanh toan -> Quay ve Home");
    showIdle();
  }
}

// Cập nhật vòng lặp phát hiện chạm, animation WiFi và đếm thời gian tự động chuyển màn hình
void DisplayManager::update() {
  // Cập nhật animation loading và đếm ngược cho màn hình WiFi
  if (currentState == STATE_WIFI_CONNECTING || currentState == STATE_WIFI_FAILED) {
    updateWifiAnimation();

    // Sau 5 giây thất bại sẽ tự thử kết nối lại bằng cách giữ nguyên state
    if (currentState == STATE_WIFI_FAILED && millis() - stateStartTime >= 5000) {
      Serial.println("[DISPLAY] WiFi retry countdown done -> Redraw connecting screen");
      currentSsid = currentSsid;
      setState(STATE_WIFI_CONNECTING);
    }
    return;
  }

  // Cập nhật tiến trình đếm ngược mã QR khi đang ở màn hình QR
  if (currentState == STATE_QR) {
    updateIdleQrAnimation();
  } else if (currentState == STATE_FACE_COUNTDOWN) {
    updateFaceCountdownAnimation();
  } else if (currentState == STATE_FACE_PROCESSING) {
    updateFaceProcessingAnimation();
  }

  uint16_t x = 0;
  uint16_t y = 0;
  bool pressed = tft.getTouch(&x, &y);

  // Xử lý sự kiện nhấn chạm chống rung và phân phối sự kiện theo từng màn hình
  if (pressed) {
    if (!wasTouched && (millis() - lastTouchTime > 180)) {
      lastTouchTime = millis();
      wasTouched = true;

      if (currentState == STATE_IDLE) {
        handleIdleTouch(x, y);
      } else if (currentState == STATE_QR) {
        handleQrTouch(x, y);
      } else if (currentState == STATE_KEYPAD) {
        handleKeypadTouch(x, y);
      } else if (currentState == STATE_FACE_COUNTDOWN) {
        handleFaceCountdownTouch(x, y);
      } else if (currentState == STATE_PAYMENT_QR) {
        handlePaymentQrTouch(x, y);
      }
    }
  } else {
    wasTouched = false;
  }

  // Tự động chuyển màn hình sau khoảng thời gian chờ quy định
  if (currentState == STATE_SUCCESS) {
    if (millis() - stateStartTime >= SUCCESS_SCREEN_TIMEOUT_MS) {
      Serial.println("[DISPLAY] Success display timeout -> Return to Idle");
      showIdle();
    }
  } else if (currentState == STATE_ERROR) {
    if (millis() - stateStartTime >= ERROR_SCREEN_TIMEOUT_MS) {
      Serial.println("[DISPLAY] Error display timeout -> Return to Idle");
      showIdle();
    }
  } else if (currentState == STATE_KEYPAD) {
    // Nếu người dùng không thao tác gì trên bàn phím sau 30 giây thì tự quay về màn hình chờ
    if (millis() - stateStartTime >= 30000 && enteredPin.length() == 0) {
      Serial.println("[DISPLAY] Keypad inactive for 30s -> Return to Idle");
      showIdle();
    }
  } else if (currentState == STATE_QR) {
    // Nếu ở màn hình QR quá thời gian quy định thì tự quay về màn hình chờ
    if (millis() - stateStartTime >= QR_SCREEN_TIMEOUT_MS) {
      Serial.println("[DISPLAY] QR screen inactive for 90s -> Return to Idle");
      showIdle();
    }
  } else if (currentState == STATE_PAYMENT_QR) {
    // Nếu ở màn hình thanh toán quá thời gian quy định thì tự quay về màn hình chờ
    if (millis() - stateStartTime >= PAYMENT_SCREEN_TIMEOUT_MS) {
      Serial.println("[DISPLAY] Payment QR inactive for 180s -> Return to Idle");
      showIdle();
    }
  }
}
