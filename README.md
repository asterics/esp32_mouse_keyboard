# ESP32 Mouse/Keyboard for BLE HID
ESP32 implementation for HID over GATT Keyboard and Mouse (Bluetooth Low Energy). Including serial API for external modules (similar to Adafruit EZKey HID).

### Credits and many thanks to:
- Paul Stoffregen for the implementation of the keyboard layouts for his Teensyduino project: www.pjrc.com
- Neil Kolban for his great contributions to the ESP32 SW (in particular the Bluetooth support): https://github.com/nkolban
- Chegewara for help and support
 
and to Espressif for providing the HID implementation within the esp-idf.


# Control via stdin (make monitor)

For basic mouse and keyboard testing, some Bluettooh HID reports can be triggered via the 
keyboard when the make monitor console is running (see Espressiv IDF: https://github.com/espressif/esp-idf).


|Key|Function   |Description|
|---|-----------|-----------|
|a  |Mouse left |Move mouse left by 30px |
|s  |Mouse down |Move mouse down by 30px |
|d  |Mouse right|Move mouse right by 30px |
|w  |Mouse up   |Move mouse up by 30px |
|l  |Click left |Mouse click right |
|r  |Click right|Mouse click left  |
|q  |Type 'y'   |just for testing keyboard reports|



# Control via 2nd UART

This interface is primarily used to control mouse / keyboard activities via an external microcontroller.
Each command is started with a '$' character, followed by a command name (uppercase letters) and a variable number of parameters.
A command must be terminated by a '\n' character (LF, ASCII value 10)!


|Command|Function|Parameters|Description|
|-------|--------|----------|-----------|
|ID|Get ID|--|Prints out the ID of this module (firmware version number)|
|GP|Get BLE pairings|--|Prints out all paired devices' MAC adress. The order is used for DP as well, starting with 0|
|DP|Delete one pairing|number of pairing, given as ASCII-characer '0'-'9'|Deletes one pairing. The pairing number is determined by the command GP|
|PM|Set pairing mode|'0' / '1'|Enables (1) or disables (0) discovery/advertising and terminates an exisiting connection if enabled|
|NAME|Set BLE device name|name as ASCII string|Set the device name to the given name. Restart required.|
|M|Mouse control|4 ASCII-integer values (seperated by space or comma)|Issue a mouse HID report, parameter description is below|
|KA|Keyboard, release all|--|Releases all keys & modifiers and sends a HID report|
|KH|Keyboard, key hold|keycode as ASCII integer value|Adds this keycode to the 6 available key slots and sends a HID report.|
|KR|Keyboard, key release|keycode as ASCII integer value|Removes this keycode from the 6 available key slots and sends a HID report.|
|KL|Keyboard, set layout/locale|locale code as ASCII integer value|Set the keyboard layout to the given locale number (see below), stored permanently. Restarting required.|
|KW|Keyboard write text|n Bytes|Write a text via the keyboard. Supported are ASCII as well as UTF8 character streams.|
|KC|Keyboard get keycode for character|2 Bytes|Get a corresponding keycode for the given character. 2 Bytes are parsed for UTF8, if there is only a ASCII character, use the first sent byte only.|


## Mouse command

|Command byte|Param1|Param2|Param3|Param4|
|------------|------|------|------|------|
|'M' |uint8 buttons|int8 X|int8 Y|int8 scroll wheel|

Mouse buttons are encoded as bit values in the parameter 'buttons' as follows:
(1<<0): Left mouse button
(1<<1): Right mouse button
(1<<2): Middle mouse button

Example: "M 3 10 -10 0"

Due to this encoding in a bitmask, more than one button might be sent in one command.
Releasing the mouse buttons is done via setting the corresponding bit postition to zero and
sending the mouse command again.


## Keyboard layouts/locales

|Number|Layout|
|------|------|
|0x00/0| US English (no Unicode)|
|0x01/1| US International|
|0x02/2| German|
|0x03/3| German Mac|
|0x04/4| Canadian (French)|
|0x05/5| Canadian (Multilingual)|
|0x06/6| United Kingdom|
|0x07/7| Finnish|
|0x08/8| French|
|0x09/9| Danish|
|0x0A/10| Norwegian|
|0x0B/11| Swedish|
|0x0C/12| Spanish|
|0x0D/13| Portuguese|
|0x0E/14| Italian|
|0x0F/15| Portuguese (Brazilian)|
|0x10/16| Belgian (French)|
|0x11/17| German (Switzerland)|
|0x12/18| French (Switzerland)|
|0x13/19| Spanish (Latin America)|
|0x14/20| Irish|
|0x15/21| Icelandic|
|0x16/22| Turkish|
|0x17/23| Czech|
|0x18/24| Serbian (Latin only)|

Setting a keyboard locale is done with the "KL" command. Changes taking effect after a restart of the ESP32. This is necessary for initializing the HID country code accordingly.
