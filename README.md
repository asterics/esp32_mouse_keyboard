# ESP32 Mouse/Keyboard for BLE HID
ESP32 implementation for HID over GATT Keyboard and Mouse (Bluetooth Low Energy). Including serial API for external modules (similar to Adafruit EZKey HID).

A great thank you to Paul Stoffregen for the implementation of the keyboard layouts for his Teensyduino project:
www.pjrc.com
and to Espressif for providing the HID implementation within the esp-idf.


# Control via stdin (make monitor)

For basic mouse and keyboard testing, this project includes some commands which
can be triggered via the make monitor console.
If there is no command found via this parser, the data is forwarded to the 
parser for the second UART (see following chapter).

Note: To be sure there is no garbage sent to the second parser, please flush
the buffer via the Enter key. Then type the command and send it via Enter.

|Key|Function   |Description|
|---|-----------|-----------|
|a  |Mouse left |Move mouse left by 30px |
|s  |Mouse down |Move mouse down by 30px |
|d  |Mouse right|Move mouse right by 30px |
|w  |Mouse up   |Move mouse up by 30px |
|l  |Click left |Mouse click right |
|r  |Click right|Mouse click left  |
|Q  |Type 'y'   |Type the key y with a 1s delay (avoiding endless loops)|

Any none listed keys are forwarded to the second parser.


# Control via 2nd UART

This interface is primarily used to control this module by an external microcontroller (in our case
either a AVR Mega32U4 in the FABI device or a TeensyLC in the FLipMouse device).
Each command is started with one or two upper letters and a variable number of parameter bytes.
A command must be terminated by a '\n' character!

TBD: currently the UART is not configured, this parser is just used by the stdin console. After testing all
commands, it will be changed and the configured GPIO pins are stated here.

|Command|Function|Parameters|Description|
|-------|--------|----------|-----------|
|ID|Get ID|--|Prints out the ID of this module, used to determine version or attached module type|
|GP|Get BLE pairings|--|Prints out all paired devices' MAC adress. The order is used for DP as well, starting with 0|
|DP|Delete one pairing|number of pairing, either as ASCII '0'-'9' or 0x00-0x09|Deletes one pairing. The pairing number is determined by the command GP|
|PM|Set pairing mode|'0'/'1' or 0x00/0x01|Enables (1) or disables (0) discovery/advertising and terminates an exisiting connection if enabled|
|ID_FABI|Set BLE device name|--|Set the device name to "FABI" and store permanently. Restarting required.|
|ID_FLIPMOUSE|Set BLE device name|--|Set the device name to "FLipMouse" and store permanently. Restarting required.|
|M|Mouse control|4 Bytes|Issue a mouse HID report, parameter description is below|
|KR|Keyboard, release all|--|Releases all keys & modifiers and sends a HID report|
|KD|Keyboard, key down|1Byte keycode|Adds this keycode to the 6 available key slots and sends a HID report.|
|KU|Keyboard, key up|1Byte keycode|Removes this keycode from the 6 available key slots and sends a HID report.|
|KL|Keyboard, set layout/locale|1Byte|Set the keyboard layout to the given locale number (see below), stored permanently. Restarting required.|
|KW|Keyboard write text|n Bytes|Write a text via the keyboard. Supported are ASCII as well as UTF8 character streams.|
|KC|Keyboard get keycode for character|2 Bytes|Get a corresponding keycode for the given character. 2 Bytes are parsed for UTF8, if there is only a ASCII character, use the first sent byte only.|


## Mouse command

|Command byte|Param1|Param2|Param3|Param4|
|------------|------|------|------|------|
|'M'/0x4D    |uint8 buttons|int8 X|int8 Y|int8 scroll wheel|

Mouse buttons are assigned as following:
(1<<0): Left mouse button
(1<<1): Right mouse button
(1<<2): Middle mouse button

Due to the button masks, more than one button might be sent in one command.
Releasing the mouse buttons is done via setting the corresponding mask to zero and
send the mouse command again.


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
