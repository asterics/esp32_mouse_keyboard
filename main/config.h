
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MODULE_ID "ESP32miniBT_v0.3.2"

#if CONFIG_USE_AS_FLIPMOUSE_FABI
	#define EX_SERIAL_RXPIN      GPIO_NUM_17
	#define EX_SERIAL_TXPIN      GPIO_NUM_16
	
	#if CONFIG_MODULE_FLIPMOUSE
		#define GATTS_TAG "FLipMouse"
		#define EX_UART_NUM UART_NUM_2
		#define INDICATOR_LED_PIN GPIO_NUM5
	#endif
	#if CONFIG_MODULE_FABI
		#define GATTS_TAG "FABI"
		#define EX_UART_NUM UART_NUM_2
		#define INDICATOR_LED_PIN GPIO_NUM_5
	#endif

	#if CONFIG_MODULE_NANO
		//will be overwritten anyway by RP2040 FW
		#define GATTS_TAG "esp32_nano"
		#define EX_UART_NUM UART_NUM_0
		//GPIO26 is blue; 25 is green and 27 red
		#define INDICATOR_LED_PIN GPIO_NUM_26
	#endif

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
} config_data_t;


#endif
