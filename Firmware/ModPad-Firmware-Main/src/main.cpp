#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "HID/HID.h"

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
    {1, HID_USAGE_CONSUMER_VOLUME_INCREMENT, 0, true, HIGH, false, 0},
    {2, HID_USAGE_CONSUMER_VOLUME_DECREMENT, 0, true, HIGH, false, 0},
    {4, HID_KEY_C, KEYBOARD_MODIFIER_LEFTCTRL, false, HIGH, false, 0},
    {3, HID_KEY_V, KEYBOARD_MODIFIER_LEFTCTRL, false, HIGH, false, 0},
};
const size_t NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

void setup()
{
    Serial.begin(115200);
    for (size_t i = 0; i < NUM_BUTTONS; i++)
    {
        pinMode(buttons[i].pin, INPUT_PULLUP);
    }
    ModpadHID::setup();
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
            }
            b.pressed = isPressed;
        }
    }
}
