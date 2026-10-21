#include <Arduino.h>
#include "Modules/Modules.h"

// index in this array is the encoder id sent to the main module
Encoder encoders[] = {
    {PA7, PB1, false, 0, 0, 0}, // SW1
    {PA5, PA6, false, 0, 0, 0}, // SW2
};
const size_t NUM_ENCODERS = sizeof(encoders) / sizeof(encoders[0]);

const int8_t QUAD_TABLE[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
const uint8_t DETENT_STATE = 0b11;

uint8_t readEncoder(const Encoder &e)
{
  return (digitalRead(e.pinA) << 1) | digitalRead(e.pinB);
}

void setup()
{
  Serial.begin(115200);
  for (size_t i = 0; i < NUM_ENCODERS; i++)
  {
    pinMode(encoders[i].pinA, INPUT_PULLUP);
    pinMode(encoders[i].pinB, INPUT_PULLUP);
    encoders[i].lastState = readEncoder(encoders[i]);
  }

  ModpadModules::setup();
}

void loop()
{
  for (size_t i = 0; i < NUM_ENCODERS; i++)
  {
    Encoder &e = encoders[i];
    uint8_t state = readEncoder(e);
    if (state == e.lastState)
      continue;

    int8_t delta = QUAD_TABLE[(e.lastState << 2) | state];
    // both pins changed at once: a state went by while the I2C interrupt held the CPU,
    // so count it as two steps the way the knob was already turning
    if (delta == 0)
      delta = 2 * e.lastDir;
    else
      e.lastDir = delta;

    e.accum += delta;
    e.lastState = state;

    // only count a step once the knob settles in a detent
    if (state == DETENT_STATE)
    {
      if (e.accum >= 2 || e.accum <= -2)
      {
        bool cw = (e.accum > 0) != e.reversed;
        ModpadModules::stepEncoder(i, cw ? 0 : 1);
      }
      e.accum = 0;
    }
    // a whole detent went by without the detent state itself being seen
    else if (e.accum >= 4 || e.accum <= -4)
    {
      bool cw = (e.accum > 0) != e.reversed;
      ModpadModules::stepEncoder(i, cw ? 0 : 1);
      e.accum -= e.accum > 0 ? 4 : -4;
    }
  }
}
