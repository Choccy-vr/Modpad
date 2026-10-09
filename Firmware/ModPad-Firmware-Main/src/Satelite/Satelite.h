#pragma once

#include <Arduino.h>
#include <map>
#include <cstring>

namespace ModpadSatelite
{
    struct KeyAction
    {
        uint16_t code;
        uint8_t modifiers;
        bool consumer;
    };

    struct Keymap
    {
        KeyAction *actions;
        size_t count;
    };

    // compare module names by content, not pointer
    struct NameLess
    {
        bool operator()(const char *a, const char *b) const { return strcmp(a, b) < 0; }
    };

    // Keymaps per module name, defined in Satelite.cpp
    extern std::map<const char *, Keymap, NameLess> MODULE_KEYMAPS;
    // Connection state per module name, defined in Satelite.cpp
    extern std::map<const char *, bool, NameLess> ACTIVE_MODULES;

    // Configures Satelite Modules
    void setup();
    // poll for new i2c modules
    bool searchNewModule();
    // poll for new i2c data to be recieved
    void searchSlaveData();
    // init new satelite module
    void initSateliteModule(String name);
    // check i2c event
    void checkI2CEvent();
    // re-scan module addresses on a timer, catches removed modules, call every loop()
    void pollModulePresence();

}
