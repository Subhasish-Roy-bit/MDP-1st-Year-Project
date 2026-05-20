#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <DHT.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DHTPIN 14
#define DHTTYPE DHT11

#define WIFI_SSID " "
#define WIFI_PASSWORD " "

// Supabase
const char* supabaseUrl = " ";
const char* supabaseKey = " ";

Adafruit_SSD1306 display(128,64,&Wire,-1);
Adafruit_INA219 ina219;
DHT dht(DHTPIN, DHTTYPE);

void sendToSupabase(float current, float voltage, float temp, float hum)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    http.begin(supabaseUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", supabaseKey);
    http.addHeader("Authorization", "Bearer " + String(supabaseKey));
    http.addHeader("Prefer", "return=minimal");

    String json = "{";
    json += "\"current\":" + String(current,2) + ",";
    json += "\"voltage\":" + String(voltage,2) + ",";
    json += "\"temp\":" + String(temp,2) + ",";
    json += "\"humidity\":" + String(hum,2);
    json += "}";

    int httpResponseCode = http.POST(json);

    Serial.print("HTTP Response: ");
    Serial.println(httpResponseCode);

    http.end();
  }
}

void setup()
{
  Serial.begin(115200);

  Wire.begin(26,27);

  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay();

  ina219.begin();
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Still connecting...");
  }

  Serial.println("WiFi connected!");
}

void loop()
{
  // ===== READ SENSORS =====
  float current = ina219.getCurrent_mA();

  float busVoltage = ina219.getBusVoltage_V();
  float shuntVoltage = ina219.getShuntVoltage_mV() / 1000;
  float voltage = busVoltage + shuntVoltage;

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // ===== VALIDATION =====
  if (isnan(temp) || isnan(hum))
  {
    Serial.println("DHT ERROR → Skipping upload");
    delay(2000);
    return;
  }

  // Remove noise / fake values
  if (current < 0) current = 0;
  if (voltage < 0) voltage = 0;

  // ===== DEBUG PRINT =====
  Serial.print("Current: "); Serial.print(current);
  Serial.print(" | Voltage: "); Serial.print(voltage);
  Serial.print(" | Temp: "); Serial.print(temp);
  Serial.print(" | Humidity: "); Serial.println(hum);

  // ===== SEND DATA =====
  sendToSupabase(current, voltage, temp, hum);

  // ===== OLED DISPLAY (optional) =====
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(0,0);
  display.print("I: "); display.print(current); display.println(" mA");

  display.print("V: "); display.print(voltage); display.println(" V");

  display.print("T: "); display.print(temp); display.println(" C");

  display.print("H: "); display.print(hum); display.println(" %");

  display.display();

  // ===== DELAY =====
  delay(5000);
}
