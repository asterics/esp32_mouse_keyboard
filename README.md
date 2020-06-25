# ESP32 Mouse/Keyboard for BLE HID
ESP32 implementation for HID over GATT Keyboard and Mouse (Bluetooth Low Energy). Including serial API for external modules (compatible to Adafruit EZKey HID).

## Building the ESP32 firmware

Please follow the step-by-step instructions of Espressif's ESP-IDF manual to setup the build infrastructure:
[Setup Toolchain](https://esp-idf.readthedocs.io/en/latest/get-started/index.html#setup-toolchain)

With `idf.py build` or `make` you can build this project.

With `idf.py -p (PORT) flash` or `make flash` you can upload this build to an ESP32

With `idf.py -p (PORT) monitor` or `make monitor` you can see the debug output (please use this output if you open an issue) or trigger basic test commands (mouse movement or a keyboard key press) on
a connected target.
 
# Usage via Console or second UART

## Control via stdin (make monitor)

For basic mouse and keyboard testing, some Bluetooth HID reports can be triggered via the 
keyboard when the make monitor console is running (see Espressif IDF docs: https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/tools/idf-monitor.html ).


|Key|Function   |Description|
|---|-----------|-----------|
|a  |Mouse left |Move mouse left by 30px |
|s  |Mouse down |Move mouse down by 30px |
|d  |Mouse right|Move mouse right by 30px |
|w  |Mouse up   |Move mouse up by 30px |
|l  |Click left |Mouse click right |
|r  |Click right|Mouse click left  |
|q  |Type 'y' (US layout)   |just for testing keyboard reports|



## Control via 2nd UART

This interface is primarily used to control mouse / keyboard activities via an external microcontroller.


### Commands

Each command is started with a '$' character, followed by a command name (uppercase letters) and a variable number of parameters.
A command must be terminated by a '\n' character (LF, ASCII value 10)!

_Note:_ We do not test these on a regular basis!

|Command|Function|Parameters|Description|
|-------|--------|----------|-----------|
|$ID|Get ID|--|Prints out the ID of this module (firmware version number)|
|$GP|Get BLE pairings|--|Prints out all paired devices' MAC adress. The order is used for DP as well, starting with 0|
|$DP|Delete one pairing|number of pairing, given as ASCII-characer '0'-'9'|Deletes one pairing. The pairing number is determined by the command GP|
|$PM|Set pairing mode|'0' / '1'|Enables (1) or disables (0) discovery/advertising and terminates an exisiting connection if enabled|
|$NAME|Set BLE device name|name as ASCII string|Set the device name to the given name. Restart required.|

### HID input

We use a format which is compatible to the Adafruit EZKey module, and added a few backwards compatible features.

__Mouse:__

|Byte 0|Byte 1|Byte 2|Byte 3|Byte 4|Byte 5|Byte 6|Byte 7|Byte 8|
|------|------|------|------|------|------|------|------|------|
| 0xFD | don't care | 0x03 | button mask | X-axis | Y-axis | wheel | don't care | don't care |


__Keyboard:__

_Note:_ currently we only support the US keyboard layout.

|Byte 0|Byte 1|Byte 2|Byte 3|Byte 4|Byte 5|Byte 6|Byte 7|Byte 8|
|------|------|------|------|------|------|------|------|------|
| 0xFD | modifier mask | don't care | keycode 1 | keycode 2 | keycode 3 | keycode 4 | keycode 5 | keycode 6 |


## RAW HID input from sourcecode

If you want to change anything within the sourcecode, it is possible to send an HID report directly via:

```
hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
                  HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, mouse_report);
```

Use according `HID_RPT_ID_*`/`HID_*_IN_RPT_LEN`, depending on input type (mouse, keyboard).
The HID report data is located in array, passed as last parameter of `hid_dev_send_report`.

## Mouse & Keyboard input from sourcecode

Please use the functions provided by `esp_hidd_prf_api.c`.


## Credits and many thanks to:
- Paul Stoffregen for the implementation of the keyboard layouts for his Teensyduino project: www.pjrc.com
- Neil Kolban for his great contributions to the ESP32 SW (in particular the Bluetooth support): https://github.com/nkolban
- Chegewara for help and support
 
and to Espressif for providing the HID implementation within the esp-idf.


