#include "Display.h"
#include <Adafruit_SSD1306.h>

namespace ModpadDisplay
{

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define MIN_REDRAW_MS 100 // throttle for setText() redraws

    // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1);

    String pendingText;
    bool dirty = false;
    unsigned long lastDrawTime = 0;

    void setup()
    {

        if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
        {
            Serial.println(F("SSD1306 allocation failed"));
            for (;;)
                ;
        }
    }

    void displayText(String msg)
    {
        display.clearDisplay();

        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 10);
        // Display static text
        display.println(msg);
        display.display();

        // an immediate draw supersedes any pending text
        dirty = false;
        lastDrawTime = millis();
    }

    void setText(String msg)
    {
        pendingText = msg;
        dirty = true;
    }

    void update()
    {
        if (!dirty || millis() - lastDrawTime < MIN_REDRAW_MS)
            return;
        displayText(pendingText);
    }

}
