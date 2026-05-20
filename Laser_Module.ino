#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128,64,&Wire,-1);

int laserPin = 33;
int buttonPin = 12;

bool laserState = false;
bool lastButton = HIGH;

void setup()
{
pinMode(laserPin, OUTPUT);
pinMode(buttonPin, INPUT_PULLUP);

Wire.begin(26,27);

display.begin(SSD1306_SWITCHCAPVCC,0x3C);
display.clearDisplay();
display.setTextColor(WHITE);
}

void loop()
{

bool button = digitalRead(buttonPin);

if(button == LOW && lastButton == HIGH)
{
laserState = !laserState;
digitalWrite(laserPin, laserState);
delay(200);
}

lastButton = button;

display.clearDisplay();
display.setTextSize(2);
display.setCursor(10,20);

if(laserState)
{
display.println("LASER ON");
}
else
{
display.println("LASER OFF");
}

display.display();

}