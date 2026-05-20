#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>

Adafruit_SSD1306 display(128,64,&Wire,-1);
Adafruit_INA219 ina219;

int relayPin = 25;
int ledPin = 13;

void setup()
{
Serial.begin(115200); 

pinMode(relayPin,OUTPUT);
pinMode(ledPin,OUTPUT);

Wire.begin(26,27);

display.begin(SSD1306_SWITCHCAPVCC,0x3C);
display.clearDisplay();
display.setTextSize(2);
display.setTextColor(WHITE);
display.setCursor(10,20);
display.println("START");
display.display();

ina219.begin();
}

void loop()
{
float current = ina219.getCurrent_mA();

display.clearDisplay();
display.setTextSize(1);
display.setCursor(0,10);
display.print("Current:");
display.print(current);
display.println(" mA");
display.display();

digitalWrite(relayPin,LOW);
digitalWrite(ledPin,HIGH);
delay(2000);

digitalWrite(relayPin,HIGH);
digitalWrite(ledPin,LOW);
delay(2000);
}