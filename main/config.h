 
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MODULE_ID "ESP32BT_v0.1"

//TODO: change to external UART; currently set to serial port of monitor for debugging
#define EX_UART_NUM UART_NUM_0

// from USB HID Specification 1.1, Appendix B.2
const uint8_t hid_descriptor[] = {
    /**++++ KEYBOARD ++++**/
    
    0x05, 0x01,                    // Usage Page (Generic Desktop)
    0x09, 0x06,                    // Usage (Keyboard)
    0xa1, 0x01,                    // Collection (Application)
    //TODO: is this right here?!?
    0x85, 0x01,                    //     REPORT_ID (1)
    // Modifier byte

    0x75, 0x01,                    //   Report Size (1)
    0x95, 0x08,                    //   Report Count (8)
    0x05, 0x07,                    //   Usage Page (Key codes)
    0x19, 0xe0,                    //   Usage Minimum (Keyboard LeftControl)
    0x29, 0xe7,                    //   Usage Maxium (Keyboard Right GUI)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x25, 0x01,                    //   Logical Maximum (1)
    0x81, 0x02,                    //   Input (Data, Variable, Absolute)

    // Reserved byte

    0x75, 0x01,                    //   Report Size (1)
    0x95, 0x08,                    //   Report Count (8)
    0x81, 0x03,                    //   Input (Constant, Variable, Absolute)

    // LED report + padding

    0x95, 0x05,                    //   Report Count (5)
    0x75, 0x01,                    //   Report Size (1)
    0x05, 0x08,                    //   Usage Page (LEDs)
    0x19, 0x01,                    //   Usage Minimum (Num Lock)
    0x29, 0x05,                    //   Usage Maxium (Kana)
    0x91, 0x02,                    //   Output (Data, Variable, Absolute)

    0x95, 0x01,                    //   Report Count (1)
    0x75, 0x03,                    //   Report Size (3)
    0x91, 0x03,                    //   Output (Constant, Variable, Absolute)

    // Keycodes

    0x95, 0x06,                    //   Report Count (6)
    0x75, 0x08,                    //   Report Size (8)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x25, 0xff,                    //   Logical Maximum (1)
    0x05, 0x07,                    //   Usage Page (Key codes)
    0x19, 0x00,                    //   Usage Minimum (Reserved (no event indicated))
    0x29, 0xff,                    //   Usage Maxium (Reserved)
    0x81, 0x00,                    //   Input (Data, Array)

    0xc0,                          // End collection  
 
    /**++++ MOUSE ++++**/
    
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)

    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    //TODO: is this right here?!?
    0x85, 0x02,                    //     REPORT_ID (2)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)

    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)

    0xc0,                          //   END_COLLECTION
    0xc0,                           // END_COLLECTION
    
    /**++++ JOYSTICK ++++*/
    //TODO: add joystick to GATT table, otherwise it does not work
    
    0x05, 0x01,                     // Usage Page (Generic Desktop)
    0x09, 0x04,                     // Usage (Joystick)
    0xA1, 0x01,                     // Collection (Application)
    //TODO: is this right here?!?
    0x85, 0x03,                    //     REPORT_ID (3)
    
	0x15, 0x00,			// Logical Minimum (0)
	0x25, 0x01,			// Logical Maximum (1)
	0x75, 0x01,			// Report Size (1)
	0x95, 0x20,			// Report Count (32)
	0x05, 0x09,			// Usage Page (Button)
	0x19, 0x01,			// Usage Minimum (Button #1)
	0x29, 0x20,			// Usage Maximum (Button #32)
	0x81, 0x02,			// Input (variable,absolute)
	0x15, 0x00,			// Logical Minimum (0)
	0x25, 0x07,			// Logical Maximum (7)
	0x35, 0x00,			// Physical Minimum (0)
	0x46, 0x3B, 0x01,		// Physical Maximum (315)
	0x75, 0x04,			// Report Size (4)
	0x95, 0x01,			// Report Count (1)
	0x65, 0x14,			// Unit (20)
        0x05, 0x01,                     // Usage Page (Generic Desktop)
	0x09, 0x39,			// Usage (Hat switch)
	0x81, 0x42,			// Input (variable,absolute,null_state)
        0x05, 0x01,                     // Usage Page (Generic Desktop)
	0x09, 0x01,			// Usage (Pointer)
        0xA1, 0x00,                     // Collection ()
	0x15, 0x00,			//   Logical Minimum (0)
	0x26, 0xFF, 0x03,		//   Logical Maximum (1023)
	0x75, 0x0A,			//   Report Size (10)
	0x95, 0x04,			//   Report Count (4)
	0x09, 0x30,			//   Usage (X)
	0x09, 0x31,			//   Usage (Y)
	0x09, 0x32,			//   Usage (Z)
	0x09, 0x35,			//   Usage (Rz)
	0x81, 0x02,			//   Input (variable,absolute)
        0xC0,                           // End Collection
	0x15, 0x00,			// Logical Minimum (0)
	0x26, 0xFF, 0x03,		// Logical Maximum (1023)
	0x75, 0x0A,			// Report Size (10)
	0x95, 0x02,			// Report Count (2)
	0x09, 0x36,			// Usage (Slider)
	0x09, 0x36,			// Usage (Slider)
	0x81, 0x02,			// Input (variable,absolute)
    0xC0                // End Collection
};



typedef struct joystick_data {
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
    uint16_t Z_rotate;
    uint16_t slider_left;
    uint16_t slider_right;
    uint16_t buttons1;
    uint16_t buttons2;
    int16_t hat;
} joystick_data_t;

typedef struct config_data {
    uint8_t bt_device_name_index;
    uint8_t locale;
} config_data_t;


#endif
