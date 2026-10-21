#include "SoftI2CSlave.h"

// the polling loops are timing critical, -Os leaves them ~3x slower
#pragma GCC optimize("O2")

namespace SoftI2CSlave
{
// loop counts are rough, one filtered bus sample is ~14 cycles at 144MHz
#define LOOPS_PER_MS 10000
// how long the master may hold one clock phase before the transfer is abandoned
#define EDGE_TIMEOUT_LOOPS (5 * LOOPS_PER_MS)
// both lines high this long means no transfer is running, used when the STOP itself was missed
#define IDLE_LOOPS (LOOPS_PER_MS / 2)
// gives up following a bus that never goes idle (module unplugged, line stuck low)
#define RESYNC_TIMEOUT_LOOPS (20 * LOOPS_PER_MS)
// longer than any SCL high phase, so a running transfer always shows SCL dropping within it
#define SPIKE_CHECK_LOOPS (LOOPS_PER_MS / 20)
#define MAX_READ_SIZE 8

    // negative results from the bus readers, 0 and 1 are bit values
    enum Result
    {
        STOP = -1,
        RESTART = -2,
        TIMEOUT = -3,
    };

    GPIO_TypeDef *port;
    uint32_t sclMask;
    uint32_t sdaMask;
    uint32_t busMask;
    uint8_t ownAddress;
    RequestHandler requestHandler;

    // true once a STOP or idle bus has been seen, so the next SDA fall can only be a START.
    // without it any data edge on someone else's transfer could be mistaken for one
    bool synced = false;

    // open-drain: writing 1 releases the line, 0 pulls it low
    inline void sclLow() { port->BCR = sclMask; }
    inline void sclRelease() { port->BSHR = sclMask; }
    inline void sdaLow() { port->BCR = sdaMask; }
    inline void sdaRelease() { port->BSHR = sdaMask; }

    // samples both lines at once, a reading only counts once two in a row agree
    // so ringing on the clock edges can't add phantom bits
    __attribute__((always_inline)) inline uint32_t readBus()
    {
        uint32_t a = port->INDR & busMask;
        while (true)
        {
            uint32_t b = port->INDR & busMask;
            if (a == b)
                return b;
            a = b;
        }
    }

    // returns the bus sample once SCL reaches the level, or TIMEOUT
    int waitScl(bool high)
    {
        for (uint32_t i = 0; i < EDGE_TIMEOUT_LOOPS; i++)
        {
            uint32_t bus = readBus();
            if (((bus & sclMask) != 0) == high)
                return bus;
        }
        return TIMEOUT;
    }

    // clocks in one bit, SDA changing while SCL is high means START or STOP instead
    int readBit()
    {
        int bus = waitScl(true);
        if (bus < 0)
            return bus;

        uint32_t bit = bus & sdaMask;
        for (uint32_t i = 0; i < EDGE_TIMEOUT_LOOPS; i++)
        {
            uint32_t now = readBus();
            if (!(now & sclMask))
                return bit ? 1 : 0;
            if ((now & sdaMask) != bit)
                return bit ? RESTART : STOP;
        }
        return TIMEOUT;
    }

    // after a START SCL goes low before the first address bit. SDA rising first means
    // it was a spike on the idle bus (or an empty START+STOP), so there is nothing to follow
    int waitAfterStart()
    {
        for (uint32_t i = 0; i < EDGE_TIMEOUT_LOOPS; i++)
        {
            uint32_t now = readBus();
            if (!(now & sclMask))
                return 0;
            if (now & sdaMask)
                return STOP;
        }
        return TIMEOUT;
    }

    // returns the byte, or a negative Result
    int readByte()
    {
        int value = 0;
        for (uint8_t i = 0; i < 8; i++)
        {
            int bit = readBit();
            if (bit < 0)
                return bit;
            value = (value << 1) | bit;
        }
        return value;
    }

    // called while SCL is low after the 8th bit
    bool sendAck()
    {
        sdaLow();
        bool ok = waitScl(true) >= 0 && waitScl(false) >= 0;
        sdaRelease();
        return ok;
    }

    // returns the master's ACK bit (0 = ACK, 1 = NACK), or a negative Result
    int writeByte(uint8_t value)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            if (value & 0x80)
                sdaRelease();
            else
                sdaLow();
            value <<= 1;

            if (waitScl(true) < 0 || waitScl(false) < 0)
            {
                sdaRelease();
                return TIMEOUT;
            }
        }
        sdaRelease();
        return readBit();
    }

    // follows a transfer we aren't part of so its data edges aren't mistaken for a START
    int skipUntilStop()
    {
        while (true)
        {
            int bit = readBit();
            if (bit < 0)
                return bit;
        }
    }

    // master write, the satellite modules have no commands so the data is just ACKed and dropped
    int receiveData()
    {
        if (!sendAck())
            return TIMEOUT;

        while (true)
        {
            int value = readByte();
            if (value < 0)
                return value;
            if (!sendAck())
                return TIMEOUT;
        }
    }

    int sendData()
    {
        // stretch the clock while the packet is built, then ACK the address
        sclLow();
        uint8_t buf[MAX_READ_SIZE];
        uint8_t len = requestHandler(buf, MAX_READ_SIZE);
        sdaLow();
        // data setup time before the clock is let go, delayMicroseconds needs the SysTick interrupt
        for (volatile uint8_t i = 0; i < 50; i++)
            ;
        sclRelease();

        if (waitScl(true) < 0 || waitScl(false) < 0)
        {
            sdaRelease();
            return TIMEOUT;
        }

        // pad with 0xFF if the master asks for more than we have
        for (uint8_t i = 0;; i++)
        {
            int ack = writeByte(i < len ? buf[i] : 0xFF);
            if (ack != 0)
                return ack == 1 ? skipUntilStop() : ack;
        }
    }

    // runs from just after a START until the transfer ends, returns STOP or TIMEOUT
    int handleTransfer()
    {
        while (true)
        {
            int start = waitAfterStart();
            if (start < 0)
                return start;

            int address = readByte();
            int result;
            if (address < 0)
                result = address;
            else if ((address >> 1) != ownAddress)
                result = skipUntilStop();
            else if (address & 1)
                result = sendData();
            else
                result = receiveData();

            if (result != RESTART)
                return result;
        }
    }

    // follows the bus until a STOP, or until both lines sit high long enough that nothing is running
    bool waitForIdle()
    {
        uint32_t idle = 0;
        uint32_t prev = readBus();
        for (uint32_t i = 0; i < RESYNC_TIMEOUT_LOOPS; i++)
        {
            uint32_t now = readBus();
            bool bothHigh = now == busMask;
            if (bothHigh && prev == sclMask)
                return true; // SDA rose while SCL was high
            idle = bothHigh ? idle + 1 : 0;
            if (idle >= IDLE_LOOPS)
                return true;
            prev = now;
        }
        return false;
    }

    // SDA was already back high when the interrupt ran: either a spike on the idle bus, or the
    // interrupt ran so late that a transfer is under way. a transfer gives itself away by SCL
    // dropping within one clock period, returns false if the bus can't be trusted to be idle
    bool idleAfterSpike()
    {
        for (uint32_t i = 0; i < SPIKE_CHECK_LOOPS; i++)
        {
            uint32_t now = readBus();
            if (!(now & sclMask))
                return false;
            if (!(now & sdaMask))
                return true; // a real START arrived, handled below
        }
        return true;
    }

    void onSdaFalling()
    {
        // nothing may interrupt a transfer, a single missed clock edge shifts every bit after it
        uint32_t mstatus;
        asm volatile("csrrci %0, mstatus, 0x8" : "=r"(mstatus));

        do
        {
            if (synced && (readBus() & sdaMask))
                synced = idleAfterSpike();
            if (synced && !(readBus() & sdaMask))
                synced = handleTransfer() == STOP;
            if (!synced)
                synced = waitForIdle();

            // drop the edges we drove or followed above, a START that landed after the STOP
            // still shows as SDA low and is handled by the next pass
            EXTI->INTFR = sdaMask;
        } while (synced && !(readBus() & sdaMask));

        asm volatile("csrw mstatus, %0" : : "r"(mstatus));
    }

    void begin(uint8_t address, uint32_t sclPin, uint32_t sdaPin, RequestHandler onRequest)
    {
        ownAddress = address;
        requestHandler = onRequest;
        port = digitalPinToPort(sdaPin);
        sclMask = digitalPinToBitMask(sclPin);
        sdaMask = digitalPinToBitMask(sdaPin);
        busMask = sclMask | sdaMask;

        // latch both lines released before switching to output so the shared bus doesn't glitch
        pinMode(sclPin, INPUT);
        pinMode(sdaPin, INPUT);
        sclRelease();
        sdaRelease();
        pinMode(sclPin, OUTPUT_OD);
        sclRelease();

        // the main module may already be talking, so find the end of whatever is on the bus first
        synced = waitForIdle();

        attachInterrupt(sdaPin, GPIO_Mode_Out_OD, onSdaFalling, EXTI_Mode_Interrupt, EXTI_Trigger_Falling);
        sdaRelease();
    }
}
