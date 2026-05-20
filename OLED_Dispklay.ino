#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <DHT.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define DHTPIN 14
#define DHTTYPE DHT11

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_INA219 ina219;
DHT dht(DHTPIN, DHTTYPE);

int buttonPin = 12;
int ledPin = 13;
int relayPin = 25;
int buzzerPin = 2;

int screen = 0;
int lastButton = HIGH;

void setup()
{
Serial.begin(115200);

pinMode(buttonPin, INPUT_PULLUP);
pinMode(ledPin, OUTPUT);
pinMode(relayPin, OUTPUT);
pinMode(buzzerPin, OUTPUT);

Wire.begin(26,27);

display.begin(SSD1306_SWITCHCAPVCC,0x3C);
display.clearDisplay();

ina219.begin();
dht.begin();
}

void loop()
{
int buttonState = digitalRead(buttonPin);

if(buttonState == LOW && lastButton == HIGH)
{
screen++;
if(screen > 2) screen = 0;

digitalWrite(buzzerPin,HIGH);
delay(100);   
digitalWrite(buzzerPin,LOW);
}

lastButton = buttonState;

display.clearDisplay();
display.setTextSize(2);
display.setTextColor(WHITE);

if(screen == 0)
{
float current = ina219.getCurrent_mA();

display.setCursor(0,10);
display.print("Current");
display.setCursor(0,35);
display.print(current);
display.print(" mA");
}

if(screen == 1)
{
float temp = dht.readTemperature();

display.setCursor(0,10);
display.print("Temp");
display.setCursor(0,35);
display.print(temp);
display.print(" C");
}

if(screen == 2)
{
float hum = dht.readHumidity();

display.setCursor(0,10);
display.print("Humidity");
display.setCursor(0,35);
display.print(hum);
display.print(" %");
}

display.display();

delay(200);
}