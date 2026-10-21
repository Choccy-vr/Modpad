#pragma once

#include <Arduino.h>

// bit-banged I2C slave, the CH32V203F6P6 has no hardware I2C on its pins.
// a transfer runs entirely inside the SDA falling-edge interrupt with all other
// interrupts masked, holding SCL low (clock stretching) while the read packet is built.
// the bus is only treated as idle after a STOP, so traffic to other devices
// (the 400kHz display) can't be mistaken for a START addressed to us.
namespace SoftI2CSlave
{
    // fills buf with the bytes to send for a read, returns how many were written
    typedef uint8_t (*RequestHandler)(uint8_t *buf, uint8_t maxLen);

    // both pins must be on the same GPIO port and have external pullups
    void begin(uint8_t address, uint32_t sclPin, uint32_t sdaPin, RequestHandler onRequest);
}
