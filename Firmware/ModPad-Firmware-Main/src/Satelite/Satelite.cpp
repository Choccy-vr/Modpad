#include "Satelite.h"
#include <Adafruit_TinyUSB.h>
#include <map>
#include <cstring>
#include <Wire.h>
#include "../Display/Display.h"
#include "../HID/HID.h"
#include "../Configurator/Configurator.h"
namespace ModpadSatelite
{
#define INT_PIN 29

// satellite packet: [flags, button index]
#define PACKET_SIZE 2
#define FLAG_VALID 0x01
#define FLAG_ENCODER 0x02 // index = encoder * 2 + direction (0 = CW, 1 = CCW)
// max packets drained from a queueing module (Knobs) per poll, matches its queue depth
#define MAX_PACKETS_PER_POLL 16
// unplugged modules can't pull INT, so presence is also re-checked on this interval
#define PRESENCE_POLL_MS 250

    // Macro module keymap, indexed by the button index the module sends
    KeyAction MACRO_KEYMAP[] = {
        {HID_USAGE_CONSUMER_VOLUME_INCREMENT, 0, true},
        {HID_USAGE_CONSUMER_VOLUME_DECREMENT, 0, true},
        {HID_KEY_C, 0, false},
        {HID_USAGE_CONSUMER_VOLUME_INCREMENT, 0, true},
    };

    // Knobs module keymap, indexed by encoder * 2 + direction (0 = CW, 1 = CCW)
    KeyAction KNOBS_KEYMAP[] = {
        {HID_USAGE_CONSUMER_VOLUME_INCREMENT, 0, true},     // encoder 1 CW
        {HID_USAGE_CONSUMER_VOLUME_DECREMENT, 0, true},     // encoder 1 CCW
        {HID_USAGE_CONSUMER_BRIGHTNESS_INCREMENT, 0, true}, // encoder 2 CW
        {HID_USAGE_CONSUMER_BRIGHTNESS_DECREMENT, 0, true}, // encoder 2 CCW
    };

    std::map<const char *, Keymap, NameLess> MODULE_KEYMAPS = {
        {"Knobs", {KNOBS_KEYMAP, sizeof(KNOBS_KEYMAP) / sizeof(KNOBS_KEYMAP[0])}},
        {"Macro", {MACRO_KEYMAP, sizeof(MACRO_KEYMAP) / sizeof(MACRO_KEYMAP[0])}},
    };

    volatile bool i2cEventPending = false;

    std::map<const char *, uint8_t, NameLess> MODULE_ADDRESSES = {
        {"Knobs", 0x20},
        {"Macro", 0x21},
        {"Slider", 0x22},
    };

    std::map<const char *, bool, NameLess> ACTIVE_MODULES = {
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

        if (digitalRead(INT_PIN) == LOW)
            i2cEventPending = true;
    }

    // poll for new i2c modules
    bool searchNewModule()
    {
        bool foundNewModule = false;
        bool modulesChanged = false;

        for (auto &[name, addr] : MODULE_ADDRESSES)
        {
            Wire1.beginTransmission(addr);
            bool present = Wire1.endTransmission() == 0;

            if (present && !ACTIVE_MODULES[name])
            {
                ACTIVE_MODULES[name] = true;
                foundNewModule = true;
                modulesChanged = true;
                ModpadConfigurator::sendLogJson("Found " + String(name) + " at address " + String(addr));
                initSateliteModule(name);
            }
            else if (!present && ACTIVE_MODULES[name])
            {
                ACTIVE_MODULES[name] = false;
                modulesChanged = true;
                ModpadConfigurator::sendLogJson("Removed " + String(name) + " at address " + String(addr));
                ModpadDisplay::displayText(String(name) + " was removed");
            }
        }

        // push the new module state so the configurator doesn't have to poll
        if (modulesChanged)
            ModpadConfigurator::sendActiveModulesJson();

        return foundNewModule;
    }
    // read and handle one packet, returns true if it held an event
    bool readSlavePacket(const char *name, uint8_t addr)
    {
        // expects 2 bytes: flags, button index
        if (Wire1.requestFrom(addr, (uint8_t)PACKET_SIZE) != PACKET_SIZE)
        {
            ModpadConfigurator::sendLogJson("No response from " + String(name));
            while (Wire1.available())
                Wire1.read(); // drain partial data
            return false;
        }

        uint8_t flags = Wire1.read();
        uint8_t index = Wire1.read();

        if (!(flags & FLAG_VALID))
            return false; // nothing pressed

        bool encoder = flags & FLAG_ENCODER;
        ModpadConfigurator::sendPressNotificationJson(name, index, encoder);
        // encoder index packs encoder number and direction
        String event = encoder ? String(name) + " enc " + String(index / 2 + 1) + (index % 2 == 0 ? " CW" : " CCW")
                               : String(name) + " button " + String(index) + " pressed";

        if (encoder)
            ModpadConfigurator::sendLogJson(event);
        else
            ModpadConfigurator::sendLogJson("Button " + String(index) + " recieved from " + String(name));

        auto keymap = MODULE_KEYMAPS.find(name);
        if (keymap == MODULE_KEYMAPS.end() || index >= keymap->second.count)
        {
            ModpadConfigurator::sendLogJson("No keymap for " + String(encoder ? "encoder step " : "button ") + String(index) + " on " + String(name));
            return true;
        }

        const KeyAction &action = keymap->second.actions[index];
        if (action.consumer)
            ModpadHID::tapConsumer(action.code);
        else
            ModpadHID::tapKey((uint8_t)action.code, action.modifiers);

        // mark the display dirty if it is a encoder
        if (encoder)
            ModpadDisplay::setText(event);
        else
            ModpadDisplay::displayText(event);
        return true;
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
            bool isKnobs = strcmp(name, "Knobs") == 0;
            int maxPackets = isKnobs ? MAX_PACKETS_PER_POLL : 1;
            for (int i = 0; i < maxPackets; i++)
            {
                if (!readSlavePacket(name, addr))
                    break;
                if (digitalRead(INT_PIN) == HIGH)
                    break; // all queues empty
            }
        }
    }
    void initSateliteModule(String name)
    {
        ModpadDisplay::displayText(name + " was just connected!");
    }
    // re-scan module addresses on a timer, call every loop()
    void pollModulePresence()
    {
        static unsigned long lastPoll = 0;
        unsigned long now = millis();
        if (now - lastPoll < PRESENCE_POLL_MS)
            return;
        lastPoll = now;

        // pushes a modules message to the configurator if anything changed
        searchNewModule();
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
