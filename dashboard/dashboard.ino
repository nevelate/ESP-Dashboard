#define SH110X_NO_SPLASH

#include <Arduino.h>
#include <Bme280.h>
#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Fonts/Org_01.h>

#include "bitmaps.h"
#include "StringHelpers.h"
#include "constants.h"

Bme280TwoWire sensor;
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);

int temperature, pressure, humidity, year;
String currentTime, currentDate;

bool isWiFiDisabled = false;

void setup() {
  Wire.begin(6, 7);

  sensor.begin(Bme280TwoWireAddress::Primary);
  sensor.setSettings(Bme280Settings::indoor());

  display.begin(0x3C, false);

  setTime();

  display.display();
  delay(2000);

  display.clearDisplay();
}

void loop() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    if (!isWiFiDisabled) {
      WiFi.disconnect(true);
      isWiFiDisabled = true;
    }

    currentTime = pad2(timeinfo.tm_hour) + ":" + pad2(timeinfo.tm_min);
    currentDate = pad2(timeinfo.tm_mday) + " " + month(timeinfo.tm_mon);
    year = timeinfo.tm_year + 1900;
  }

  temperature = int(sensor.getTemperature());
  pressure = int(sensor.getPressure() / 133.3f);
  humidity = int(sensor.getHumidity());

  show();

  delay(5000);
}

void show() {
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

void setTime() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");
}

int getRIghtAlignedXPos(const String &str, int x, int y, int rightXBound) {
  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(str, x, y, &x1, &y1, &w, &h);

  return rightXBound - w;
}