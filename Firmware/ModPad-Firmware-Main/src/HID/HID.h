#pragma once

#include <Arduino.h>

namespace ModpadHID
{
    // Configures USB HID
    void setup();
    // Tap Key
    void tapKey(uint8_t key, uint8_t modifiers);
    // Tap Consumer Control usage (16-bit, e.g. HID_USAGE_CONSUMER_*)
    void tapConsumer(uint16_t usage);
}
