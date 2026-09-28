#pragma once

#include <Arduino.h>

namespace ModpadConfigurator
{
    // Sets a Binding in the keymap
    bool setKeymapBinding(const char *moduleName, size_t index, uint16_t code, uint8_t modifiers, bool consumer);
    // Sends all Bindings to the configurator
    void sendAllKeymapsJson();
    // Sends which modules are active
    void sendActiveModulesJson();
    // Processes Configurator Command
    void processConfigCommand(const String &payload);
    // Non-blocking serial listener
    void processConfiguratorSerial();

}
