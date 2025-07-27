#include <Arduino.h>
#include <Bme280.h>
#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Fonts/Org_01.h>

static const unsigned char PROGMEM image_humidity_bits[] = { 0x04, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x1e, 0x00, 0x1f, 0x00, 0x3f, 0x80, 0x3f, 0x80, 0x7e, 0xc0, 0x7f, 0x40, 0xff, 0x60, 0xff, 0xe0, 0x7f, 0xc0, 0x7f, 0xc0, 0x3f, 0x80, 0x0f, 0x00 };
static const unsigned char PROGMEM image_pressure_bits[] = { 0x1c, 0x00, 0x22, 0x00, 0x2a, 0x00, 0x2a, 0x00, 0x2a, 0x00, 0x2a, 0x00, 0x2a, 0x00, 0x2a, 0x00, 0x2a, 0x00, 0x49, 0x00, 0x9c, 0x80, 0xae, 0x80, 0xbe, 0x80, 0x9c, 0x80, 0x41, 0x00, 0x3e, 0x00 };
static const unsigned char PROGMEM image_square_bits[] = { 0xc0, 0xc0 };
static const unsigned char PROGMEM image_temperature_bits[] = { 0x1c, 0x00, 0x22, 0x02, 0x2b, 0x05, 0x2a, 0x02, 0x2b, 0x38, 0x2a, 0x60, 0x2b, 0x40, 0x2a, 0x40, 0x2a, 0x60, 0x49, 0x38, 0x9c, 0x80, 0xae, 0x80, 0xbe, 0x80, 0x9c, 0x80, 0x41, 0x00, 0x3e, 0x00 };
static const unsigned char PROGMEM image_paws_bits[] = { 0x1b, 0x00, 0x6c, 0x01, 0xb0, 0x24, 0x80, 0x92, 0x02, 0x48, 0x64, 0xc1, 0x93, 0x06, 0x4c, 0x9f, 0x22, 0x7c, 0x89, 0xf2, 0x91, 0x22, 0x44, 0x89, 0x12, 0xe0, 0xe3, 0x83, 0x8e, 0x0e, 0x40, 0x41, 0x01, 0x04, 0x04, 0x40, 0x41, 0x01, 0x04, 0x04, 0x64, 0xc1, 0x93, 0x06, 0x4c, 0x3b, 0x80, 0xee, 0x03, 0xb8 };

const char* ssid = "com_net";
const char* password = "asdfghjkl";

const long gmtOffset_sec = 5 * 3600;
const int daylightOffset_sec = 0;

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

  display.drawBitmap(5, 40, image_temperature_bits, 16, 16, 1);

  display.setTextSize(1);
  display.setCursor(26, 46);
  display.print(temperature);

  display.setCursor(32, 53);
  display.print("C");

  display.drawBitmap(29, 49, image_square_bits, 2, 2, 1);

  display.setCursor(72, 46);
  display.print(humidity);

  display.setCursor(78, 53);
  display.print("%");

  display.drawBitmap(48, 40, image_humidity_bits, 11, 16, 1);

  display.setCursor(105, 46);
  display.print(pressure);

  display.setCursor(111, 52);
  display.print("mm");

  display.drawBitmap(94, 40, image_pressure_bits, 9, 16, 1);

  display.setTextSize(2);
  display.setCursor(79, 28);
  display.print(year);

  display.drawBitmap(83, 5, image_paws_bits, 39, 10, 1);

  display.display();
}

void setTime() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");
}

String pad2(int number) {
  if (number < 10) return "0" + String(number);
  else return String(number);
}

String month(int month) {
  switch (month) {
    case 0: return "Jan.";
    case 1: return "Feb.";
    case 2: return "Mar.";
    case 3: return "Apr.";
    case 4: return "May.";
    case 5: return "Jun.";
    case 6: return "Jul.";
    case 7: return "Aug.";
    case 8: return "Sep.";
    case 9: return "Oct.";
    case 10: return "Nov.";
    case 11: return "Dec.";
    default: return "NaN.";
  }
}