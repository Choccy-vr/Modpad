#include "Satelite.h"
#include <Adafruit_TinyUSB.h>
#include <map>
#include <Wire.h>
#include "../Display/Display.h"
#include "../HID/HID.h"
namespace ModpadSatelite
{
#define INT_PIN 29

// satellite packet: [flags, modifiers, code low, code high]
#define PACKET_SIZE 4
#define FLAG_VALID 0x01
#define FLAG_CONSUMER 0x02

    volatile bool i2cEventPending = false;

    std::map<const char *, uint8_t> MODULE_ADDRESSES = {
        {"Knobs", 0x20},
        {"Macro", 0x21},
        {"Slider", 0x22},
    };

    std::map<const char *, bool> ACTIVE_MODULES = {
        {"Knobs", false},
        {"Macro", false},
        {"Slider", false},
    };

    void onIntPin()
    {
        i2cEventPending = true;
    }

    void setup()
    {
        pinMode(INT_PIN, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(INT_PIN), onIntPin, FALLING);
    }

    // poll for new i2c modules
    bool searchNewModule()
    {
        bool foundNewModule = false;

        for (auto &[name, addr] : MODULE_ADDRESSES)
        {
            Wire.beginTransmission(addr);
            bool present = Wire.endTransmission() == 0;

            if (present && !ACTIVE_MODULES[name])
            {
                ACTIVE_MODULES[name] = true;
                foundNewModule = true;
                Serial.print("Found " + String(name) + " at address " + String(addr));
                initSateliteModule(name);
            }
            else if (!present && ACTIVE_MODULES[name])
            {
                ACTIVE_MODULES[name] = false;
                Serial.print("Removed " + String(name) + " at address " + String(addr));
            }
        }

        return foundNewModule;
    }
    // poll for new i2c data to be recieved
    void searchSlaveData()
    {
        for (auto &[name, addr] : MODULE_ADDRESSES)
        {
            if (!ACTIVE_MODULES[name])
            {
                continue;
            }
            // expects 4 bytes: flags, modifiers, code low, code high
            if (Wire.requestFrom(addr, (uint8_t)PACKET_SIZE) != PACKET_SIZE)
            {
                Serial.println("No response from " + String(name));
                while (Wire.available())
                    Wire.read(); // drain partial data
                continue;
            }

            uint8_t flags = Wire.read();
            uint8_t modifiers = Wire.read();
            uint16_t code = Wire.read();
            code |= (uint16_t)Wire.read() << 8;

            if (!(flags & FLAG_VALID))
                continue; // nothing pressed

            bool consumer = flags & FLAG_CONSUMER;
            Serial.println("Code " + String(code) + " recieved from " + String(name));
            if (consumer)
                ModpadHID::tapConsumer(code);
            else
                ModpadHID::tapKey((uint8_t)code, modifiers);
            ModpadDisplay::displayText(String(code) + " was just pressed");
        }
    }
    void initSateliteModule(String name)
    {
        ModpadDisplay::displayText(name + " was just connected!");
    }
    // check i2c event, call every loop()
    void checkI2CEvent()
    {
        // nothing happens unless the INT pin was pulled LOW
        if (!i2cEventPending)
            return;

        noInterrupts();
        i2cEventPending = false;
        interrupts();

        bool newModule = searchNewModule();
        if (!newModule)
        {
            searchSlaveData();
        }

        if (digitalRead(INT_PIN) == LOW)
            i2cEventPending = true;
    }
}
