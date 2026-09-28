#pragma once

#include <Arduino.h>

struct Button
{
    uint8_t pin;
    bool lastReading;
    bool pressed;
    unsigned long lastChangeTime;
};

namespace ModpadModules
{
    // Configures Satelite Module
    void setup();
    // Syncs with main module
    void syncModule();
    // send i2c event
    void pressButton(uint8_t index);

}
