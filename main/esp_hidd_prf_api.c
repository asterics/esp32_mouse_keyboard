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

// NOTICE:
// Copyright 2020 - Benjamin Aigner (aignerb@technikum-wien.at / beni@asterics-foundation.org)
//   for:
// Changes on rate limiting mouse reports

#include "esp_hidd_prf_api.h"
#include "hidd_le_prf_int.h"
#include "hid_dev.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// HID keyboard input report length
#define HID_KEYBOARD_IN_RPT_LEN     8

// HID LED output report length
#define HID_LED_OUT_RPT_LEN         1

// HID mouse input report length
#define HID_MOUSE_IN_RPT_LEN        5

// HID consumer control input report length
#define HID_CC_IN_RPT_LEN           2

//// This variables are used, if the rate limiter is enabled.
////  we need additional variables here, namely:
////  * Store a timestamp for last transmitted mouse (or joystick in future) report
////  * Store a timer handle for accumulating data
////  * If enabled, store accumulated relative movements.
#if CONFIG_MODULE_USERATELIMITER
	/** @brief The timestamp of the last transmitted HID report 
	 * @note This handle is used in discard mode only -> disable accumulate in KConfig.
	 */
	int64_t lastTimeHIDCommandSent;
	/** @brief Accumulated mouse values for relative axis X/Y/Wheel */
	int16_t accumValues[3];
	/** @brief Absolute mouse button value currently set at GATT client (BLE central device aka HID host) */
	uint8_t buttonstate;
	/** @brief Absolute mouse button value currently set via the esp_hidd_send_mouse_value. Not currently sent to the host  
	 * @note This variable is only used in accumulate mode, to reflect any mousebutton updates which should be sent on next timer occurance.*/
	uint8_t newbuttonstate;
	/** @brief Default time interval between sending the limited reports */
	uint32_t timeIntervalHIDCommand = CONFIG_MODULE_RATELIMITERDEFAULT;
	/** @brief High-res timer handle for periodic sending of HID reports
	 * @note This handle is used in accumulate mode only -> enable accumulate in KConfig.
	 */
	esp_timer_handle_t intervalTimer;
	/** @brief Currently used connection ID, used for the rate limiter callback */
	uint16_t currentconnid;
	/** @brief Semaphore for synchronizing data between esp_hidd_send_mouse_value and rate_limier_callback */
	SemaphoreHandle_t rate_limiter_sem = NULL;
#endif /* CONFIG_MODULE_USERATELIMITER */


#if CONFIG_MODULE_RATELIMITERACCUMULATE
/** @brief Rate limiter timer CB, send accumulated relative values
 * 
 * This function is called if the timer expires and the accumulate mode is enabled.
 * If the CB is called, any remaining relative data is sent to the HID host.
 * 
 * @note MODULE_RATELIMITERACCUMULATE in KConfig must be enabled, otherwise
 * The mouse packets are discarded (simply done by esp_timer_get tick).
 */
static void rate_limiter_callback(void* arg)
{
	if(xSemaphoreTake(rate_limiter_sem,(TickType_t)0) == pdTRUE )
	{
		//trigger actions only if one of the values are != 0 or the button state changed.
		if((accumValues[0] != 0) || (accumValues[1] != 0) || (accumValues[2] != 0) || (buttonstate != newbuttonstate))
		{
			int8_t buffer[HID_MOUSE_IN_RPT_LEN];
			
			buffer[0] = newbuttonstate;   // Buttons
			buffer[4] = 0;              // AC Pan (unused)
			
			for(uint8_t i = 0; i<3; i++)
			{
				if(accumValues[i] > 127)
				{
					buffer[i+1] = 127;
					accumValues[i] -= 127;
				} else if(accumValues[i] < -127) {
					buffer[i+1] = -127;
					accumValues[i] += 127;
				} else {
					buffer[i+1] = accumValues[i];
					accumValues[i] = 0;
				}
			}
			ESP_LOGI(HID_LE_PRF_TAG,"mouse: %d,%d,%d,%d",buffer[0],buffer[1],buffer[2],buffer[3]);
			hid_dev_send_report(hidd_le_env.gatt_if, currentconnid,
				HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, (uint8_t *)buffer);
			buttonstate = newbuttonstate;
		}
		xSemaphoreGive(rate_limiter_sem);
	}
}
#endif /* CONFIG_MODULE_RATELIMITERACCUMULATE */


esp_err_t esp_hidd_register_callbacks(esp_hidd_event_cb_t callbacks)
{
    esp_err_t hidd_status;

    if(callbacks != NULL) {
   	    hidd_le_env.hidd_cb = callbacks;
    } else {
        return ESP_FAIL;
    }

    if((hidd_status = hidd_register_cb()) != ESP_OK) {
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
    
    //init stuff for rate limiting
    #if CONFIG_MODULE_USERATELIMITER
	    //init additional stuff, depending on discarding or accumulating mode.
	    #if CONFIG_MODULE_RATELIMITERACCUMULATE
			for(uint8_t i = 0; i<3; i++) accumValues[i] = 0;
			
			//create esp timer
			esp_timer_create_args_t args = {
				.callback = rate_limiter_callback,
				.arg = NULL,
				.dispatch_method = ESP_TIMER_TASK,
				.name = "ratelimiter"
			};
			if(esp_timer_create(&args,&intervalTimer) != ESP_OK) {
				ESP_LOGE(HID_LE_PRF_TAG,"error creating timer for rate limiter!");
				return ESP_FAIL;
			}
			if(esp_timer_start_periodic(intervalTimer,timeIntervalHIDCommand) != ESP_OK) {
				ESP_LOGE(HID_LE_PRF_TAG,"error starting rate limit timer");
				return ESP_FAIL;
			}
			rate_limiter_sem = xSemaphoreCreateBinary();
			if(rate_limiter_sem == NULL) {
				ESP_LOGE(HID_LE_PRF_TAG,"error creating semaphore!");
				return ESP_FAIL;
			}
			xSemaphoreGive(rate_limiter_sem);
	    #else /* CONFIG_MODULE_RATELIMITERACCUMULATE */
			lastTimeHIDCommandSent = esp_timer_get_time();
	    #endif /* CONFIG_MODULE_RATELIMITERACCUMULATE */
    #endif /* CONFIG_MODULE_USERATELIMITER */
    return ESP_OK;
}

#if CONFIG_MODULE_USERATELIMITER
/**
 * @brief Set the new interval for the ratelimiter (in us)
 * @param newinterval New interval [us]
 */
void esp_hidd_set_interval(uint32_t newinterval)
{
	if((newinterval > 100000) || (newinterval < 100))
	{
		ESP_LOGE(HID_LE_PRF_TAG,"Rate limiter out of range!");
		return;
	}
	timeIntervalHIDCommand = newinterval;
	#if CONFIG_MODULE_RATELIMITERACCUMULATE
		esp_timer_stop(intervalTimer);
		if(esp_timer_start_periodic(intervalTimer,timeIntervalHIDCommand) != ESP_OK) {
			ESP_LOGE(HID_LE_PRF_TAG,"error restarting rate limit timer");
		}
	#endif /* CONFIG_MODULE_RATELIMITERACCUMULATE */
}

/**
 * @brief Get the current interval for the ratelimiter (in us)
 * @return Current interval [us]
 */
uint32_t esp_hidd_get_interval(void)
{
	return timeIntervalHIDCommand;
}
#endif /* CONFIG_MODULE_USERATELIMITER */

esp_err_t esp_hidd_profile_deinit(void)
{
    uint16_t hidd_svc_hdl = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC];
    if (!hidd_le_env.enabled) {
        ESP_LOGE(HID_LE_PRF_TAG, "HID device profile already initialized");
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
    if (num_key > HID_KEYBOARD_IN_RPT_LEN - 2) {
        ESP_LOGE(HID_LE_PRF_TAG, "%s(), the number key should not be more than %d", __func__, HID_KEYBOARD_IN_RPT_LEN);
        return;
    }
   
    uint8_t buffer[HID_KEYBOARD_IN_RPT_LEN] = {0};
   
    buffer[0] = special_key_mask;
    
    for (int i = 0; i < num_key; i++) {
        buffer[i+2] = keyboard_cmd[i];
    }

    ESP_LOGD(HID_LE_PRF_TAG, "the key vaule = %d,%d,%d, %d, %d, %d,%d, %d", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
    hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
                        HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT, HID_KEYBOARD_IN_RPT_LEN, buffer);
    return;
}

void esp_hidd_send_mouse_value(uint16_t conn_id, uint8_t mouse_button, int8_t mickeys_x, int8_t mickeys_y, int8_t wheel)
{
    uint8_t buffer[HID_MOUSE_IN_RPT_LEN];
    
    buffer[0] = mouse_button;   // Buttons
    buffer[1] = mickeys_x;      // X
    buffer[2] = mickeys_y;      // Y
    buffer[3] = wheel;          // Wheel
    buffer[4] = 0;              // AC Pan (unused)
    
    #if CONFIG_MODULE_USERATELIMITER
		//// either we accumulate relative values and send them via the
		// interval timer
		// OR we discard them.
		#if CONFIG_MODULE_RATELIMITERACCUMULATE
			if(xSemaphoreTake(rate_limiter_sem,(TickType_t)5) == pdTRUE )
			{
				//use parameter values, if buffer array is used we loose sign.
				accumValues[0] += mickeys_x;
				accumValues[1] += mickeys_y;
				accumValues[2] += wheel;
				//limit accumulated values to max. ~4 reports
				for(uint8_t i = 0; i<3; i++)
				{
					if(accumValues[i] > 512) accumValues[i] = 512;
					if(accumValues[i] < -512) accumValues[i] = -512;
				}
				newbuttonstate = mouse_button;
				currentconnid = conn_id;
				xSemaphoreGive(rate_limiter_sem);
			} else ESP_LOGW(HID_LE_PRF_TAG,"Error obtaining semaphore");
		#else /* CONFIG_MODULE_RATELIMITERACCUMULATE */
			//check interval, just to be sure use the absolute value of the subtraction.
			//if the mouse button changes, send too.
			if((abs(esp_timer_get_time() - lastTimeHIDCommandSent) > timeIntervalHIDCommand) || (mouse_button != buttonstate))
			{
				hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
					HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, buffer);
				lastTimeHIDCommandSent = esp_timer_get_time();
				buttonstate = mouse_button;
			} else {
				ESP_LOGV(HID_LE_PRF_TAG,"Discarding mouse packet.");
			}
		#endif /* CONFIG_MODULE_RATELIMITERACCUMULATE */
    #else /* CONFIG_MODULE_USERATELIMITER */
		//// If we don't use the rate limiter, just send...
		hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
			HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, buffer);
    #endif /* CONFIG_MODULE_USERATELIMITER */
    return;
}
