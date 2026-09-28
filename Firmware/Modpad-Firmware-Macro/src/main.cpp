#include <Arduino.h>
#include "Modules/Modules.h"
const unsigned long DEBOUNCE_MS = 20;

// index in this array is the button id sent to the main module
Button buttons[] = {
    {1, HIGH, false, 0},
    {2, HIGH, false, 0},
    {4, HIGH, false, 0},
    {3, HIGH, false, 0},
};
const size_t NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

void setup()
{
  Serial.begin(115200);
  for (size_t i = 0; i < NUM_BUTTONS; i++)
  {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }

  ModpadModules::setup();
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
        ModpadModules::pressButton(i);
      }
      b.pressed = isPressed;
    }
  }
}
