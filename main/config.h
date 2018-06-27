 
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MODULE_ID "ESP32miniBT_v0.1"
#define GATTS_TAG "FLIPMOUSE"
#define MAX_BT_DEVICENAME_LENGTH 40

// serial port of monitor and for debugging
#define CONSOLE_UART_NUM UART_NUM_0

// serial port for connection to other controllers
#define EX_UART_NUM UART_NUM_2
#define EX_SERIAL_TXPIN      (GPIO_NUM_17)
#define EX_SERIAL_RXPIN      (GPIO_NUM_16)

// indicator LED
#define INDICATOR_LED_PIN    (GPIO_NUM_5)

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
    char bt_device_name[MAX_BT_DEVICENAME_LENGTH];
    uint8_t locale;
} config_data_t;


#endif
