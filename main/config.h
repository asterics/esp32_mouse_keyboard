
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MODULE_ID "ESP32miniBT_v0.3"

#if CONFIG_MODULE_FLIPMOUSE
    #define GATTS_TAG "FLipMouse"
#else
    #if CONFIG_MODULE_FABI
        #define GATTS_TAG "FABI"
    #else
        #define GATTS_TAG "esp32_mouse_keyboard"
    #endif
#endif

#define MAX_BT_DEVICENAME_LENGTH 40

// serial port of monitor and for debugging (not in KConfig, won't be changed normally)
#define CONSOLE_UART_NUM 	 UART_NUM_0

// serial port for connection to other controllers
#define EX_UART_NUM     	 CONFIG_MODULE_UART_NR
#define EX_SERIAL_TXPIN      CONFIG_MODULE_TX_PIN
#define EX_SERIAL_RXPIN      CONFIG_MODULE_RX_PIN

// indicator LED
#define INDICATOR_LED_PIN    CONFIG_MODULE_LED_PIN

typedef struct config_data {
    char bt_device_name[MAX_BT_DEVICENAME_LENGTH];
    uint8_t locale;
} config_data_t;


#endif
