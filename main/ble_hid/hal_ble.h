/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * Copyright 2019 Benjamin Aigner <beni@asterics-foundation.org>
 */

/** @file
 * @brief This file is a wrapper for the BLE-HID example of Espressif.
*/
#ifndef _HAL_BLE_H_
#define _HAL_BLE_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include <keyboard.h>

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "hid_dev.h"
#include "nvs_flash.h"

/** @brief Stack size for BLE task */
#define TASK_BLE_STACKSIZE 2048

/** @brief Queue for sending mouse/keyboard/joystick reports
 * @see hid_cmd_t */
extern QueueHandle_t hid_ble;

/** @brief Activate/deactivate pairing mode
 * @param enable If set to != 0, pairing will be enabled. Disabled if == 0
 * @return ESP_OK on success, ESP_FAIL otherwise*/
esp_err_t halBLESetPairing(uint8_t enable);

/** @brief Get connection status
 * @return 0 if not connected, != 0 if connected */
uint8_t halBLEIsConnected(void);

/** @brief En- or Disable BLE interface.
 * 
 * This method is used to enable or disable the BLE interface. Currently, the ESP32
 * cannot use WiFi and BLE simultaneously. Therefore, when enabling wifi, it is
 * necessary to disable BLE prior calling taskWebGUIEnDisable.
 * 
 * @note Calling this method prior to initializing BLE via halBLEInit will
 * result in an error!
 * @return ESP_OK on success, ESP_FAIL otherwise
 * @param onoff If != 0, switch on BLE, switch off if 0.
 * */
esp_err_t halBLEEnDisable(int onoff);


/** @brief Reset the BLE data
 * 
 * Used for slot/config switchers.
 * It resets the keycode array and sets all HID reports to 0
 * (release all keys, avoiding sticky keys on a config change) 
 * @param exceptDevice if you want to reset only a part of the devices, set flags
 * accordingly:
 * 
 * (1<<0) excepts keyboard
 * (1<<1) excepts joystick
 * (1<<2) excepts mouse
 * If nothing is set (exceptDevice = 0) all are reset
 * */
void halBLEReset(uint8_t exceptDevice);

/** @brief Main init function to start HID interface (C interface)
 * @see hid_ble */
esp_err_t halBLEInit(uint8_t enableKeyboard, uint8_t enableMouse, uint8_t enableJoystick, char* deviceName);


/** @brief One HID command */
typedef struct hid_cmd hid_cmd_t;

/** @brief  One HID command */
struct hid_cmd {
  /** Explanation of bytefields:
   * see https://github.com/benjaminaigner/usb_bridge/blob/master/example/src/parser.c#L119
   */
  uint8_t cmd[3];
};


#endif /* _HAL_BLE_H_ */
