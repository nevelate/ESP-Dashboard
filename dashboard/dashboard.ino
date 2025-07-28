#define SH110X_NO_SPLASH

#include <Arduino.h>
#include <EncButton.h>
#include <Bme280.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <LittleFS.h>
#include <FS.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Fonts/Org_01.h>

#include "bitmaps.h"
#include "StringHelpers.h"
#include "constants.h"

#define DASHBOARD_DELAY 15000
#define BRIGHTNESS_STEP 16
#define TO_DO_START_X 4
#define TO_DO_START_Y 5
#define TO_DO_OFFSET 15

Bme280TwoWire sensor;
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);
AsyncWebServer server(80);
Button btnUp(10, INPUT_PULLDOWN, HIGH);
Button btnDown(8, INPUT_PULLDOWN, HIGH);

int temperature, pressure, humidity, year;
String currentTime, currentDate;

int8_t mode = 1;  // brightness -> dashboard -> to do list
int brightness = 255;

bool isWiFiDisabled = false;

void setup() {
  Wire.begin(6, 7);

  sensor.begin(Bme280TwoWireAddress::Primary);
  sensor.setSettings(Bme280Settings::indoor());

  if (!LittleFS.begin()) return;

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/main.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/main.css", "text/css");
  });
  server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/main.js", "text/javascript");
  });
  server.begin();

  display.begin(0x3C, false);
  display.display();
  delay(2000);
  display.clearDisplay();

  getDashboardData();
}

void loop() {
  btnUp.tick();
  btnDown.tick();

  if (btnUp.hold()) mode++;
  else if (btnDown.hold()) mode--;

  if (mode < 0) mode = 2;
  else if (mode > 2) mode = 0;

  switch (mode) {
    case 0:
      brightnessControl();
      break;
    case 1:
      dashboard();
      break;
    case 2:
      toDoList();
      break;
  }
}

void dashboard() {
  static uint32_t tmr;

  if (millis() - tmr >= DASHBOARD_DELAY) {
    tmr = millis();

    getDashboardData();
  }

  showDashboard();
}

void showDashboard() {

  display.clearDisplay();

  display.drawRect(0, 0, 128, 64, 1);
  display.drawLine(0, 0, 0, 0, 1);
  display.drawLine(0, 32, 126, 32, 1);
  display.drawLine(43, 33, 43, 62, 1);
  display.drawLine(89, 33, 89, 62, 1);

  display.setTextColor(1);
  display.setTextSize(2);
  display.setTextWrap(false);
  display.setFont(&Org_01);

  display.setCursor(3, 28);
  display.print(currentDate);

  display.setTextSize(3);
  display.setCursor(3, 15);
  display.print(currentTime);

  display.drawBitmap(5, 40, temperature_bits, 16, 16, 1);

  display.setTextSize(1);
  display.setCursor(getRIghtAlignedXPos(String(temperature), 26, 46, 37), 46);
  display.print(temperature);

  display.setCursor(32, 53);
  display.print("C");

  display.drawBitmap(29, 49, square_bits, 2, 2, 1);

  display.setCursor(getRIghtAlignedXPos(String(humidity), 72, 46, 83), 46);
  display.print(humidity);

  display.setCursor(78, 53);
  display.print("%");

  display.drawBitmap(48, 40, humidity_bits, 11, 16, 1);

  display.setCursor(getRIghtAlignedXPos(String(pressure), 105, 46, 122), 46);
  display.print(pressure);

  display.setCursor(111, 52);
  display.print("mm");

  display.drawBitmap(94, 40, pressure_bits, 9, 16, 1);

  display.setTextSize(2);
  display.setCursor(getRIghtAlignedXPos(String(year), 79, 28, 125), 28);
  display.print(year);

  display.drawBitmap(83, 5, paws_bits, 39, 10, 1);

  display.display();
}

void brightnessControl() {
  if (btnUp.click()) brightness += BRIGHTNESS_STEP;
  else if (btnDown.click()) brightness -= BRIGHTNESS_STEP;

  brightness = constrain(brightness, 0, 255);

  display.setContrast(brightness);

  showBrightnessControl();
}

void getDashboardData() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    if (!isWiFiDisabled) {
      //WiFi.disconnect(true);
      isWiFiDisabled = true;
    }

    currentTime = pad2(timeinfo.tm_hour) + ":" + pad2(timeinfo.tm_min);
    currentDate = pad2(timeinfo.tm_mday) + " " + month(timeinfo.tm_mon);
    year = timeinfo.tm_year + 1900;
  }

  temperature = int(sensor.getTemperature());
  pressure = int(sensor.getPressure() / 133.3f);
  humidity = int(sensor.getHumidity());
}

void showBrightnessControl() {

  display.clearDisplay();

  display.drawRect(0, 0, 128, 64, 1);

  display.fillRect(32, 48, (brightness + 1) / 4, 8, 1);

  display.drawRect(30, 46, 68, 12, 1);

  display.drawCircle(64, 23, 10, 1);

  display.drawBitmap(48, 7, brightness_bits, 33, 33, 1);

  display.fillCircle(64, 23, 8, 1);

  display.display();
}

void toDoList() {
  showToDoList();
}

void showToDoList() {
  display.clearDisplay();

  display.drawRect(0, 0, 128, 64, 1);

  display.setTextColor(1);
  display.setTextSize(2);
  display.setTextWrap(false);
  display.setFont(&Org_01);
  display.setCursor(11, 13);
  display.print("To Do List");

  display.setTextSize(1);
  display.setCursor(14, 26);
  display.print("Hiragana");

  display.setCursor(14, 41);
  display.print("LeetCode");

  display.drawBitmap(6, 22, check_bits, 5, 5, 1);

  display.drawRect(4, 19, 120, 11, 1);

  display.drawRect(4, 34, 120, 11, 1);

  display.drawLine(12, 21, 12, 27, 1);

  display.drawLine(12, 36, 12, 42, 1);

  display.fillRect(4, 49, 120, 11, 1);

  display.setTextColor(0);
  display.setCursor(14, 56);
  display.print("Zig");

  display.drawLine(12, 51, 12, 57, 0);

  display.drawBitmap(6, 52, check_bits, 5, 5, 0);

  display.display();
}

int getRIghtAlignedXPos(const String &str, int x, int y, int rightXBound) {
  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(str, x, y, &x1, &y1, &w, &h);

  return rightXBound - w;
}