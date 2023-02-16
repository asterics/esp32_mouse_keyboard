// Copyright 2017-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __ESP_HIDD_API_H__
#define __ESP_HIDD_API_H__

#include "esp_bt_defs.h"
#include "esp_gatt_defs.h"
#include "esp_gatt_common_api.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ESP_HIDD_EVENT_REG_FINISH = 0,                     
    ESP_BAT_EVENT_REG,
    ESP_HIDD_EVENT_DEINIT_FINISH,                       
    ESP_HIDD_EVENT_BLE_CONNECT,                         
    ESP_HIDD_EVENT_BLE_DISCONNECT,
    ESP_HIDD_EVENT_BLE_CONGEST,
} esp_hidd_cb_event_t;

/// HID config status
typedef enum {
    ESP_HIDD_STA_CONN_SUCCESS = 0x00,
    ESP_HIDD_STA_CONN_FAIL    = 0x01,
} esp_hidd_sta_conn_state_t;

/// HID init status
typedef enum {
    ESP_HIDD_INIT_OK = 0,
    ESP_HIDD_INIT_FAILED = 1,
} esp_hidd_init_state_t;

/// HID deinit status
typedef enum {
    ESP_HIDD_DEINIT_OK = 0,
    ESP_HIDD_DEINIT_FAILED = 0,
} esp_hidd_deinit_state_t;

#define LEFT_CONTROL_KEY_MASK        (1 << 0)
#define LEFT_SHIFT_KEY_MASK          (1 << 1)
#define LEFT_ALT_KEY_MASK            (1 << 2)
#define LEFT_GUI_KEY_MASK            (1 << 3)
#define RIGHT_CONTROL_KEY_MASK       (1 << 4)
#define RIGHT_SHIFT_KEY_MASK         (1 << 5)
#define RIGHT_ALT_KEY_MASK           (1 << 6)
#define RIGHT_GUI_KEY_MASK           (1 << 7)

typedef uint8_t key_mask_t;
/**
 * @brief HIDD callback parameters union 
 */
typedef union {
    /**
	 * @brief ESP_HIDD_EVENT_INIT_FINISH
	 */
    struct hidd_init_finish_evt_param {
        esp_hidd_init_state_t state;				/*!< Initial status */
        esp_gatt_if_t gatts_if;
    } init_finish;							      /*!< HID callback param of ESP_HIDD_EVENT_INIT_FINISH */

    /**
	 * @brief ESP_HIDD_EVENT_DEINIT_FINISH
	 */
    struct hidd_deinit_finish_evt_param {
        esp_hidd_deinit_state_t state;				/*!< De-initial status */
    } deinit_finish;								/*!< HID callback param of ESP_HIDD_EVENT_DEINIT_FINISH */

    /**
     * @brief ESP_HIDD_EVENT_CONNECT
	 */
    struct hidd_connect_evt_param {
        uint16_t conn_id;
        esp_bd_addr_t remote_bda;                   /*!< HID Remote bluetooth connection index */
    } connect;									    /*!< HID callback param of ESP_HIDD_EVENT_CONNECT */

    /**
     * @brief ESP_HIDD_EVENT_CONGEST
	 */
    struct hidd_congest_evt_param {
        uint16_t conn_id;
        bool congested;                   /*!< congested? */
    } congest;									    /*!< HID callback param of ESP_HIDD_EVENT_CONGEST */

    /**
     * @brief ESP_HIDD_EVENT_DISCONNECT
	 */
    struct hidd_disconnect_evt_param {
        esp_bd_addr_t remote_bda;                   /*!< HID Remote bluetooth device address */
    } disconnect;									/*!< HID callback param of ESP_HIDD_EVENT_DISCONNECT */

    /**
     * @brief ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT
	 */
    struct hidd_vendor_write_evt_param {
        uint16_t conn_id;                           /*!< HID connection index */
        uint16_t report_id;                         /*!< HID report index */
        uint16_t length;                            /*!< data length */
        uint8_t  *data;                             /*!< The pointer to the data */
    } vendor_write;									/*!< HID callback param of ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT */

} esp_hidd_cb_param_t;


/**
 * @brief HID device event callback function type
 * @param event : Event type
 * @param param : Point to callback parameter, currently is union type
 */
typedef void (*esp_hidd_event_cb_t) (esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);



/**
 *
 * @brief           This function is called to receive hid device callback event
 *
 * @param[in]    callbacks: callback functions
 * @param[in]    enablegamepad: If != 0, the gamepad interface is enabled (necessary for iOS compat.)
 *
 * @return         ESP_OK - success, other - failed
 *
 */
esp_err_t esp_hidd_register_callbacks(esp_hidd_event_cb_t callbacks, uint8_t enablegamepad);

/**
 *
 * @brief           This function is called to initialize hid device profile
 *
 * @return          ESP_OK - success, other - failed
 *
 */
esp_err_t esp_hidd_profile_init(void);

/**
 *
 * @brief           This function is called to de-initialize hid device profile
 *
 * @return          ESP_OK - success, other - failed
 *
 */
esp_err_t esp_hidd_profile_deinit(void);

/**
 *
 * @brief           Get hidd profile version
 * 
 * @return          Most 8bit significant is Great version, Least 8bit is Sub version
 *
 */
uint16_t esp_hidd_get_version(void);

/**
 *
 * @brief           Send consumer keys.
 *
 * @param           conn_id HID over GATT connection ID to be used.
 * @param           key_cmd Type of consumer key, use defines like HID_CONSUMER_xx (e.g. HID_CONSUMER_MUTE)
 * @param           key_pressed True / False if key should be pressed or not.
 *
 */
void esp_hidd_send_consumer_value(uint16_t conn_id, uint8_t key_cmd, bool key_pressed);

/**
 *
 * @brief           Send a keyboard report.
 *
 * @param           conn_id HID over GATT connection ID to be used.
 * @param           special_key_mask  All special keys (Alt / Shift / CTRL) in one byte
 * @param           keyboard_cmd  Array of keycodes to be sent
 * @param           num_key Count of keycodes in keyboard_cmd which should be sent.
 *
 */
void esp_hidd_send_keyboard_value(uint16_t conn_id, key_mask_t special_key_mask, uint8_t *keyboard_cmd, uint8_t num_key);

/**
 *
 * @brief           Send a Mouse report.
 *
 * @param           conn_id HID over GATT connection ID to be used.
 * @param           mouse_button  Mouse button values, 1 is pressed, 0 is released. bit 0: left, bit 1: right, bit 2: middle button
 * @param           mickeys_x  relative X axis movement
 * @param           mickeys_y  relative Y axis movement
 * @param           wheel  relative mouse wheel movement
 */
void esp_hidd_send_mouse_value(uint16_t conn_id, uint8_t mouse_button, int8_t mickeys_x, int8_t mickeys_y, int8_t wheel);


#if CONFIG_MODULE_USEJOYSTICK
/**
 *
 * @brief           Send a Joystick report, set individual axis
 *
 * @param           conn_id HID over GATT connection ID to be used.
 * @param           x,y,z,rz,rx,ry  Individual gamepad axis
 * @param           hat Hat switch status. Send 0 for rest/middleposition; 1-8 to for a direction.
 * @param           buttons Button bitmap, button 0 is bit 0 and so on.
 */
void esp_hidd_send_joy_value(uint16_t conn_id, int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons);

/**
 *
 * @brief           Send a Joystick report, use a byte array
 *
 * @param           conn_id HID over GATT connection ID to be used.
 * @param           report  Pointer to a 11 Byte sized array which contains the full joystick/gamepad report
 * @warning         This function reads 11 Bytes and sends them without checks.
 */
void esp_hidd_send_joy_report(uint16_t conn_id, uint8_t *report);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __ESP_HIDD_API_H__ */

