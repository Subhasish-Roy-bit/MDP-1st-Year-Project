// ============================================================
//   EV BATTERY MONITORING SYSTEM - FULL INTEGRATED MODULE
//   Author  : Subhasish Roy (25BCE5327)
//   College : Vellore Institute of Technology, Chennai
//   Board   : ESP32 Development Board
// ============================================================
//
//  ── PIN SUMMARY ──────────────────────────────────────────
//  GPIO 26 (SDA)  → OLED + INA219 (shared I2C data line)
//  GPIO 27 (SCL)  → OLED + INA219 (shared I2C clock line)
//  GPIO 14        → DHT11 data pin  (1-Wire)
//  GPIO 32        → NTC Thermistor  (ADC analog input)
//  GPIO 21        → Thermistor module digital OUT (comparator)
//  GPIO 16        → Buzzer          (digital output ~2.4 kHz)
//  GPIO 25        → Relay module    (digital output, optocoupler)
//  GPIO 33        → Laser diode     (digital output)
//  GPIO 13        → LED indicator   (digital output)
//  GPIO 18        → Push button     (INPUT_PULLUP, authoritative)
//  ─────────────────────────────────────────────────────────
//
//  ── OLED SCREENS (7 screens, button short press cycles) ──
//  Screen 0 → WiFi: OK/ERR  |  Supabase: OK/ERR  (connection)
//  Screen 1 → Live current (mA)  from INA219
//  Screen 2 → Bus voltage  (V)   from INA219
//  Screen 3 → Temperature  (°C)  from DHT11
//  Screen 4 → Humidity     (%)   from DHT11
//  Screen 5 → [RELAY] ON / OFF   (long press toggles + uploads)
//  Screen 6 → BUZZ: ON / OFF     (long press toggles + uploads)
//  ─────────────────────────────────────────────────────────
//
//  ── SUPABASE UPLOAD (every 5 seconds, non-blocking) ──────
//  Table    : ev_data
//  Columns  : current_mA, voltage_V, temperature_C,
//             humidity_pct, relay_state, buzzer_state
//  ─────────────────────────────────────────────────────────

// ── LIBRARIES ────────────────────────────────────────────
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <DHT.h>
#include <math.h>

// ── WiFi CREDENTIALS ─────────────────────────────────────
#define WIFI_SSID     " "
#define WIFI_PASSWORD " "

// ── SUPABASE CREDENTIALS ──────────────────────────────────
const char* SUPABASE_URL = " ";
const char* SUPABASE_KEY = " ";

// ── PIN DEFINITIONS ───────────────────────────────────────
#define SDA_PIN        26   // I2C SDA  → OLED + INA219
#define SCL_PIN        27   // I2C SCL  → OLED + INA219
#define DHT_PIN        14   // DHT11 data
#define THERMISTOR_AO  32   // NTC thermistor analog out
#define THERMISTOR_DO  21   // Thermistor module digital out
#define BUZZER_PIN     16   // Buzzer
#define RELAY_PIN      25   // Relay module
#define LASER_PIN      33   // Laser diode
#define LED_PIN        13   // LED indicator
#define BUTTON_PIN     18   // Push button (INPUT_PULLUP)

// ── HARDWARE OBJECTS ──────────────────────────────────────
#define DHTTYPE DHT11
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Adafruit_INA219   ina219;
DHT               dht(DHT_PIN, DHTTYPE);

// ── STATE VARIABLES ───────────────────────────────────────
int  screen      = 0;
bool relayState  = false;
bool buzzerState = false;
bool wifiOK      = false;
bool supabaseOK  = false;

// Button timing
bool          btnPressed  = false;
unsigned long pressStart  = 0;

// Non-blocking upload timer
unsigned long lastUpload  = 0;
const unsigned long UPLOAD_INTERVAL = 5000; // 5 seconds

// ── THERMISTOR HELPER ─────────────────────────────────────
float readThermistorTemp()
{
  int   raw        = analogRead(THERMISTOR_AO);
  float voltage    = raw * (3.3f / 4095.0f);
  float resistance = (3.3f - voltage) * 10000.0f / voltage;
  float tempK      = 1.0f / (log(resistance / 10000.0f) / 3950.0f + 1.0f / 298.15f);
  float tempC      = tempK - 273.15f;
  // NOTE: calibrationOffset must be derived physically before deployment
  // Default -50 is a placeholder only
  return tempC - 50.0f;
}

// ── SUPABASE UPLOAD ───────────────────────────────────────
bool uploadToSupabase(float current, float voltage,
                      float temp,    float hum,
                      bool  relay,   bool  buzzer)
{
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin(SUPABASE_URL);
  http.addHeader("Content-Type",  "application/json");
  http.addHeader("apikey",        SUPABASE_KEY);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
  http.addHeader("Prefer",        "return=minimal");

  String json = "{";
  json += "\"current_mA\":"     + String(current, 2) + ",";
  json += "\"voltage_V\":"      + String(voltage, 2) + ",";
  json += "\"temperature_C\":"  + String(temp,    2) + ",";
  json += "\"humidity_pct\":"   + String(hum,     2) + ",";
  json += "\"relay_state\":"    + String(relay  ? "true" : "false") + ",";
  json += "\"buzzer_state\":"   + String(buzzer ? "true" : "false");
  json += "}";

  int code = http.POST(json);
  http.end();

  Serial.print("[Supabase] HTTP Response: ");
  Serial.println(code);

  return (code == 200 || code == 201);
}

// ── OLED DISPLAY HELPER ───────────────────────────────────
void showScreen(int scr,
                float current, float voltage,
                float temp,    float hum,
                bool  relay,   bool  buzzer)
{
  display.clearDisplay();
  display.setTextColor(WHITE);

  switch (scr)
  {
    case 0:  // Connection status
      display.setTextSize(1);
      display.setCursor(0, 10);
      display.print("WiFi:     ");
      display.println(wifiOK     ? "OK" : "ERR");
      display.print("Supabase: ");
      display.println(supabaseOK ? "OK" : "ERR");
      break;

    case 1:  // Current
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("-- Current --");
      display.setTextSize(2);
      display.setCursor(0, 20);
      display.print(current, 1);
      display.println(" mA");
      break;

    case 2:  // Voltage
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("-- Voltage --");
      display.setTextSize(2);
      display.setCursor(0, 20);
      display.print(voltage, 2);
      display.println(" V");
      break;

    case 3:  // Temperature
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("-- Temp (DHT11) --");
      display.setTextSize(2);
      display.setCursor(0, 20);
      display.print(temp, 1);
      display.println(" C");
      break;

    case 4:  // Humidity
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("-- Humidity --");
      display.setTextSize(2);
      display.setCursor(0, 20);
      display.print(hum, 1);
      display.println(" %");
      break;

    case 5:  // Relay control
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Long press = toggle");
      display.setTextSize(2);
      display.setCursor(0, 25);
      display.print("[RELAY] ");
      display.println(relay ? "ON" : "OFF");
      break;

    case 6:  // Buzzer control
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Long press = toggle");
      display.setTextSize(2);
      display.setCursor(0, 25);
      display.print("BUZZ: ");
      display.println(buzzer ? "ON" : "OFF");
      break;
  }

  display.display();
}

// ═══════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════
void setup()
{
  Serial.begin(115200);

  // ── GPIO setup ──────────────────────────────────────────
  pinMode(THERMISTOR_DO, INPUT);
  pinMode(BUZZER_PIN,    OUTPUT);
  pinMode(RELAY_PIN,     OUTPUT);
  pinMode(LASER_PIN,     OUTPUT);
  pinMode(LED_PIN,       OUTPUT);
  pinMode(BUTTON_PIN,    INPUT_PULLUP);

  digitalWrite(RELAY_PIN,  LOW);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LASER_PIN,  LOW);
  digitalWrite(LED_PIN,    LOW);

  // ── I2C + OLED ──────────────────────────────────────────
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println("EV Monitor v6");
  display.println("Initialising...");
  display.display();
  delay(1500);

  // ── INA219 + DHT11 ──────────────────────────────────────
  ina219.begin();
  dht.begin();

  // ── WiFi connection ─────────────────────────────────────
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println("Connecting WiFi...");
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  wifiOK = (WiFi.status() == WL_CONNECTED);

  display.clearDisplay();
  display.setCursor(0, 20);
  if (wifiOK)
  {
    display.println("WiFi: CONNECTED");
    Serial.println("\n[WiFi] Connected!");
  }
  else
  {
    display.println("WiFi: FAILED");
    Serial.println("\n[WiFi] Failed - continuing offline");
  }
  display.display();
  delay(1500);
}

// ═══════════════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════════════
void loop()
{
  // ── 1. READ BUTTON ──────────────────────────────────────
  bool btn = (digitalRead(BUTTON_PIN) == LOW);

  if (btn && !btnPressed)
  {
    btnPressed = true;
    pressStart = millis();
  }

  if (!btn && btnPressed)
  {
    unsigned long duration = millis() - pressStart;
    btnPressed = false;

    if (duration < 400)
    {
      // Short press → advance screen
      screen++;
      if (screen > 6) screen = 0;
    }
    else if (duration > 800)
    {
      // Long press → toggle relay (screen 5) or buzzer (screen 6)
      if (screen == 5)
      {
        relayState = !relayState;
        digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
        Serial.print("[Relay] State: "); Serial.println(relayState);
      }
      if (screen == 6)
      {
        buzzerState = !buzzerState;
        digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
        Serial.print("[Buzzer] State: "); Serial.println(buzzerState);
      }
    }
  }

  // ── 2. READ ALL SENSORS ─────────────────────────────────
  float current = ina219.getCurrent_mA();
  float busV    = ina219.getBusVoltage_V();
  float shuntV  = ina219.getShuntVoltage_mV() / 1000.0f;
  float voltage = busV + shuntV;
  float temp    = dht.readTemperature();
  float hum     = dht.readHumidity();

  // Sanitise
  if (current < 0) current = 0;
  if (voltage < 0) voltage = 0;
  if (isnan(temp)) temp = 0;
  if (isnan(hum))  hum  = 0;

  // Thermistor digital alert → auto buzzer override
  if (digitalRead(THERMISTOR_DO) == LOW && !buzzerState)
  {
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else if (digitalRead(THERMISTOR_DO) == HIGH && !buzzerState)
  {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // LED mirrors relay
  digitalWrite(LED_PIN, relayState ? HIGH : LOW);

  // ── 3. OLED DISPLAY ─────────────────────────────────────
  showScreen(screen, current, voltage, temp, hum, relayState, buzzerState);

  // ── 4. SUPABASE UPLOAD (every 5 seconds) ────────────────
  if (millis() - lastUpload >= UPLOAD_INTERVAL)
  {
    lastUpload = millis();

    Serial.println("──────────────────────────────");
    Serial.print("Current: ");   Serial.print(current);   Serial.println(" mA");
    Serial.print("Voltage: ");   Serial.print(voltage);   Serial.println(" V");
    Serial.print("Temp:    ");   Serial.print(temp);      Serial.println(" C");
    Serial.print("Humidity:");   Serial.print(hum);       Serial.println(" %");
    Serial.print("Relay:   ");   Serial.println(relayState);
    Serial.print("Buzzer:  ");   Serial.println(buzzerState);

    supabaseOK = uploadToSupabase(current, voltage, temp, hum,
                                  relayState, buzzerState);
  }

  delay(300);
}
