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

#include "esp_hidd_prf_api.h"
#include "hidd_le_prf_int.h"
#include "hid_dev.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

// HID keyboard input report length
///@note Set to 7, because padding byte is removed
#define HID_KEYBOARD_IN_RPT_LEN     7

// HID LED output report length
//#define HID_LED_OUT_RPT_LEN         1

// HID mouse input report length
#define HID_MOUSE_IN_RPT_LEN        5

// HID joystick input report length
#define HID_JOYSTICK_IN_RPT_LEN     11

// HID consumer control input report length
#define HID_CC_IN_RPT_LEN           2

esp_err_t esp_hidd_register_callbacks(esp_hidd_event_cb_t callbacks, uint8_t enablegamepad)
{
    esp_err_t hidd_status;

    if(callbacks != NULL) {
   	    hidd_le_env.hidd_cb = callbacks;
    } else {
        return ESP_FAIL;
    }

    if((hidd_status = hidd_register_cb(enablegamepad)) != ESP_OK) {
        return hidd_status;
    }

    esp_ble_gatts_app_register(BATTRAY_APP_ID);
    
    if((hidd_status = esp_ble_gatts_app_register(HIDD_APP_ID)) != ESP_OK) {
        return hidd_status;
    }
    
    ///@note It seems that the MTU has no effect on compatibility. The BLE stack works with all systems without setting the MTU to 23Bytes.
    /*hidd_status = esp_ble_gatt_set_local_mtu(23);
    if (hidd_status != ESP_OK){
        ESP_LOGE(HID_LE_PRF_TAG, "set local  MTU failed, error code = %x", hidd_status);
    }*/
   
    return hidd_status;
}

esp_err_t esp_hidd_profile_init(void)
{
	if (hidd_le_env.enabled) {
        ESP_LOGE(HID_LE_PRF_TAG, "HID device profile already initialized");
        return ESP_FAIL;
    }
    // Reset the hid device target environment
    memset(&hidd_le_env, 0, sizeof(hidd_le_env_t));
    hidd_le_env.enabled = true;
    return ESP_OK;
}

esp_err_t esp_hidd_profile_deinit(void)
{
    uint16_t hidd_svc_hdl = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC];
    if (!hidd_le_env.enabled) {
        ESP_LOGE(HID_LE_PRF_TAG, "HID device profile already deinitialized");
        return ESP_OK;
    }

    if(hidd_svc_hdl != 0) {
		esp_ble_gatts_stop_service(hidd_svc_hdl);
		esp_ble_gatts_delete_service(hidd_svc_hdl);
    } else {
		return ESP_FAIL;
	}
    
    /* register the HID device profile to the BTA_GATTS module*/
    esp_ble_gatts_app_unregister(hidd_le_env.gatt_if);
    
    //set hidd enabled to false
    //THX @Lars-Thestorf
    //see issue #44
    hidd_le_env.enabled = false;

    return ESP_OK;
}

uint16_t esp_hidd_get_version(void)
{
	return HIDD_VERSION;
}

void esp_hidd_send_consumer_value(uint16_t conn_id, uint8_t key_cmd, bool key_pressed)
{
    uint8_t buffer[HID_CC_IN_RPT_LEN] = {0, 0};
    if (key_pressed) {
        ESP_LOGD(HID_LE_PRF_TAG, "hid_consumer_build_report");
        hid_consumer_build_report(buffer, key_cmd);
    }
    ESP_LOGD(HID_LE_PRF_TAG, "buffer[0] = %x, buffer[1] = %x", buffer[0], buffer[1]);
    hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
                        HID_RPT_ID_CC_IN, HID_REPORT_TYPE_INPUT, HID_CC_IN_RPT_LEN, buffer);
    return;
}

void esp_hidd_send_keyboard_value(uint16_t conn_id, key_mask_t special_key_mask, uint8_t *keyboard_cmd, uint8_t num_key)
{
    //if (num_key > HID_KEYBOARD_IN_RPT_LEN - 2) {
    ///@note Here without padding byte as well.
    if (num_key > HID_KEYBOARD_IN_RPT_LEN - 1) {
        ESP_LOGE(HID_LE_PRF_TAG, "%s(), the number key should not be more than %d", __func__, HID_KEYBOARD_IN_RPT_LEN);
        return;
    }
   
    uint8_t buffer[HID_KEYBOARD_IN_RPT_LEN] = {0};
   
    buffer[0] = special_key_mask;
    
    for (int i = 0; i < num_key; i++) {
        ///@note start @1 instead of @2 with padding byte.
        buffer[i+1] = keyboard_cmd[i];
    }

    //ESP_LOGD(HID_LE_PRF_TAG, "the key value = %d,%d,%d, %d, %d, %d,%d, %d", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
    ///@note here is the padding byte removed as well.
    ESP_LOGD(HID_LE_PRF_TAG, "the key value = %d,%d,%d, %d, %d, %d,%d", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
    hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
                        HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT, HID_KEYBOARD_IN_RPT_LEN, buffer);
    return;
}

void esp_hidd_send_mouse_value(uint16_t conn_id, uint8_t mouse_button, int8_t mickeys_x, int8_t mickeys_y, int8_t wheel)
{
    uint8_t buffer[HID_MOUSE_IN_RPT_LEN];
    
    buffer[0] = mouse_button;   // Buttons
    buffer[1] = mickeys_x;           // X
    buffer[2] = mickeys_y;           // Y
    buffer[3] = wheel;           // Wheel
    buffer[4] = 0;           // AC Pan

    hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
                        HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, buffer);
    return;
}

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
void esp_hidd_send_joy_value(uint16_t conn_id, int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons)
{
  uint8_t data[HID_JOYSTICK_IN_RPT_LEN] = {0};
  
  //build axis into array
  data[0] = x;
  data[1] = y;
  data[2] = z;
  data[3] = rz;
  data[4] = rx;
  data[5] = ry;
  
  //add hat & buttons
  data[6] = hat;
  data[7] = (uint8_t)(buttons & 0xFF);
  data[8] = (uint8_t)((buttons>>8) & 0xFF);
  data[9] = (uint8_t)((buttons>>16) & 0xFF);
  data[10] = (uint8_t)((buttons>>24) & 0xFF);
  
  //use other joystick function to send data
  esp_hidd_send_joy_report(conn_id, data);
}

/**
 *
 * @brief           Send a Joystick report, use a byte array
 *
 * @param           conn_id HID over GATT connection ID to be used.
 * @param           report  Pointer to a 11 Byte sized array which contains the full joystick/gamepad report
 * @warning         This function reads 11 Bytes and sends them without checks.
 */
void esp_hidd_send_joy_report(uint16_t conn_id, uint8_t *report)
{
  hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
    HID_RPT_ID_JOY_IN, HID_REPORT_TYPE_INPUT, HID_JOYSTICK_IN_RPT_LEN, report);
}

#endif
