#include "Configurator.h"
#include "../Satelite/Satelite.h"
#include <ArduinoJson.h>

namespace ModpadConfigurator
{
    // Sets a Binding in the keymap
    bool setKeymapBinding(const char *moduleName, size_t index, uint16_t code, uint8_t modifiers, bool consumer)
    {
        auto keymap = ModpadSatelite::MODULE_KEYMAPS.find(moduleName);
        if (keymap == ModpadSatelite::MODULE_KEYMAPS.end())
        {
            return false;
        }

        ModpadSatelite::Keymap &targetKeymap = keymap->second;

        if (index >= targetKeymap.count)
        {
            return false;
        }

        targetKeymap.actions[index].code = code;
        targetKeymap.actions[index].modifiers = modifiers;
        targetKeymap.actions[index].consumer = consumer;
        return true;
    }

    // Sends all Bindings to the configurator
    void sendAllKeymapsJson()
    {
        Serial.print(F("{\"status\":\"layout\",\"modules\":{"));

        bool firstModule = true;
        for (const auto &pair : ModpadSatelite::MODULE_KEYMAPS)
        {
            if (!firstModule)
                Serial.print(F(","));
            firstModule = false;

            Serial.print(F("\""));
            Serial.print(pair.first);
            Serial.print(F("\":["));

            for (size_t i = 0; i < pair.second.count; i++)
            {
                const ModpadSatelite::KeyAction &act = pair.second.actions[i];
                Serial.print(F("{\"code\":"));
                Serial.print(act.code);
                Serial.print(F(",\"mod\":"));
                Serial.print(act.modifiers);
                Serial.print(F(",\"consumer\":"));
                Serial.print(act.consumer ? F("true") : F("false"));
                Serial.print(F("}"));

                if (i < pair.second.count - 1)
                    Serial.print(F(","));
            }
            Serial.print(F("]"));
        }

        Serial.println(F("}}")); // message terminator
    }

    // Sends which modules are active
    void sendActiveModulesJson()
    {
        Serial.print(F("{\"status\":\"modules\",\"modules\":{"));

        bool firstModule = true;
        for (const auto &pair : ModpadSatelite::ACTIVE_MODULES)
        {
            if (!firstModule)
                Serial.print(F(","));
            firstModule = false;

            Serial.print(F("\""));
            Serial.print(pair.first);
            Serial.print(F("\":"));
            Serial.print(pair.second ? F("true") : F("false"));
        }

        Serial.println(F("}}")); // message terminator
    }

    // Sends press notification, index matches the keymap index used by set_key
    void sendPressNotificationJson(const char *moduleName, uint8_t index, bool encoder)
    {
        Serial.print(F("{\"status\":\"press\",\"module\":{"));

        Serial.print(F("\"name\":\""));
        Serial.print(moduleName);
        Serial.print(F("\",\"index\":"));
        Serial.print(index);
        Serial.print(F(",\"type\":"));
        Serial.print(encoder ? F("\"encoder\"") : F("\"button\""));

        Serial.println(F("}}")); // message terminator
    }

    // Sends a debug message, keeps the serial stream pure JSON
    void sendLogJson(const String &msg)
    {
        Serial.print(F("{\"status\":\"log\",\"msg\":\""));

        for (size_t i = 0; i < msg.length(); i++)
        {
            char c = msg[i];
            if (c == '"' || c == '\\')
            {
                Serial.print('\\');
                Serial.print(c);
            }
            else if ((uint8_t)c >= 0x20)
            {
                Serial.print(c);
            }
            // control characters are dropped, they would break the JSON
        }

        Serial.println(F("\"}")); // message terminator
    }

    // Processes Configurator Command
    void processConfigCommand(const String &payload)
    {
        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (err)
            return;

        const char *cmd = doc["cmd"];
        if (!cmd)
            return;

        // request layout
        if (strcmp(cmd, "get_layout") == 0)
        {
            sendAllKeymapsJson();
        }
        // request connected modules
        if (strcmp(cmd, "get_modules") == 0)
        {
            sendActiveModulesJson();
        }
        // set new action
        if (strcmp(cmd, "set_key") == 0)
        {
            const char *module = doc["module"];
            size_t index = doc["index"];
            uint16_t code = doc["code"];
            uint8_t modifiers = doc["modifiers"] | 0;
            bool consumer = doc["consumer"] | false;

            if (module && setKeymapBinding(module, index, code, modifiers, consumer))
            {
                Serial.println(F("{\"status\":\"ok\"}"));
            }
            else
            {
                Serial.println(F("{\"status\":\"error\",\"msg\":\"invalid_key_or_module\"}"));
            }
        }
    }
    // Non-blocking serial listener
    void processConfiguratorSerial()
    {
        static String buffer = "";

        while (Serial.available() > 0)
        {
            char c = (char)Serial.read();
            if (c == '\n')
            {
                buffer.trim();
                if (buffer.length() > 0)
                {
                    processConfigCommand(buffer);
                }
                buffer = "";
            }
            else if (c != '\r')
            {
                buffer += c;
            }
        }
    }

}
