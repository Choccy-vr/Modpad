#include "Modules.h"
#include <Wire.h>
namespace ModpadModules
{
#define I2C_ADDRESS 0x21
#define SDA_PIN 6
#define SCL_PIN 7
// shared open-drain line, main module listens for FALLING
#define INT_PIN 29

// satellite packet: [flags, button index]
#define PACKET_SIZE 2
#define FLAG_VALID 0x01

#define QUEUE_SIZE 16

    uint8_t queue[QUEUE_SIZE][PACKET_SIZE];
    volatile uint8_t queueHead = 0;
    volatile uint8_t queueTail = 0;

    // open-drain: only ever drive LOW, release to let pullup take it HIGH
    void assertInt()
    {
        pinMode(INT_PIN, OUTPUT);
        digitalWrite(INT_PIN, LOW);
    }

    void releaseInt()
    {
        pinMode(INT_PIN, INPUT);
    }

    void onRequest()
    {
        uint8_t packet[PACKET_SIZE] = {0, 0}; // flags = 0 means nothing pressed

        if (queueHead != queueTail)
        {
            memcpy(packet, queue[queueTail], PACKET_SIZE);
            queueTail = (queueTail + 1) % QUEUE_SIZE;
        }

        Wire1.write(packet, PACKET_SIZE);

        if (queueHead == queueTail)
            releaseInt();
    }

    void setup()
    {
        releaseInt();

        Wire1.setSDA(SDA_PIN);
        Wire1.setSCL(SCL_PIN);
        Wire1.begin(I2C_ADDRESS);
        Wire1.onRequest(onRequest);

        syncModule();
    }

    void syncModule()
    {
        assertInt();
    }

    void pressButton(uint8_t index)
    {
        noInterrupts();
        uint8_t next = (queueHead + 1) % QUEUE_SIZE;
        if (next == queueTail)
        {
            interrupts();
            Serial.println("Button queue full, dropping " + String(index));
            return;
        }

        queue[queueHead][0] = FLAG_VALID;
        queue[queueHead][1] = index;
        queueHead = next;
        // if INT is already low the main module is mid-drain and will pick this up
        assertInt();
        interrupts();
    }
}
