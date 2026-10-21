#include "Modules.h"
#include "SoftI2CSlave.h"
namespace ModpadModules
{
#define I2C_ADDRESS 0x20
#define SDA_PIN PA1
#define SCL_PIN PA0
// shared open-drain line, main module listens for FALLING
#define INT_PIN PA4

// satellite packet: [flags, index]
#define PACKET_SIZE 2
#define FLAG_VALID 0x01
// index = encoder * 2 + direction (0 = CW, 1 = CCW)
#define FLAG_ENCODER 0x02

#define QUEUE_SIZE 16

    uint8_t queue[QUEUE_SIZE][PACKET_SIZE];
    volatile uint8_t queueHead = 0;
    volatile uint8_t queueTail = 0;

    // open-drain: only ever drive LOW, release to let pullup take it HIGH
    void assertInt()
    {
        // latch LOW first so the pin never drives the shared line HIGH
        digitalWrite(INT_PIN, LOW);
        pinMode(INT_PIN, OUTPUT_OD);
    }

    void releaseInt()
    {
        pinMode(INT_PIN, INPUT);
    }

    uint8_t onRequest(uint8_t *packet, uint8_t maxLen)
    {
        packet[0] = 0; // flags = 0 means no event
        packet[1] = 0;

        if (queueHead != queueTail)
        {
            memcpy(packet, queue[queueTail], PACKET_SIZE);
            queueTail = (queueTail + 1) % QUEUE_SIZE;
        }

        if (queueHead == queueTail)
            releaseInt();

        return PACKET_SIZE;
    }

    void setup()
    {
        releaseInt();

        SoftI2CSlave::begin(I2C_ADDRESS, SCL_PIN, SDA_PIN, onRequest);

        syncModule();
    }

    void syncModule()
    {
        assertInt();
    }

    void stepEncoder(uint8_t encoder, uint8_t direction)
    {
        uint8_t index = encoder * 2 + direction;

        noInterrupts();
        uint8_t next = (queueHead + 1) % QUEUE_SIZE;
        if (next == queueTail)
        {
            interrupts();
            Serial.println("Encoder queue full, dropping step " + String(index));
            return;
        }

        queue[queueHead][0] = FLAG_VALID | FLAG_ENCODER;
        queue[queueHead][1] = index;
        queueHead = next;
        // if INT is already low the main module is mid-drain and will pick this up
        assertInt();
        interrupts();
    }
}
