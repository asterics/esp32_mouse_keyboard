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

#include "hal_ble.h"

#define LOG_TAG "hal_ble"

/** @brief Set a global log limit for this file */
#define LOG_LEVEL_BLE ESP_LOG_WARN

/** @brief Connection ID for an opened HID connection */
static uint16_t hid_conn_id = 0;
/** @brief Do we have a secure connection? */
static bool sec_conn = false;
/** @brief Bluetooth device name. */
char dev_name[64] = {0};

/** @brief Full UUID for the HID service */
static uint8_t hidd_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

/** @brief Advertising data for BLE */
static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x03c0,       //HID Generic,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
};

/** @brief Advertising parameters */
static esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x30,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


///@brief Is Keyboard interface active?
uint8_t activateKeyboard = 0;
///@brief Is Mouse interface active?
uint8_t activateMouse = 0;
///@brief Is Joystick interface active?
uint8_t activateJoystick = 0;

/** @brief Currently active keyboard report
 * This report is changed and sent on an incoming command
 * 1. byte is the modifier, bytes 2-7 are keycodes
 */
uint8_t keyboard_report[8];


/** @brief Currently active mouse report
 * This report is changed and sent on an incoming command
 * 1. byte is the button map, bytes 2-5 are X/Y/wheel/AC pan (int8_t)
 */
uint8_t mouse_report[5];


/** @brief Currently active joystick report
 * This report is changed and sent on an incoming command
 * Byte assignment:
 * [0]			button mask 1 (buttons 0-7)
 * [1]			button mask 2 (buttons 8-15)
 * [2]			button mask 3 (buttons 16-23)
 * [3]			button mask 4 (buttons 24-31)
 * [4]			bit 0-3: hat
 * [4]			bit 4-7: X axis low bits
 * [5]			bit 0-5: X axis high bits
 * [5]			bit 6-7: Y axis low bits
 * [6]			bit 0-7: Y axis high bits
 * [7]			bit 0-7: Z axis low bits
 * [8]			bit 0-1: Z axis high bits
 * [8]			bit 2-7: Z rotate low bits
 * [9]			bit 0-3: Z rotate high bits
 * [9]			bit 4-7: slider left low bits
 * [10]			bit 0-5: slider left high bits
 * [10]			bit 6-7: slider right low bits
 * [11]			bit 0-7: slider right high bits
 */
uint8_t joystick_report[12];


/** @brief Callback for HID events. */
static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
  switch(event) {
    case ESP_HIDD_EVENT_REG_FINISH: 
      if (param->init_finish.state == ESP_HIDD_INIT_OK) {
        //esp_bd_addr_t rand_addr = {0x04,0x11,0x11,0x11,0x11,0x05};
        if(strnlen(dev_name,10) > 0) esp_ble_gap_set_device_name(dev_name);
        else  esp_ble_gap_set_device_name("FLipMouse");
        esp_ble_gap_config_adv_data(&hidd_adv_data);
      }
      break;

    case ESP_BAT_EVENT_REG: break;
    case ESP_HIDD_EVENT_DEINIT_FINISH: break;
		case ESP_HIDD_EVENT_BLE_CONNECT:
      ESP_LOGI(LOG_TAG, "ESP_HIDD_EVENT_BLE_CONNECT");
      hid_conn_id = param->connect.conn_id;
      break;
    case ESP_HIDD_EVENT_BLE_DISCONNECT:
      sec_conn = false;
      ESP_LOGI(LOG_TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT");
      esp_ble_gap_start_advertising(&hidd_adv_params);
      break;
    case ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT:
      ESP_LOGI(LOG_TAG, "%s, ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT", __func__);
      ESP_LOG_BUFFER_HEX(LOG_TAG, param->vendor_write.data, param->vendor_write.length);
      break;
    default:
      break;
  }
  return;
}

/** @brief Callback for GAP events */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  switch (event)
  {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
      esp_ble_gap_start_advertising(&hidd_adv_params);
      break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
      for(int i = 0; i < ESP_BD_ADDR_LEN; i++) {
        ESP_LOGD(LOG_TAG, "%x:",param->ble_security.ble_req.bd_addr[i]);
      }
      esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
      break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
      sec_conn = true;
      esp_bd_addr_t bd_addr;
      memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
      ESP_LOGI(LOG_TAG, "remote BD_ADDR: %08x%04x",\
        (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
        (bd_addr[4] << 8) + bd_addr[5]);
      ESP_LOGI(LOG_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
      ESP_LOGI(LOG_TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
      if(!param->ble_security.auth_cmpl.success) {
          ESP_LOGE(LOG_TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
      }
      break;
    default:
        break;
  }
}

/** @brief CONTINOUS TASK - sending HID commands via BLE
 * 
 * This task is used to wait for HID commands, sent to the hid_ble
 * queue. If one command is received, it will be sent to a (possibly)
 * connected BLE device.
 */
void halBLETask(void * params)
{
  hid_cmd_t rx;
  
  //Empty queue if initialized (there might be something left from last connection)
  if(hid_ble != NULL) xQueueReset(hid_ble);
    while(1)
    {
	  //check if queue is initialized
	  if(hid_ble == NULL)
	  {
		ESP_LOGE(LOG_TAG,"ble hid queue not initialized, retry in 200ms");
		vTaskDelay(200/portTICK_PERIOD_MS);
		continue;
	  }
		
      //pend on MQ, if timeout triggers, just wait again.
      if(xQueueReceive(hid_ble,&rx,portMAX_DELAY))
      {
        //if we are not connected, discard.
        if(sec_conn == false) continue;
        
        //parse command (similar to usb_bridge controller)
        switch(rx.cmd[0] & 0xF0)
        {
          //reset all reports
          case 0x00:
            switch(rx.cmd[0] & 0x0F)
            {
              //reset report
              case 0:
                halBLEReset(0);
                break;
              //mouse X/Y report
              case 1:
                mouse_report[1] = rx.cmd[1];
                mouse_report[2] = rx.cmd[2];
                hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
                  HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, mouse_report);
                //reset the mouse_report's relative values (X/Y/wheel/pan)
                mouse_report[1] = 0;
                mouse_report[2] = 0;
                break;
              case 2:
				mouse_report[0] = rx.cmd[1];
				mouse_report[1] = rx.cmd[2];
				break;
              case 3:
				mouse_report[2] = rx.cmd[1];
				mouse_report[3] = rx.cmd[2];
				hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
                  HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, mouse_report);
                //reset the mouse_report's relative values (X/Y/wheel/pan)
                mouse_report[1] = 0;
                mouse_report[2] = 0;
                mouse_report[3] = 0;
				break;
              default: break;
            }
            break;
            
          //mouse handling
          case 0x10:
            switch(rx.cmd[0] & 0x0F)
            {
              case 0: //move X
                mouse_report[1] = rx.cmd[1];
                break;
              case 1: //move Y
                mouse_report[2] = rx.cmd[1];
                break;
              case 2: //move wheel
                mouse_report[3] = rx.cmd[1];
                break;
              /* Press & release */
              case 3: //left
                mouse_report[0] |= (1<<0);
                //send press report, wait for free EP (sending release is done after switch)
                hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
                  HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, mouse_report);
                mouse_report[0] &= ~(1<<0);
                break;
              case 4: //right
                mouse_report[0] |= (1<<1);
                //send press report, wait for free EP (sending release is done after switch)
                hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
                  HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, mouse_report);
                mouse_report[0] &= ~(1<<1);
                break;
              case 5: //middle
                mouse_report[0] |= (1<<2);
                //send press report, wait for free EP (sending release is done after switch)
                hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
                  HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, mouse_report);
                mouse_report[0] &= ~(1<<2);
                break;
              /* Press */
              case 6: //left
                mouse_report[0] |= (1<<0);
                break;
              case 7: //right
                mouse_report[0] |= (1<<1);
                break;
              case 8: //middle
                mouse_report[0] |= (1<<2);
                break;
              /* Release */
              case 9: //left
                mouse_report[0] &= ~(1<<0);
                break;
              case 10: //right
                mouse_report[0] &= ~(1<<1);
                break;
              case 11: //middle
                mouse_report[0] &= ~(1<<2);
                break;
              /* Toggle */
              case 12: //left
                mouse_report[0] ^= (1<<0);
                break;
              case 13: //right
                mouse_report[0] ^= (1<<1);
                break;
              case 14: //middle
                mouse_report[0] ^= (1<<2);
                break;
              case 15: //reset mouse (excepting keyboard & joystick)
                halBLEReset((1<<0)|(1<<1));
                break;
            }
            hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
              HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, mouse_report);
            //reset relative movement values
            mouse_report[1] = 0;
            mouse_report[2] = 0;
            mouse_report[3] = 0;
            break;
          //Keyboard handling
          case 0x20:
            switch(rx.cmd[0] & 0x0F)
            {
              case 0: //Press & release a key
                //press key & send
                add_keycode(rx.cmd[1], &keyboard_report[2]);
                hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
                  HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT, HID_KEYBOARD_IN_RPT_LEN, keyboard_report);
                //remove keycode
                //sending the second report is done after this switch
                remove_keycode(rx.cmd[1], &keyboard_report[2]);
                break;
              case 1: //Press a key
                add_keycode(rx.cmd[1], &keyboard_report[2]);
                break;
              case 2: //Release a key
                remove_keycode(rx.cmd[1], &keyboard_report[2]);
                break;
              case 3:
                if(is_in_keycode_arr(rx.cmd[1],&keyboard_report[2])) remove_keycode(rx.cmd[1], &keyboard_report[2]);
                else add_keycode(rx.cmd[1], &keyboard_report[2]);
                break;
              case 4: //Press & release a modifier (mask!)
                keyboard_report[0] |= rx.cmd[1];
                hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
                  HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT, HID_KEYBOARD_IN_RPT_LEN, keyboard_report);
                //remove modifier
                //sending the second report is done after this switch
                keyboard_report[0] &= ~rx.cmd[1];
                break;
              case 5: //Press a modifier (mask!)
                keyboard_report[0] |= rx.cmd[1];
                break;
              case 6: //Release a modifier (mask!)
                keyboard_report[0] &= ~rx.cmd[1];
                break;
              case 7: //Toggle a modifier (mask!)
                keyboard_report[0] ^= rx.cmd[1];
                break;
              case 15: //reset mouse (excepting mouse & joystick)
                halBLEReset((1<<1)|(1<<2));
                break;
            }
            hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
              HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT, HID_KEYBOARD_IN_RPT_LEN, keyboard_report);
            break;
          case 0x30:
            switch(rx.cmd[0] & 0x0F)
            {
              case 0: //Press & release button/hat
                //test if it is buttons or hat?
                if((rx.cmd[1] & (1<<7)) == 0)
                {
                  //buttons, map to corresponding bits in 4 bytes
                  if(rx.cmd[1] <= 7) joystick_report[0] |= (1<<rx.cmd[1]);
                  else if(rx.cmd[1] <= 15) joystick_report[1] |= (1<<(rx.cmd[1]-8));
                  else if(rx.cmd[1] <= 24) joystick_report[2] |= (1<<(rx.cmd[1]-16));
                  else if(rx.cmd[1] <= 31) joystick_report[3] |= (1<<(rx.cmd[1]-24));
                } else {
                  //hat, remove bit 7 and set to report (don't touch 4 bits of X)
                  joystick_report[4] = (joystick_report[4] & 0xF0) | (rx.cmd[1] & 0x0F);
                }
                //send press action
                hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
                  HID_RPT_ID_JOY_IN, HID_REPORT_TYPE_INPUT, HID_JOYSTICK_IN_RPT_LEN, joystick_report);
                //release button/hat
                //test if it is buttons or hat?
                if((rx.cmd[1] & (1<<7)) == 0)
                {
                  //buttons, map to corresponding bits in 4 bytes
                  if(rx.cmd[1] <= 7) joystick_report[0] &= ~(1<<rx.cmd[1]);
                  else if(rx.cmd[1] <= 15) joystick_report[1] &= ~(1<<(rx.cmd[1]-8));
                  else if(rx.cmd[1] <= 24) joystick_report[2] &= ~(1<<(rx.cmd[1]-16));
                  else if(rx.cmd[1] <= 31) joystick_report[3] &= ~(1<<(rx.cmd[1]-24));
                } else {
                  //hat release means always 15.
                  joystick_report[4] = (joystick_report[4] & 0xF0) | 0x0F;
                }
                break;
              case 1: //Press button/hat
                //test if it is buttons or hat?
                if((rx.cmd[1] & (1<<7)) == 0)
                {
                  //buttons, map to corresponding bits in 4 bytes
                  if(rx.cmd[1] <= 7) joystick_report[0] |= (1<<rx.cmd[1]);
                  else if(rx.cmd[1] <= 15) joystick_report[1] |= (1<<(rx.cmd[1]-8));
                  else if(rx.cmd[1] <= 24) joystick_report[2] |= (1<<(rx.cmd[1]-16));
                  else if(rx.cmd[1] <= 31) joystick_report[3] |= (1<<(rx.cmd[1]-24));
                } else {
                  //hat, remove bit 7 and set to report (don't touch 4 bits of X)
                  joystick_report[4] = (joystick_report[4] & 0xF0) | (rx.cmd[1] & 0x0F);
                }
                break;
              case 2: //Release button/hat
                //test if it is buttons or hat?
                if((rx.cmd[1] & (1<<7)) == 0)
                {
                  //buttons, map to corresponding bits in 4 bytes
                  if(rx.cmd[1] <= 7) joystick_report[0] &= ~(1<<rx.cmd[1]);
                  else if(rx.cmd[1] <= 15) joystick_report[1] &= ~(1<<(rx.cmd[1]-8));
                  else if(rx.cmd[1] <= 24) joystick_report[2] &= ~(1<<(rx.cmd[1]-16));
                  else if(rx.cmd[1] <= 31) joystick_report[3] &= ~(1<<(rx.cmd[1]-24));
                } else {
                  //hat release means always 15.
                  joystick_report[4] = (joystick_report[4] & 0xF0) | 0x0F;
                }
                break;
              case 4: //X Axis
                //preserve 4 bits of hat
                joystick_report[4] = (joystick_report[4] & 0x0F) | ((rx.cmd[1] & 0x0F) << 4);
                //preserve 2 bits of Y
                joystick_report[5] = (joystick_report[5] & 0xC0) | ((rx.cmd[1] & 0xF0) >> 4) | ((rx.cmd[2] & 0x03) << 4);
                break;
              case 5: //Y Axis
                //preserve 6 bits of X
                joystick_report[5] = (joystick_report[5] & 0x3F) | ((rx.cmd[1] & 0x03) << 6);
                //save remaining Y
                joystick_report[6] = ((rx.cmd[1] & 0xFC) >> 2) | ((rx.cmd[2] & 0x03) << 6);
                break;
              case 6: //Z Axis
                joystick_report[7] = rx.cmd[1];
                joystick_report[8] = (joystick_report[8] & 0xFC) | (rx.cmd[2] & 0x03);
                break;
              case 7: //Z-rotate
                //preserve 2 bits of Z-axis
                joystick_report[8] = (joystick_report[8] & 0x03) | ((rx.cmd[1] & 0x3F) << 2);
                //preserve slider left & combine 2 bits of LSB & MSB to one nibble
                joystick_report[9] = (joystick_report[9] & 0xF0) | ((rx.cmd[1] & 0xC0) >> 6) | ((rx.cmd[2] & 0x03) << 2);
                break;
              case 8: //slider left
                //preserve 4 bits of Z-rotate, add low nibble of first byte
                joystick_report[9] = (joystick_report[9] & 0x0F) | ((rx.cmd[1] & 0x0F) << 4);
                //preserve 2 bits of slider right, add high nibble of first byte and second byte
                joystick_report[10] = (joystick_report[10] & 0xC0) | ((rx.cmd[1] & 0xF0) >> 4) | ((rx.cmd[2] & 0x03) << 4);
                break;
              case 9: //slider right
                //preserve 6 bits of slider left, add 2 bits for slider right
                joystick_report[10] = (joystick_report[10] & 0x3F) | ((rx.cmd[1] & 0x03) << 6);
                //save remaining slider right
                joystick_report[11] = ((rx.cmd[1] & 0xFC) >> 2) | ((rx.cmd[2] & 0x03) << 6);
                break;
              case 15: //reset joystick (excepting mouse & keyboard)
                halBLEReset((1<<0)|(1<<2));
                break;
            }
            hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
              HID_RPT_ID_JOY_IN, HID_REPORT_TYPE_INPUT, HID_JOYSTICK_IN_RPT_LEN, joystick_report);
            break;
          }   
      }
    }
}

/** @brief Activate/deactivate pairing mode
 * @param enable If set to != 0, pairing will be enabled. Disabled if == 0
 * @return ESP_OK on success, ESP_FAIL otherwise*/
esp_err_t halBLESetPairing(uint8_t enable)
{
	ESP_LOGW(LOG_TAG,"en-/disable pairing not implemented");
	return ESP_OK;
}

/** @brief Get connection status
 * @return 0 if not connected, != 0 if connected */
uint8_t halBLEIsConnected(void)
{
  if(sec_conn == false) return 0;
  else return 1;
}

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
esp_err_t halBLEEnDisable(int onoff) {
  return ESP_OK;
}


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
void halBLEReset(uint8_t exceptDevice)
{
  uint8_t send = 0;
  
  //check if we need to reset the mouse report
  if(!(exceptDevice & (1<<2))) {
    //compare the global mouse report against an empty one
    uint8_t m[sizeof(mouse_report)] = {0};
    if(memcmp(mouse_report,m,sizeof(mouse_report)) != 0) {
      //if the global one wasn't empty before, we empty and send it
      memset(mouse_report,0,sizeof(mouse_report));
      send = 1;
    }
  }
  //same procedure as for the mouse....
  if(!(exceptDevice & (1<<0))) {
    uint8_t k[sizeof(keyboard_report)] = {0};
    if(memcmp(keyboard_report,k,sizeof(keyboard_report)) != 0) {
      memset(keyboard_report,0,sizeof(keyboard_report));
      send = 1;
    }
  }
  //and once again for the joystick.
  if(!(exceptDevice & (1<<1))) {
    uint8_t j[sizeof(joystick_report)] = {0};
    if(memcmp(joystick_report,j,sizeof(joystick_report)) != 0) {
      memset(joystick_report,0,sizeof(joystick_report));
      send = 1;
    }
  }
  
  //we don't need to send empty reports all the time, just if they
  //weren't empty before.
  if(send == 0) return;
  
  if(sec_conn && activateMouse) {
    hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
      HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, mouse_report);
  }
  
  if(sec_conn && activateKeyboard) {
    hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
      HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT, HID_KEYBOARD_IN_RPT_LEN, keyboard_report);
  }
  
  if(sec_conn && activateJoystick) {
    hid_dev_send_report(hidd_le_env.gatt_if, hid_conn_id,
      HID_RPT_ID_JOY_IN, HID_REPORT_TYPE_INPUT, HID_JOYSTICK_IN_RPT_LEN, joystick_report);
  }
}

/** @brief Main init function to start HID interface (C interface)
 * @see hid_ble */
esp_err_t halBLEInit(uint8_t enableKeyboard, uint8_t enableMouse, uint8_t enableJoystick, char* deviceName)
{
  activateKeyboard = enableKeyboard;
  activateMouse = enableMouse;
  activateJoystick = enableJoystick;
  
  // Initialize NVS.
  esp_err_t ret = nvs_flash_init();
  
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(LOG_TAG, "%s initialize controller failed\n", __func__);
    return ESP_FAIL;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(LOG_TAG, "%s enable controller failed\n", __func__);
    return ESP_FAIL;
  }

  ret = esp_bluedroid_init();
  if (ret) {
    ESP_LOGE(LOG_TAG, "%s init bluedroid failed\n", __func__);
    return ESP_FAIL;
  }

  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(LOG_TAG, "%s init bluedroid failed\n", __func__);
    return ESP_FAIL;
  }
  
  if(!hidd_le_env.enabled) {
    memset(&hidd_le_env, 0, sizeof(hidd_le_env_t));
    hidd_le_env.enabled = true;
  }
  
  esp_ble_gap_register_callback(gap_event_handler);
  
  hidd_le_env.hidd_cb = hidd_event_callback;
  if(hidd_register_cb() != ESP_OK) {
    ESP_LOGE(LOG_TAG,"register CB failed");
    return ESP_FAIL;
  }
  esp_ble_gatts_app_register(BATTRAY_APP_ID);
  if(esp_ble_gatts_app_register(HIDD_APP_ID) != ESP_OK) {
    ESP_LOGE(LOG_TAG,"Register App failed");
    return ESP_FAIL;
  }

  /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;     //bonding with peer device after authentication
  esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
  uint8_t key_size = 16;      //the key size should be 7~16 bytes
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
  /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribut to you,
  and the response key means which key you can distribut to the Master;
  If your BLE device act as a master, the response key means you hope which types of key of the slave should distribut to you, 
  and the init key means which key you can distribut to the slave. */
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
  
  //create BLE task
  xTaskCreate(&halBLETask, "ble_task", TASK_BLE_STACKSIZE, NULL, 10, NULL);
  
  //save device name
  strncpy(dev_name,deviceName,63);
  
  //set log level according to define
  esp_log_level_set(HID_LE_PRF_TAG,LOG_LEVEL_BLE);
  esp_log_level_set(LOG_TAG,LOG_LEVEL_BLE);
  
  return ESP_OK;
}
