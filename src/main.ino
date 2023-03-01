#include <Arduino.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <FastLED.h>
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <FS.h>

const int LED_PIN = 4;
const int LED_PIN2 = 17;
const int NUM_LEDS = 612;       // Khai bao tong so led co tren 1 mat
const int NUM_DIGIT = 6;        // Khai bao tong so cac so tren 1 mat dong ho
const int NUM_LED_SEGMENT = 14; // Khai bao so luong led co tren 1 segment
const int NUM_LED_DOT = 12;     // Khai bao so luon gled co tren 1 dot , phan chia gio phut giay
CRGB matLed1[NUM_LEDS];
CRGB matLed2[NUM_LEDS];

// Tạo mảng các điểm ảnh để biểu diễn đoạn số led 7 doan
// 0bgfedcba
byte digits[11] = {
    0b0111111, // So 0
    0b0000110, // So 1
    0b1011011, // So 2
    0b1001111, // So 3
    0b1100110, // So 4
    0b1101101, // So 5
    0b1111101, // So 6
    0b0000111, // So 7
    0b1111111, // So 8
    0b1101111, // So 9
    0b0000000  // tat led
};
// CONNECTIONS:
// DS1302 CLK/SCLK --> 25
// DS1302 DAT/IO --> 26
// DS1302 RST/CE --> 27
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND
// ThreeWire myWire(26, 27, 25);  // IO, SCLK, CE
ThreeWire myWire(26, 25, 27); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

const char *ssid = "MyESP32AP";
const char *password = "123456789";
boolean isDebug = true;
boolean isTimer;
boolean modeRaceUp;
boolean loopMode;
boolean isNotStop;
String state = "countToStart";
boolean loopLed1;
String startTime = "";
String endTime = "";

RtcDateTime timeStart;
RtcDateTime timeEnd;
RtcDateTime timeNow;
RtcDateTime timePause;

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
// Hàm ghi dữ liệu vào SPIFFS
void saveSettingWifi()
{
  // Tạo file để ghi dữ liệu
  File file = SPIFFS.open("/settingWifi.txt", FILE_WRITE);
  if (file)
  {
    // Ghi dữ liệu vào file
    DynamicJsonDocument json(1024);
    json["ssid"] = ssid;
    json["password"] = password;
    serializeJson(json, file);
  }
  file.close();
}
// Hàm đọc dữ liệu từ SPIFFS và gán cho các biến toàn cục
void loadSettingWifi()
{
  // Mở file để đọc dữ liệu
  File file = SPIFFS.open("/settingWifi.txt");
  if (!file)
  {
    Serial.println("Failed to create file setingWifi");
    return;
  }

  // Đọc dữ liệu từ file và gán cho các biến toàn cục
  DynamicJsonDocument json(1024);
  DeserializationError error = deserializeJson(json, file);
  if (error)
  {
    Serial.println("Khong tim dc thong tin WIFI. Khoi dong wifi mac dinh");
    WiFi.softAP(ssid, password);
  }
  else
  {
    ssid = json["ssid"];
    password = json["password"];
    Serial.println(ssid);
    Serial.println(password);
    WiFi.softAP(ssid, password);
    Serial.printf("Access point started: %s\n", ssid);
  }
  file.close();
}
// Hàm ghi dữ liệu vào SPIFFS
void saveSettingsToSPIFFS()
{
  // Tạo file để ghi dữ liệu
  File file = SPIFFS.open("/settings.txt", FILE_WRITE);
  if (file)
  {
    // Ghi dữ liệu vào file
    DynamicJsonDocument json(1024);
    json["isTimer"] = isTimer;
    json["startTime"] = startTime;
    json["endTime"] = endTime;
    json["modeRaceUp"] = modeRaceUp;
    json["loopMode"] = loopMode;
    json["loopLed1"] = loopLed1;
    json["isNotStop"] = isNotStop;
    serializeJson(json, file);
  }
  file.close();
}

// Hàm đọc dữ liệu từ SPIFFS và gán cho các biến toàn cục
void loadSettingsFromSPIFFS()
{
  // Mở file để đọc dữ liệu
  File file = SPIFFS.open("/settings.txt");
  if (!file)
  {
    Serial.println("Failed to open file");
    return;
  }

  // Đọc dữ liệu từ file và gán cho các biến toàn cục
  DynamicJsonDocument json(1024);
  DeserializationError error = deserializeJson(json, file);
  if (!error)
  {
    isTimer = json["isTimer"];
    startTime = json["startTime"].as<String>();
    endTime = json["endTime"].as<String>();
    modeRaceUp = json["modeRaceUp"];
    loopMode = json["loopMode"];
    loopLed1 = json["loopLed1"];
    isNotStop = json["isNotStop"];
    // Chuyen String ve Char
    char charArrStartTime[startTime.length() + 1];
    startTime.toCharArray(charArrStartTime, startTime.length() + 1);

    char charArrEndTime[endTime.length() + 1];
    endTime.toCharArray(charArrEndTime, endTime.length() + 1);
    showMessage(startTime);
    timeStart = convertTimeStringToRTC(charArrStartTime);
    showMessage((String)timeStart.TotalSeconds());
    showMessage(endTime);
    timeEnd = convertTimeStringToRTC(charArrEndTime);
    showMessage((String)timeEnd.TotalSeconds());

    // convectEndTimeStringToRTC(charArrEndTime);
    // timeStart = RtcDateTime(2023, 3, 1, 18, 05, 00);
    // timeEnd = RtcDateTime(2023, 3, 1, 19, 05, 00);
  }
  file.close();
}
// chuyen doi chuoi dinh dang "03/21/1991 12:40:30" sang RtcDateTime
RtcDateTime convertTimeStringToRTC(char *dateTimeStr)
{

  char *dateStr = strtok(dateTimeStr, " ");
  char *timeStr = strtok(NULL, " ");

  // Tách ngày tháng năm từ chuỗi ngày
  int month = atoi(strtok(dateStr, "/"));
  int day = atoi(strtok(NULL, "/"));
  int year = atoi(strtok(NULL, "/"));

  // Tách giờ phút giây từ chuỗi giờ
  int hour = atoi(strtok(timeStr, ":"));
  int minute = atoi(strtok(NULL, ":"));
  int second = atoi(strtok(NULL, ":"));
  showMessage((String)day);
  showMessage("/");
  showMessage((String)month);
  showMessage("/");
  showMessage((String)year);
  showMessage("/");
  showMessage((String)hour);
  showMessage("/");
  showMessage((String)minute);
  showMessage("/");
  showMessage((String)second);
  showMessage("/");
  return RtcDateTime(year, month, day, hour, minute, second);
}

void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_DISCONNECTED)
    Serial.printf("[WSc] Disconnected!\n");
  if (type == WStype_CONNECTED)
    Serial.printf("[WSc] Connected to url: %s\n", payload);
  if (type == WStype_BIN)
    Serial.printf("[WSc] get binary length: %u\n", length);
  if (type == WStype_TEXT)
  {
    // Parse the JSON data sent by the client
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error)
    {
      Serial.println("deserializeJson() failed");
      return;
    }
    //{"isTimer":false,"startTime":"2023-02-27T20:26","endTime":"","modeRaceUp":"true","loopMode":"false","loopLed1":"true","isNotStop":false}
    // Extract data from the JSON data
    if (doc["action"] == "setDataClock")
    {
      state = "countToStart";
      isTimer = doc["isTimer"];
      startTime = doc["startTime"].as<String>();
      endTime = doc["endTime"].as<String>();
      modeRaceUp = doc["modeRaceUp"];
      loopMode = doc["loopMode"];
      loopLed1 = doc["loopLed1"];
      isNotStop = doc["isNotStop"];
      webSocket.sendTXT(num, "I Got Your Messager");
      saveSettingsToSPIFFS();
      loadSettingsFromSPIFFS();
    }
    else if (doc["action"] == "setClock")
    {
      // chuyen vê clock mode khong xoa du lieu
      webSocket.sendTXT(num, "Chuyen ve Mode CLock thanh cong! Giai dau khong bi gian doan!");
    }
    else if (doc["action"] == "clearClock")
    {
      // bat tín hiệu xoá toàn bộ seting chuyên về clock mode
      webSocket.sendTXT(num, "Xoa du lieu Giai Dau thanh cong! Chuyen ve Mode Clock");
    }
    else if (doc["action"] == "finishJob")
    {
      // bắt tín hiệu báo dừng giải đấu. Lưu toàn bộ dữ liệu hiện tại và stop hiển thị
      webSocket.sendTXT(num, "Giai dau ket thuc!");
    }
    else if (doc["action"] == "stop")
    {
      // bắt tín hiệu báo dừng giải đấu. Lưu toàn bộ dữ liệu hiện tại và stop hiển thị
      webSocket.sendTXT(num, "Giai dau ket thuc!");
    }
    else if (doc["action"] == "configWifi")
    {
      // bắt tín hiệu báo dừng giải đấu. Lưu toàn bộ dữ liệu hiện tại và stop hiển thị
      ssid = doc["ssid"];
      password = doc["password"];
      Serial.println(ssid);
      Serial.println(password);
      saveSettingWifi();
      webSocket.sendTXT(num, "Luu thong tin thanh cong! Server dang khoi dong lai");
    }
  }
}
// khoi tao dong ho
void initRtc()
{
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!Rtc.IsDateTimeValid())
  {
    showMessage("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected())
  {
    showMessage("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning())
  {
    showMessage("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }
}
// khoi tao FastLed
void initFastLed()
{
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(matLed1, NUM_LEDS);  // Khoi tao control cho mat 1
  FastLED.addLeds<WS2812B, LED_PIN2, GRB>(matLed2, NUM_LEDS); // khoi tao control cho mat 2
  FastLED.clear();
}
void setup()
{
  Serial.begin(9600);
  initFastLed();
  initRtc();
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }
  loadSettingWifi();
  loadSettingsFromSPIFFS();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              Serial.print("Get ON");
              request->send(SPIFFS, "/index.html", "text/html"); });
  server.on("/setting", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              Serial.print("Get ON");
              request->send(SPIFFS, "/setting.html", "text/html"); });

  server.begin();
  webSocket.begin();
  webSocket.onEvent(handleWebSocketEvent);
  initConnectWifiTest();
}
void initConnectWifiTest()
{

  // Connect to the new Wi-Fi network
  WiFi.begin("Quang Cao Nguyen Ho.Com", "908165185");
  Serial.printf("Connecting to %s...\n", ssid);

  // Wait for the connection to complete
  int timeout = 10;
  while (WiFi.status() != WL_CONNECTED && timeout-- > 0)
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println();

  // Check if the connection was successful
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connection failed");
    return;
  }

  // Print the new network details
  Serial.println("Connected to new network");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  webSocket.loop();
  timeNow = Rtc.GetDateTime();
  showMessage("Start");
  showMessage((String)timeStart.Hour());
  showMessage((String)timeStart.Minute());
  showMessage((String)timeStart.Second());
  showMessage((String)timeStart.Day());
  showMessage((String)timeStart.Month());
  showMessage((String)timeStart.Year());
  showMessage("EndStart");
  showMessage("End");
  showMessage((String)timeEnd.Hour());
  showMessage((String)timeEnd.Minute());
  showMessage((String)timeEnd.Second());
  showMessage((String)timeEnd.Day());
  showMessage((String)timeEnd.Month());
  showMessage((String)timeEnd.Year());
  showMessage("EndEnd");
  showMessage("Now");
  showMessage((String)timeNow.Hour());
  showMessage((String)timeNow.Minute());
  showMessage((String)timeNow.Second());
  showMessage((String)timeNow.Day());
  showMessage((String)timeNow.Month());
  showMessage((String)timeNow.Year());
  showMessage("ENDNOW");
  xuLyLogicDongHo();
}
// xu Ly Logic Dong ho
void xuLyLogicDongHo()
{
  if (state == "sleep")
  {
    return;
  }
  // neu state la clock chuyen dong ho hien thi thoi gian thuc
  if (state == "clock")
  {
    showClock(timeNow, 1);
    showClock(timeNow, 2);
    // Khong tiep tuc kiem tra cac dieu kien ben duoi den khi nao state duoc thay doi ( Nhan tu APP)
    return;
  }
  // chi cho phep resume khi
  // ModeRaceUp, no Loop, NotStop
  if (state == "resume" && isNotStop && modeRaceUp && !loopMode)
  {
    timeStart = RtcDateTime(calculateTimeDiff(timePause, timeNow));
    state = "start";
  }
  // Chi cho phep tam dung dem khi
  // ModeRaceUp, no Loop, NotStop
  if (state == "pause" && isNotStop && modeRaceUp && !loopMode)
  {
    timePause = timeNow;
    state = "sleep";
    return;
  }
  // Che do dem len khong dung khi nhan bat dau State == start va isNotStop == true va modeRaceUp true va not loopMode
  if (state == "start" && isNotStop && modeRaceUp && !loopMode)
  {
    showMessage("start && isNotStop && modeRaceUp && !loopMode");
    uint32_t timeDiff = calculateTimeDiff(timeNow, timeStart);
    uint8_t currentSecond = timeNow.Second();
    showTimeToClock(timeDiff, 1);
    showTimeToClock(timeDiff, 2);
    return;
  }
  if (state == "finish")
  {
    // ket thuc chuong trinh.
    // HIen thi noi dung hien thi cuoi cung
    // luu thoi gian hien thi cuoi cung khi vo state nay
    return;
  }
  // Kiem tra xem dong ho dang o trang thai gi.
  // Neu co hen gio va state dang o trang thai dem toi hen gio thi hien thi thoi gian dem nguoc toi luc bat dau
  if (isTimer && state == "countToStart")
  {
    if (timeNow < timeStart)
    {
      showMessage("isTimer && state == countToStart && timeNow < timeStart");
      uint32_t timeDiff = calculateTimeDiff(timeStart, timeNow);
      uint8_t currentSecond = timeNow.Second();
      showMessage("Dem Toi Start : ");
      Serial.println(timeDiff);
      showTimeToClock(timeDiff, 1);
      showTimeToClock(timeDiff, 2);
      // Retun ngay khuc nay de khong kiem tra nhung buoc duoi tranh lam giam hieu nang
      return;
    }
    else
    {
      showMessage("Xong dem < timeStart");
      // neu het thoi gian dem nguoc, chuyen che do qua start
      state = "start";
      return;
    }
  }
  else if (!isTimer && state == "countToStart")
  {
    // Neu khong o mode hen gio va sate van dang la countToStart thi chuyen state ve start
    state = "start";
  }
  // ##### bat dau vao che do xu ly nang cao ########//
  switch (loopMode)
  {
  case true: // loopMode True
             // Khi loopmode kich hoat, chi quan tam toi cac bien timebat dau , timeket thuoc. ko kich hoat dc tam dung, ko kich hoat duoc tiep tuc
    switch (modeRaceUp)
    {
    case true: // 1 la Up
      showMessage("Loop Mode Race Up");
      showClockWhenLoopModeActiveAndUp();
      break;
    case false: // 0 la down
      showMessage("Loop Mode Race Down");
      showClockWhenLoopModeActiveAndDown();
      break;
    }
    break;
  case false: // loopMode False
    switch (modeRaceUp)
    {
    // true la Up false la Down
    case true:
      showClockWhenLoopModeNotActiveAndUp();
      break;
    // END CASE
    case false: // false la down
      showClockWhenLoopModeNotActiveAndDown();
      break;
      // END CASE
    }
    break;
  }
  // ##### ket thuc che do xu ly nang cao #####//
}
// Hien thi Debug ra Serial , se ko hien thi neu la product //
void showMessage(String message)
{
  if (isDebug)
    Serial.println(message);
}
// Hien thi thoi gian tren dong khi chi che do loop khong duoc kich hoat va mode up
void showClockWhenLoopModeNotActiveAndUp()
{
  showMessage("Not Lop Mode Race Up");
  // Che do dem Tu timeStart toi timeEnd
  if (state == "start" && timeNow >= timeStart && timeNow <= timeEnd)
  {
    uint32_t timeDiff = calculateTimeDiff(timeNow, timeStart);
    uint8_t currentSecond = timeNow.Second();
    showMessage("Dem len toi End: ");
    showTimeToClock(timeDiff, 1);
    showTimeToClock(timeDiff, 2);
  }
  else if (state == "start" && timeNow < timeStart)
  { // Khong o che do timer dem len hien thi 000000
    showTimeToClock(0, 1);
    showTimeToClock(0, 2);
  }
  else
  {
    state = "continue";
  }
  // Che do hien thi khi che do timeNow > timeEnd
  if (state == "continue" && timeNow >= timeEnd && !isNotStop)
  {
    // La mode Down va khong Loop nen hien thi so 00:00:00
    uint32_t timeDiff = calculateTimeDiff(timeEnd, timeStart);
    showMessage("Dung Dem: ");
    showTimeToClock(timeDiff, 1);
    showTimeToClock(timeDiff, 2);
  }
  else if (state == "continue" && timeNow >= timeEnd && isNotStop)
  {
    state = "start";
  }
}
// Hien thi thoi gian tren dong ho khi che do loop khong duocj kich hoat va mode down
void showClockWhenLoopModeNotActiveAndDown()
{
  showMessage("Not Lop Mode Race Down");
  showMessage((String)timeStart.TotalSeconds());
  showMessage((String)timeEnd.TotalSeconds());
  showMessage((String)timeNow.TotalSeconds());
  if (state == "start" && timeNow >= timeStart && timeNow <= timeEnd)
  {
    uint32_t timeDiff = calculateTimeDiff(timeEnd, timeNow);
    uint8_t currentSecond = timeNow.Second();
    showMessage("Dem toi End: ");
    showTimeToClock(timeDiff, 1);
    showTimeToClock(timeDiff, 2);
  }
  else if (state == "start" && timeNow < timeStart)
  {
    // chua den thoi gian bat dau hien thi 000000
    showTimeToClock(0, 1);
    showTimeToClock(0, 2);
  }
  else
  {
    state = "continue";
  }
  if (state == "continue" && timeNow >= timeEnd)
  {
    // La mode Down va khong Loop nen hien thi so 00:00:00
    showTimeToClock(0, 1);
    showTimeToClock(0, 2);
  }
}
// Hien thi thoi gian tren dong ho khi che do loop dc kich hoat va Mode UP
void showClockWhenLoopModeActiveAndUp()
{
    if (timeNow < timeStart)
  {
    if (loopLed1)
    {
      showTimeToClock(0, 2);
      ShowLoopCount(0, 1);
    }
    else
    {
      showTimeToClock(0, 1);
      ShowLoopCount(0, 2);
    }
    return;
  }
  uint32_t timeDiffAB = calculateTimeDiff(timeEnd, timeStart);
  uint32_t timeDiffNow = calculateTimeDiff(timeNow, timeStart);
  int loopCount = timeDiffNow / timeDiffAB;
  if (loopCount > 99)
  {
    loopCount = 99;
    state = "finish";
    if (loopLed1)
    {
      showTimeToClock(timeDiffAB, 2);
      ShowLoopCount(loopCount, 1);
    }
    else
    {
      showTimeToClock(timeDiffAB, 1);
      ShowLoopCount(loopCount, 2);
    }
    return;
  }
  uint32_t timeNewStartSeconds = timeStart.TotalSeconds() + timeDiffAB * loopCount;
  uint32_t timeNewEndSeconds = timeEnd.TotalSeconds() + timeDiffAB * (loopCount + 1);

  uint32_t timeDiff1 = labs(timeNow.TotalSeconds() - timeNewStartSeconds);
  if (loopLed1)
  {
    showTimeToClock(timeDiff1, 2);
    ShowLoopCount(loopCount, 1);
  }
  else
  {
    showTimeToClock(timeDiff1, 1);
    ShowLoopCount(loopCount, 2);
  }
}
// Hien thi thoi gian tren dong ho khi che do loop dc kich hoat va Mode Down
void showClockWhenLoopModeActiveAndDown()
{
  if (timeNow < timeStart)
  {
    if (loopLed1)
    {
      showTimeToClock(0, 2);
      ShowLoopCount(0, 1);
    }
    else
    {
      showTimeToClock(0, 1);
      ShowLoopCount(0, 2);
    }
    return;
  }
  uint32_t timeDiffAB = calculateTimeDiff(timeEnd, timeStart);
  uint32_t timeDiffNow = calculateTimeDiff(timeNow, timeStart);
  int loopCount = timeDiffNow / timeDiffAB;
  if (loopCount > 99)
  {
    loopCount = 99;
    state = "finish";
    if (loopLed1)
    {
      showTimeToClock(0, 2);
      ShowLoopCount(loopCount, 1);
    }
    else
    {
      showTimeToClock(0, 1);
      ShowLoopCount(loopCount, 2);
    }
    return;
  }
  uint32_t timeNewStartSeconds = timeStart.TotalSeconds() + timeDiffAB * loopCount;
  uint32_t timeNewEndSeconds = timeEnd.TotalSeconds() + timeDiffAB * (loopCount);

  uint32_t timeDiff1 = labs(timeNewEndSeconds - timeNow.TotalSeconds());
  if (loopLed1)
  {
    showTimeToClock(timeDiff1, 2);
    ShowLoopCount(loopCount, 1);
  }
  else
  {
    showTimeToClock(timeDiff1, 1);
    ShowLoopCount(loopCount, 2);
  }
}
// Tinh toan chenh lech giua 2 khoang thoi gian. Tra ve don vi giay
uint32_t calculateTimeDiff(RtcDateTime &time1, RtcDateTime &time2)
{
  uint32_t secondsDiff = labs(time1.TotalSeconds() - time2.TotalSeconds());
  return secondsDiff;
}
// Chuyen dinh dang giay thanh HH:MM:SS ( Dung de Printr ra SerialMonitor)
String formatTime(unsigned long seconds)
{
  unsigned int hours = seconds / 3600;
  unsigned int minutes = (seconds / 60) % 60;
  unsigned int secs = seconds % 60;
  char buffer[9];
  sprintf(buffer, "%02u:%02u:%02u", hours, minutes, secs);
  return String(buffer);
}
// Hien thi thoi gian chenh lech giua 2 khoang thoi gian len dong ho
// Dinh dang gui vo seconds, hien thi HH:MM:SS
// KHong su dung ham nay de hien thi thoi gian Thuc.
void ShowLoopCount(int loopCount, int matLed)
{
  ShowDigit(10, 1, 0, matLed);
  ShowDigit(10, 2, 0, matLed);
  ShowDot(1, 0, matLed);
  ShowDigit(loopCount / 10, 3, NUM_LED_DOT, matLed);
  ShowDigit(loopCount % 10, 4, NUM_LED_DOT, matLed);
  ShowDot(2, 0, matLed);
  ShowDigit(10, 5, NUM_LED_DOT * 2, matLed);
  ShowDigit(10, 6, NUM_LED_DOT * 2, matLed);
}
// Hien thi thoi gian chenh lech giua 2 khoang thoi gian len dong ho
// Dinh dang gui vo seconds, hien thi HH:MM:SS
// KHong su dung ham nay de hien thi thoi gian Thuc.
void showTimeToClock(unsigned long seconds, int matLed)
{
  showMessage("showTimeToClock call");
  showMessage(state);
  showMessage((String)seconds);
  unsigned int hours = seconds / 3600;
  unsigned int minutes = (seconds / 60) % 60;
  unsigned int secs = seconds % 60;
  ShowDigit(hours / 10, 1, 0, matLed);
  ShowDigit(hours % 10, 2, 0, matLed);
  ShowDot(1, 1, matLed);
  ShowDigit(minutes / 10, 3, NUM_LED_DOT, matLed);
  ShowDigit(minutes % 10, 4, NUM_LED_DOT, matLed);
  ShowDot(2, 1, matLed);
  ShowDigit(secs / 10, 5, NUM_LED_DOT * 2, matLed);
  ShowDigit(secs % 10, 6, NUM_LED_DOT * 2, matLed);
}
// hien thi thoi gian thuc len dong ho
// dinh dang hien thi HH:MM:SS
void showClock(const RtcDateTime &dt, int matLed)
{
  showMessage("showClock call");
  ShowDigit(dt.Hour() / 10, 1, 0, matLed);
  ShowDigit(dt.Hour() % 10, 2, 0, matLed);
  ShowDot(1, 1, matLed);
  ShowDigit(dt.Minute() / 10, 3, NUM_LED_DOT, matLed);
  ShowDigit(dt.Minute() % 10, 4, NUM_LED_DOT, matLed);
  ShowDot(2, 1, matLed);
  ShowDigit(dt.Second() / 10, 5, NUM_LED_DOT * 2, matLed);
  ShowDigit(dt.Second() % 10, 6, NUM_LED_DOT * 2, matLed);
}
// Ham dieu khien FASTLED
// Nhan vao du lieu so 0-9
// digit vi tri hien thi cua so tren dong ho HH:MM:SS tuong ung vi tri 12:34:56
// plusLed cong them bao nhieu led neu can
// matLed de truyen tham so vao control led thich hop - 1 hoac 2
void ShowDigit(int number, int digit, int plusLed, int matLed)
{

  if (number < 0 || number > 10)
  {
    showMessage("Invalid number");
    return;
  }

  int index = (digit - 1) * NUM_LED_SEGMENT * 7 + plusLed;

  byte segment = digits[number]; // lay du lieu dinh dang cua so duoc khai bao trong digits

  for (int j = 0; j < 7; j++)
  {
    if (bitRead(segment, j))
    {
      for (int i = 0; i < NUM_LED_SEGMENT; i++)
      {
        if (matLed == 1)
          matLed1[index + i] = CRGB::Red;
        else
          matLed2[index + i] = CRGB::Red;
      }
      index = index + NUM_LED_SEGMENT;
    }
    else
    {
      for (int i = 0; i < NUM_LED_SEGMENT; i++)
      {
        if (matLed == 1)
          matLed1[index + i] = CRGB::Black;
        else
          matLed2[index + i] = CRGB::Black;
      }
      index = index + NUM_LED_SEGMENT;
    }
  }
  FastLED.show();
}
// hien thi dau cham
// du lieu truyen vao vi tri Dot 1 hay 2, tat hay mo, mat led control nao
void ShowDot(int numDot, int Light, int matLed)
{
  int index;

  if (numDot == 1)
    index = NUM_LED_SEGMENT * 7 * 2 * numDot;
  else
    index = NUM_LED_SEGMENT * 7 * 2 * numDot + NUM_LED_DOT;
  if (Light == 1)
  {
    for (int i = 0; i < NUM_LED_DOT; i++)
    {
      if (matLed == 1)
        matLed1[index + i] = CRGB::Red;
      else
        matLed2[index + i] = CRGB::Red;
    }
  }
  else
  {
    for (int i = 0; i < NUM_LED_DOT; i++)
    {
      if (matLed == 1)
        matLed1[index + i] = CRGB::Black;
      else
        matLed2[index + i] = CRGB::Black;
    }
  }
  FastLED.show();
}
