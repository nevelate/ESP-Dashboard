#include <Arduino.h>

#include <EncButton.h>

#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <LittleFS.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <AsyncMessagePack.h>

#include <time.h>

#include <Wire.h>
#include <Bme280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Fonts/Org_01.h>

#include "bitmaps.h"
#include "StringHelpers.h"
#include "constants.h"

Bme280TwoWire sensor;
AsyncWebServer server(80);
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);
Button btnUp(10, INPUT_PULLDOWN, HIGH);
Button btnDown(8, INPUT_PULLDOWN, HIGH);
MultiButton btnMulti;

int temperature, pressure, humidity, year;
String currentTime, currentDate;

int8_t mode = 1;  // brightness -> dashboard -> to do list
int8_t selectedTaskIndex, lowerBoundTaskIndex = 2;
int brightness = 255;

JsonDocument doc;
JsonArray tasks;

bool isWiFiConnected = true;

void setup() {
  Wire.begin(6, 7);

  sensor.begin(Bme280TwoWireAddress::Primary);
  sensor.setSettings(Bme280Settings::indoor());

  if (!LittleFS.begin()) return;

  //LittleFS.format();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");

  display.begin(0x3C, false);
  display.display();
  delay(2000);
  display.clearDisplay();

  getDashboardData();

  loadTasks();
}

void loop() {
  btnMulti.tick(btnUp, btnDown);

  if (btnUp.hold()) mode++;
  else if (btnDown.hold()) mode--;

  if (btnMulti.hold()) changeServerState();

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
    if (isWiFiConnected) {
      WiFi.disconnect(true);
      isWiFiConnected = false;
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
  if (btnUp.click()) selectedTaskIndex--;
  else if (btnDown.click()) selectedTaskIndex++;

  if (btnMulti.click()) {
    tasks[selectedTaskIndex]["completed"] = !tasks[selectedTaskIndex]["completed"];
    saveTasks();
  }

  if (selectedTaskIndex < lowerBoundTaskIndex - 3) lowerBoundTaskIndex--;
  selectedTaskIndex = constrain(selectedTaskIndex, 0, tasks.size() - 1);
  if (selectedTaskIndex > lowerBoundTaskIndex) lowerBoundTaskIndex++;

  showToDoList();
}

void showToDoList() {
  bool showTitle = lowerBoundTaskIndex < 3;

  display.clearDisplay();

  display.drawRect(0, 0, 128, 64, 1);

  display.setTextColor(1);
  display.setTextWrap(false);
  display.setFont(&Org_01);

  if (showTitle) {
    display.setTextSize(2);
    display.setCursor(11, 13);
    display.print("To Do List");
  }

  display.setTextSize(1);

  int startIndex = showTitle ? lowerBoundTaskIndex - 2 : lowerBoundTaskIndex - 3;

  for (int i = startIndex; i <= lowerBoundTaskIndex; i++) {
    JsonVariant task = tasks[i];
    int offset = showTitle ? (i + 1 - startIndex) * TO_DO_OFFSET : (i - startIndex) * TO_DO_OFFSET;

    if (i == selectedTaskIndex) {
      display.fillRect(4, 4 + offset, 120, 11, 1);
      display.setTextColor(0);
      display.setCursor(14, 11 + offset);
      display.print(task["text"].as<String>());
      if (task["completed"]) {
        display.drawBitmap(6, 7 + offset, check_bits, 5, 5, 0);
      }
      display.drawLine(12, 6 + offset, 12, 12 + offset, 0);
    } else {
      display.drawRect(4, 4 + offset, 120, 11, 1);
      display.setTextColor(1);
      display.setCursor(14, 11 + offset);
      display.print(task["text"].as<String>());
      if (task["completed"]) {
        display.drawBitmap(6, 7 + offset, check_bits, 5, 5, 1);
      }
      display.drawLine(12, 6 + offset, 12, 12 + offset, 1);
    }
  }

  display.display();
}

int getRIghtAlignedXPos(const String &str, int x, int y, int rightXBound) {
  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(str, x, y, &x1, &y1, &w, &h);

  return rightXBound - w;
}

int getCenterAlignedXPos(const String &str) {
  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);

  return (display.width() - w) / 2;
}

void configureServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/style.css", "text/css");
  });
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/app.js", "text/javascript");
  });

  server.on("/tasks", HTTP_GET, [](AsyncWebServerRequest *request) {
    String content;
    File file = LittleFS.open("/tasks.json", "r");
    if (!file) content = "[]";
    content = file.readString();
    file.close();
    request->send(200, "application/json", content);
  });

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/tasks", [](AsyncWebServerRequest *request, JsonVariant &json) {
    File file = LittleFS.open("/tasks.json", "w");
    if (!file) {
      request->send(500, "text/plain", "Could not open file");
      return;
    }
    serializeJson(json, file);
    file.close();
    request->send(200, "text/plain", "Saved");
  });

  server.addHandler(handler);
}

bool saveTasks() {
  File file = LittleFS.open("/tasks.json", "w");
  if (!file) return false;
  serializeJson(doc, file);
  file.close();
  return true;
}

void loadTasks() {
  String content;
  File file = LittleFS.open("/tasks.json", "r");
  if (!file) content = "[]";
  content = file.readString();
  deserializeJson(doc, content);
  file.close();
  tasks = doc.as<JsonArray>();
}

void changeServerState() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);

    display.clearDisplay();

    display.drawRect(0, 0, 128, 64, 1);

    display.drawBitmap(37, 10, wifi_bits, 52, 31, 1);

    display.display();

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }

    display.setTextColor(1);
    display.setTextSize(2);
    display.setTextWrap(false);
    display.setFont(&Org_01);
    display.setCursor(getCenterAlignedXPos(WiFi.localIP().toString()), 56);
    display.print(WiFi.localIP());

    display.display();

    configureServer();
    server.begin();
    delay(3000);
  } else {
    server.end();
    WiFi.disconnect(true);
    loadTasks();

    display.clearDisplay();

    display.drawRect(0, 0, 128, 64, 1);

    display.drawBitmap(37, 10, wifi_bits, 52, 31, 1);

    display.setTextColor(1);
    display.setTextSize(2);
    display.setTextWrap(false);
    display.setFont(&Org_01);
    display.setCursor(getCenterAlignedXPos("Wifi off"), 56);
    display.print("Wifi off");

    display.display();

    delay(2000);
  }
}