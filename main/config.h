 
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MODULE_ID "ESP32BT_v0.1"

//TODO: change to external UART; currently set to serial port of monitor for debugging
#define EX_UART_NUM UART_NUM_2
#define CONSOLE_UART_NUM UART_NUM_0

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
