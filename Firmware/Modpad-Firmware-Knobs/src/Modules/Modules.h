#pragma once

#include <Arduino.h>

struct Encoder
{
    uint8_t pinA;
    uint8_t pinB;
    // flip if turning clockwise reports CCW
    bool reversed;
    uint8_t lastState;
    int8_t accum;
};

namespace ModpadModules
{
    // Configures Satelite Module
    void setup();
    // Syncs with main module
    void syncModule();
    // send i2c event for one detent, direction 0 = CW, 1 = CCW
    void stepEncoder(uint8_t encoder, uint8_t direction);

}
