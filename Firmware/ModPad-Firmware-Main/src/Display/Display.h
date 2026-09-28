#pragma once

#include <Arduino.h>

namespace ModpadDisplay
{
    // Configures Display
    void setup();
    // Display Text
    void displayText(String msg);
    // Set text without redrawing, drawn by update()
    void setText(String msg);
    // Redraw pending text, at most every ~100ms, call every loop()
    void update();
}
