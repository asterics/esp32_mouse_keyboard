
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MODULE_ID "ESP32miniBT_v0.3.7"

#if CONFIG_USE_AS_FLIPMOUSE_FABI
  //this will be overwritten by FABI/FLipMouse/FLipPad firmware to correct
  //name, this is the BT device name for the first start
  #define GATTS_TAG "AsTeRICS Foundation Module"
//non FM/FP/FABI boards
#else 
	#define EX_UART_NUM     	 CONFIG_MODULE_UART_NR
	#define GATTS_TAG "esp32_mouse_keyboard"
	// indicator LED
	#define INDICATOR_LED_PIN    CONFIG_MODULE_LED_PIN
	#define EX_SERIAL_TXPIN      CONFIG_MODULE_TX_PIN
	#define EX_SERIAL_RXPIN      CONFIG_MODULE_RX_PIN
#endif


#define MAX_BT_DEVICENAME_LENGTH 40

// serial port of monitor and for debugging (not in KConfig, won't be changed normally)
#define CONSOLE_UART_NUM 	 UART_NUM_0

typedef struct config_data {
    char bt_device_name[MAX_BT_DEVICENAME_LENGTH];
    uint8_t locale;
    uint8_t joystick_active;
} config_data_t;


#endif
