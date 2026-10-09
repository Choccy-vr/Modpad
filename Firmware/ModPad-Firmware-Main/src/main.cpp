#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "HID/HID.h"
#include "Display/Display.h"
#include "Satelite/Satelite.h"
#include "Configurator/Configurator.h"
#include <Wire.h>

void setup()
{
    Serial.begin(115200);
    Wire1.setSDA(6);
    Wire1.setSCL(7);
    Wire1.begin();

    ModpadHID::setup();
    ModpadDisplay::setup();
    ModpadSatelite::setup();
}

void loop()
{
    ModpadSatelite::checkI2CEvent();
    ModpadSatelite::pollModulePresence();
    ModpadDisplay::update();
    ModpadConfigurator::processConfiguratorSerial();
}
