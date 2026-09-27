#pragma once

#include <Arduino.h>

namespace ModpadSatelite
{
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

}
