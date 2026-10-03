#include "HID.h"
#include <Adafruit_TinyUSB.h>

namespace ModpadHID
{
    enum ReportId : uint8_t
    {
        RID_KEYBOARD = 1,
        RID_CONSUMER,
    };

    static uint8_t const desc_hid_report[] = {
        TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD)),
        TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(RID_CONSUMER)),
    };
    static Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report));

    void setup()
    {
        TinyUSBDevice.setProductDescriptor("ModPad");
        usb_hid.begin();
    }

    void tapKey(uint8_t key, uint8_t modifiers)
    {
        uint8_t keys[6] = {key};
        usb_hid.keyboardReport(RID_KEYBOARD, modifiers, keys);
        delay(10);
        usb_hid.keyboardRelease(RID_KEYBOARD);
    }

    void tapConsumer(uint16_t usage)
    {
        usb_hid.sendReport16(RID_CONSUMER, usage);
        delay(10);
        usb_hid.sendReport16(RID_CONSUMER, 0);
    }
}
