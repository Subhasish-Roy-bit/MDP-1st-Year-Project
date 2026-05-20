#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

int digitalPin = 21;
int buzzerPin = 16;
int relayPin = 25;
int buttonPin = 12;

int screen = 0;
int lastButton = HIGH;

bool relayState = false;

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

        if (screen > 1)
        {
            screen = 0;
        }

        delay(200);
    }

    lastButton = buttonState;

    int sensorState = digitalRead(digitalPin);

    if (sensorState == LOW)
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

        if (sensorState == LOW)
        {
            display.print("ALERT");
        }
        else
        {
            display.print("NORMAL");
        }
    }

    if (screen == 1)
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