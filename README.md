# DigitalClock7Segment
Điều khiển led 7 đoạn sử dụng RGB, mạch DS1302 và ESP32

Sử dụng Thư Viên :
// Thu Vien RTC cua Makuna
/https://github.com/Makuna/Rtc
#include <ThreeWire.h>
#include <RtcDS1302.h>

// THu vien FastLED
https://github.com/FastLED/FastLED.git
#include <FastLED.h>

https://github.com/me-no-dev
// Thu vien ESPWebSocketServer cua me-no-dev
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>

// Thu vien Mac dinh arduino
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <FS.h>

Cài đặt đầy đủ thư viện này cho Platform IO

Mạch ESP32 WROOM, RTC DS1302
Chinh lại thông số chân nếu sài board mạch khác.
