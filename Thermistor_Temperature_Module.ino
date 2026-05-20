#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

int analogPin = 32;
int digitalPin = 17;
int buzzerPin = 16;
int relayPin  = 25;
int buttonPin = 12;

int screen = 0;
int lastButton = HIGH;

bool relayState = false;

float calibrationOffset = -50;

float readTemperature()
{
    int raw = analogRead(analogPin);

    float voltage = raw * (3.3 / 4095.0);

    float resistance = (3.3 - voltage) * 10000.0 / voltage;

    float tempK = 1.0 / (log(resistance / 10000.0) / 3950.0 + 1.0 / 298.15);

    float tempC = tempK - 273.15;

    tempC = tempC + calibrationOffset;

    return tempC;
}

void setup()
{
    Wire.begin(26, 27);

    pinMode(digitalPin, INPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(relayPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.setTextColor(WHITE);
}

void loop()
{
    int buttonState = digitalRead(buttonPin);

    if (buttonState == LOW && lastButton == HIGH)
    {
        screen++;

        if (screen > 2)
        {
            screen = 0;
        }

        delay(200);
    }

    lastButton = buttonState;

    float temperature = readTemperature();

    int digitalValue = digitalRead(digitalPin);

    if (digitalValue == LOW)
    {
        digitalWrite(buzzerPin, HIGH);
    }
    else
    {
        digitalWrite(buzzerPin, LOW);
    }

    display.clearDisplay();

    if (screen == 0)
    {
        display.setTextSize(2);
        display.setCursor(0, 20);
        display.print("Temp ");
        display.print(temperature);
        display.print(" C");
    }

    if (screen == 1)
    {
        display.setTextSize(2);
        display.setCursor(0, 20);

        if (digitalValue == LOW)
        {
            display.print("ALERT");
        }
        else
        {
            display.print("NORMAL");
        }
    }

    if (screen == 2)
    {
        display.setTextSize(2);
        display.setCursor(0, 20);

        if (relayState)
        {
            display.print("Relay ON");
        }
        else
        {
            display.print("Relay OFF");
        }

        if (buttonState == LOW)
        {
            relayState = !relayState;
            digitalWrite(relayPin, relayState);
            delay(300);
        }
    }

    display.display();

    delay(1000);
}