#include <Arduino.h>
#include "Modules/Modules.h"

// index in this array is the encoder id sent to the main module
Encoder encoders[] = {
    {1, 2, false, 0, 0},
    {3, 27, false, 0, 0},
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

    e.accum += QUAD_TABLE[(e.lastState << 2) | state];
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
  }
}
