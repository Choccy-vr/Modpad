#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "HID/HID.h"
#include "Display/Display.h"
#include <Wire.h>
const unsigned long DEBOUNCE_MS = 20;

struct Button
{
    uint8_t pin;
    uint16_t code;
    uint8_t modifiers;
    bool consumer;
    bool lastReading;
    bool pressed;
    unsigned long lastChangeTime;
};

Button buttons[] = {
    {28, HID_USAGE_CONSUMER_VOLUME_INCREMENT, 0, true, HIGH, false, 0},
    {27, HID_USAGE_CONSUMER_VOLUME_DECREMENT, 0, true, HIGH, false, 0},
    {26, HID_KEY_C, 0, false, HIGH, false, 0},
};
const size_t NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

void setup()
{
    Serial.begin(115200);
    Wire1.setSDA(6);
    Wire1.setSCL(7);
    for (size_t i = 0; i < NUM_BUTTONS; i++)
    {
        pinMode(buttons[i].pin, INPUT_PULLUP);
    }

    ModpadHID::setup();
    ModpadDisplay::setup();
}

void loop()
{
    unsigned long now = millis();

    for (size_t i = 0; i < NUM_BUTTONS; i++)
    {
        Button &b = buttons[i];
        bool reading = digitalRead(b.pin);

        if (reading != b.lastReading)
        {
            b.lastChangeTime = now;
            b.lastReading = reading;
        }

        if (now - b.lastChangeTime >= DEBOUNCE_MS)
        {
            bool isPressed = (reading == LOW);
            if (isPressed && !b.pressed)
            {
                if (b.consumer)
                    ModpadHID::tapConsumer(b.code);
                else
                    ModpadHID::tapKey(b.code, b.modifiers);
                ModpadDisplay::displayText(b.code + " was just pressed");
            }
            b.pressed = isPressed;
        }
    }
}
