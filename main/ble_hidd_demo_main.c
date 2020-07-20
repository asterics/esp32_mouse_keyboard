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
 *
 * Copyright 2020, Benjamin Aigner <beni@asterics-foundation.org>,<aignerb@technikum-wien.at>
 *
 * This file is mostly based on the Espressif ESP32 BLE HID example.
 * Adaption were made for:
 * * UART interface
 * * console input for testing purposes
 * * Joystick support (replacing vendor report)
 * * command input via UART for controlling the BLE interface (get & delete pairings,...)
 *
 */

/* Original license text:
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this software is
   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_hidd_prf_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "hid_dev.h"
#include "config.h"

/**
 * Brief:
 * This example Implemented BLE HID device profile related functions, in which the HID device
 * has 4 Reports (1 is mouse, 2 is keyboard and LED, 3 is Consumer Devices, 4 is Vendor devices).
 * Users can choose different reports according to their own application scenarios.
 * BLE HID profile inheritance and USB HID class.
 */

/**
 * Note:
 * 1. Win10 does not support vendor report , So SUPPORT_REPORT_VENDOR is always set to FALSE, it defines in hidd_le_prf_int.h
 * 2. Update connection parameters are not allowed during iPhone HID encryption, slave turns
 * off the ability to automatically update connection parameters during encryption.
 * 3. After our HID device is connected, the iPhones write 1 to the Report Characteristic Configuration Descriptor,
 * even if the HID encryption is not completed. This should actually be written 1 after the HID encryption is completed.
 * we modify the permissions of the Report Characteristic Configuration Descriptor to `ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE_ENCRYPTED`.
 * if you got `GATT_INSUF_ENCRYPTION` error, please ignore.
 */

#define HID_DEMO_TAG "HID_DEMO"



/** @warning Currently (07.2020) whitelisting devices is still not possible for all devices,
 * because many BT devices use resolvable random addresses, this seems to unsupported:
 * https://github.com/espressif/esp-idf/issues/1368
 * https://github.com/espressif/esp-idf/issues/2262
 * Therefore, if we enable pairing only on request, it is not possible to connect
 * to the ESP32 anymore. Either the ESP32 is visible by all devices or none.
 *
 * To circumvent any problems with this repository, if the config for
 * disabled pairing by default is active, we throw an error here.
 * @todo Check regularily for updates on the above mentioned issues.
 **/
#if CONFIG_MODULE_BT_PAIRING
#error "Sorry, currently the BT controller of the ESP32 does NOT support whitelisting. Please deactivate the pairing on demand option in make menuconfig!"
#endif

static uint16_t hid_conn_id = 0;
static bool sec_conn = false;
static bool send_volum_up = false;
#define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

#define MOUSE_SPEED 30
#define MAX_CMDLEN  100

#define EXT_UART_TAG "EXT_UART"
#define CONSOLE_UART_TAG "CONSOLE_UART"

static config_data_t config;

#define CMDSTATE_IDLE 0
#define CMDSTATE_GET_RAW 1
#define CMDSTATE_GET_ASCII 2

struct cmdBuf {
    int state;
    int expectedBytes;
    int bufferLength;
    uint8_t buf[MAX_CMDLEN];
};

static uint8_t manufacturer[19]= {'A', 's', 'T', 'e', 'R', 'I', 'C', 'S', ' ', 'F', 'o', 'u', 'n', 'd', 'a', 't', 'i', 'o', 'n'};


static uint8_t hidd_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

/** @brief Event bit, set if pairing is enabled
 * @note If MODULE_BT_PAIRING ist set in menuconfig, this bit is disable by default
 * and can be enabled via $PM1 , disabled via $PM0.
 * If MODULE_BT_PAIRING is not set, this bit will be set on boot.*/
#define SYSTEM_PAIRING_ENABLED (1<<0)

/** @brief Event bit, set if the ESP32 is currently advertising.
 *
 * Used for determining if we need to set advertising params again,
 * when the pairing mode is changed. */
#define SYSTEM_CURRENTLY_ADVERTISING (1<<1)

/** @brief Event group for system status */
EventGroupHandle_t eventgroup_system;

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x000A, //slave connection min interval, Time = min_interval * 1.25 msec
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

// config scan response data
///@todo Scan response is currently not used. If used, add state handling (adv start) according to ble/gatt_security_server example of Espressif
static esp_ble_adv_data_t hidd_adv_resp = {
    .set_scan_rsp = true,
    .include_name = true,
    .manufacturer_len = sizeof(manufacturer),
    .p_manufacturer_data = manufacturer,
};

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

uint8_t uppercase(uint8_t c)
{
    if ((c>='a') && (c<='z')) return (c-'a'+'A');
    return(c);
}

int get_int(const char * input, int index, int * value)
{
    int sign=1, result=0, valid=0;

    while (input[index]==' ') index++;   // skip leading spaces
    if (input[index]=='-') {
        sign=-1;
        index++;
    }
    while ((input[index]>='0') && (input[index]<='9'))
    {
        result= result*10+input[index]-'0';
        valid=1;
        index++;
    }
    while (input[index]==' ') index++;  // skip trailing spaces
    if (input[index]==',') index++;     // or a comma

    if (valid) {
        *value = result*sign;
        return (index);
    }
    return(0);
}


void update_config()
{
    nvs_handle my_handle;
    esp_err_t err = nvs_open("config_c", NVS_READWRITE, &my_handle);
    if(err != ESP_OK) ESP_LOGE("MAIN","error opening NVS");
    err = nvs_set_str(my_handle, "btname", config.bt_device_name);
    if(err != ESP_OK) ESP_LOGE("MAIN","error saving NVS - bt name");
    err = nvs_set_u8(my_handle, "locale", config.locale);
    if(err != ESP_OK) ESP_LOGE("MAIN","error saving NVS - locale");
    printf("Committing updates in NVS ... ");
    err = nvs_commit(my_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    nvs_close(my_handle);
}


static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    switch(event) {
    case ESP_HIDD_EVENT_REG_FINISH: {
        if (param->init_finish.state == ESP_HIDD_INIT_OK) {
            //esp_bd_addr_t rand_addr = {0x04,0x11,0x11,0x11,0x11,0x05};
            esp_ble_gap_set_device_name(config.bt_device_name);
            esp_ble_gap_config_adv_data(&hidd_adv_data);
        }
        break;
    }
    case ESP_BAT_EVENT_REG: {
        break;
    }
    case ESP_HIDD_EVENT_DEINIT_FINISH:
        break;
    case ESP_HIDD_EVENT_BLE_CONNECT: {
        ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_CONNECT");
        hid_conn_id = param->connect.conn_id;
        xEventGroupClearBits(eventgroup_system,SYSTEM_CURRENTLY_ADVERTISING);
        break;
    }
    case ESP_HIDD_EVENT_BLE_DISCONNECT: {
        sec_conn = false;
        ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT");
        esp_ble_gap_start_advertising(&hidd_adv_params);
        xEventGroupSetBits(eventgroup_system,SYSTEM_CURRENTLY_ADVERTISING);
        break;
    }
    case ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT: {
        ESP_LOGI(HID_DEMO_TAG, "%s, ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT", __func__);
        ESP_LOG_BUFFER_HEX(HID_DEMO_TAG, param->vendor_write.data, param->vendor_write.length);
    }
    case ESP_HIDD_EVENT_BLE_LED_OUT_WRITE_EVT: {
        ESP_LOGI(HID_DEMO_TAG, "%s, ESP_HIDD_EVENT_BLE_LED_OUT_WRITE_EVT, keyboard LED value: %d", __func__, param->vendor_write.data[0]);
    }
    default:
        break;
    }
    return;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&hidd_adv_params);
        xEventGroupSetBits(eventgroup_system,SYSTEM_CURRENTLY_ADVERTISING);
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        for(int i = 0; i < ESP_BD_ADDR_LEN; i++) {
            ESP_LOGD(HID_DEMO_TAG, "%x:",param->ble_security.ble_req.bd_addr[i]);
        }
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        sec_conn = true;
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(HID_DEMO_TAG, "remote BD_ADDR: %08x%04x",\
                 (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                 (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(HID_DEMO_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(HID_DEMO_TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
        if(!param->ble_security.auth_cmpl.success) {
            ESP_LOGE(HID_DEMO_TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
        } else {
            xEventGroupClearBits(eventgroup_system,SYSTEM_CURRENTLY_ADVERTISING);
        }
#if CONFIG_MODULE_BT_PAIRING
        //add connected device to whitelist (necessary if whitelist connections only).
        if(esp_ble_gap_update_whitelist(true,bd_addr,BLE_WL_ADDR_TYPE_PUBLIC) != ESP_OK)
        {
            ESP_LOGW(HID_DEMO_TAG,"cannot add device to whitelist, with public address");
        } else {
            ESP_LOGI(HID_DEMO_TAG,"added device to whitelist");
        }
        if(esp_ble_gap_update_whitelist(true,bd_addr,BLE_WL_ADDR_TYPE_RANDOM) != ESP_OK)
        {
            ESP_LOGW(HID_DEMO_TAG,"cannot add device to whitelist, with random address");
        }
#endif
        break;
    default:
        break;
    }
}


void processCommand(struct cmdBuf *cmdBuffer)
{
    //commands:
    // $ID
    // $PMx (0 or 1)
    // $GP
    // $DPx (number of paired device, starting with 0)
    // $NAME set name of bluetooth device

    if(cmdBuffer->bufferLength < 2) return;
    //easier this way than typecast in each str* function
    const char *input = (const char *) cmdBuffer->buf;
    int len = cmdBuffer->bufferLength;
    uint8_t keycode;
    const char nl = '\n';
    esp_ble_bond_dev_t * btdevlist;
    int counter;



    /**++++ commands without parameters ++++*/
    //get module ID
    if(strcmp(input,"ID") == 0)
    {
        uart_write_bytes(EX_UART_NUM, MODULE_ID, sizeof(MODULE_ID));
        ESP_LOGD(EXT_UART_TAG,"sending module id (ID)");
        return;
    }
    //disable pairing
    if(strcmp(input,"PM0") == 0)
    {
#if CONFIG_MODULE_BT_PAIRING
        ESP_LOGI(EXT_UART_TAG,"$PM0 - disabling pairing");
        xEventGroupClearBits(eventgroup_system,SYSTEM_PAIRING_ENABLED);
        if(hidd_adv_params.adv_filter_policy == ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY)
        {
            //restart advertising with whitelisted connection only (no connection is possible if not
            // on whitelist)
            hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST;
            esp_ble_gap_start_advertising(&hidd_adv_params);
        }
#else
        ESP_LOGW(EXT_UART_TAG,"Not available, cannot change pairing state (bug in esp-idf)");
#endif
        return;
    }
    //enable pairing
    if(strcmp(input,"PM1") == 0)
    {
#if CONFIG_MODULE_BT_PAIRING
        ESP_LOGI(EXT_UART_TAG,"$PM1 - enabling pairing");
        xEventGroupSetBits(eventgroup_system,SYSTEM_PAIRING_ENABLED);
        if(hidd_adv_params.adv_filter_policy == ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST)
        {
            //restart advertising with open connection
            hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
            esp_ble_gap_start_advertising(&hidd_adv_params);
        }
#else
        ESP_LOGW(EXT_UART_TAG,"Not available, cannot change pairing state (bug in esp-idf)");
#endif
        return;
    }

    //get all BT pairings
    if(strcmp(input,"GP") == 0)
    {
        counter = esp_ble_get_bond_device_num();
        char hexnum[5];

        if(counter > 0)
        {
            btdevlist = (esp_ble_bond_dev_t *) malloc(sizeof(esp_ble_bond_dev_t)*counter);
            if(btdevlist != NULL)
            {
                if(esp_ble_get_bond_device_list(&counter,btdevlist) == ESP_OK)
                {
                    ESP_LOGI(EXT_UART_TAG,"bonded devices (starting with index 0):");
                    ESP_LOGI(EXT_UART_TAG,"---------------------------------------");
                    for(uint8_t i = 0; i<counter; i++)
                    {
                        //print on monitor & external uart
                        esp_log_buffer_hex(EXT_UART_TAG, btdevlist[i].bd_addr, sizeof(esp_bd_addr_t));
                        for (int t=0; t<sizeof(esp_bd_addr_t); t++) {
                            sprintf(hexnum,"%02X ",btdevlist[i].bd_addr[t]);
                            uart_write_bytes(EX_UART_NUM, hexnum, 3);
                        }
                        uart_write_bytes(EX_UART_NUM,&nl,sizeof(nl)); //newline
                    }
                    ESP_LOGI(EXT_UART_TAG,"---------------------------------------");
                } else ESP_LOGE(EXT_UART_TAG,"error getting device list");
            } else ESP_LOGE(EXT_UART_TAG,"error allocating memory for device list");
        } else ESP_LOGE(EXT_UART_TAG,"error getting bonded devices count or no devices bonded");
        return;
    }


    //DP: delete one pairing
    if(input[0] == 'D' && input[1] == 'P')
    {
        int index_to_remove;
        if (!(get_int(input,2,&index_to_remove))) {
            ESP_LOGE(EXT_UART_TAG,"DP: invalid parameter, need integer");
            return;
        }

        counter = esp_ble_get_bond_device_num();
        if(counter == 0)
        {
            ESP_LOGE(EXT_UART_TAG,"error deleting device, no paired devices");
            return;
        }

        if(index_to_remove >= counter)
        {
            ESP_LOGE(EXT_UART_TAG,"error deleting device, number out of range");
            return;
        }
        if(counter >= 0)
        {
            btdevlist = (esp_ble_bond_dev_t *) malloc(sizeof(esp_ble_bond_dev_t)*counter);
            if(btdevlist != NULL)
            {
                if(esp_ble_get_bond_device_list(&counter,btdevlist) == ESP_OK)
                {
                    esp_ble_remove_bond_device(btdevlist[index_to_remove].bd_addr);
                    esp_ble_gap_update_whitelist(false,btdevlist[index_to_remove].bd_addr,BLE_WL_ADDR_TYPE_PUBLIC);
                    esp_ble_gap_update_whitelist(false,btdevlist[index_to_remove].bd_addr,BLE_WL_ADDR_TYPE_RANDOM);
                } else ESP_LOGE(EXT_UART_TAG,"error getting device list");
                free (btdevlist);
            } else ESP_LOGE(EXT_UART_TAG,"error allocating memory for device list");
        } else ESP_LOGE(EXT_UART_TAG,"error getting bonded devices count");
        return;
    }

    //set BT GATT advertising name
    if(strncmp(input,"NAME ", 5) == 0)
    {
        if ((strlen(input)>6) && (strlen(input)-4<MAX_BT_DEVICENAME_LENGTH))
        {
            strcpy (config.bt_device_name, input+5);
            update_config();
            ESP_LOGI(EXT_UART_TAG,"NAME: new bt device name was stored");
        }
        else ESP_LOGI(EXT_UART_TAG,"NAME: given bt name is too long or too short");
        return;
    }

    ESP_LOGE(EXT_UART_TAG,"No command executed with: %s ; len= %d\n",input,len);
}

void uart_parse_command (uint8_t character, struct cmdBuf * cmdBuffer)
{
    switch (cmdBuffer->state) {

    case CMDSTATE_IDLE:
        if (character==0xfd) {
            cmdBuffer->bufferLength=0;
            cmdBuffer->expectedBytes=8;   // 8 bytes for raw report size
            cmdBuffer->state=CMDSTATE_GET_RAW;
        }
        else if (character == '$') {
            cmdBuffer->bufferLength=0;   // we will read an ASCII-command until CR or LF
            cmdBuffer->state=CMDSTATE_GET_ASCII;
        }
        break;

    case CMDSTATE_GET_RAW:
        cmdBuffer->buf[cmdBuffer->bufferLength]=character;
        if ((cmdBuffer->bufferLength == 1) && (character==0x01)) { // we have a joystick report: increase by 4 bytes
            cmdBuffer->expectedBytes += 6;
            //ESP_LOGI(EXT_UART_TAG,"expecting 4 more bytes for joystick");
        }

        cmdBuffer->bufferLength++;
        cmdBuffer->expectedBytes--;
        if (!cmdBuffer->expectedBytes) {
            if(sec_conn == false) {
                ESP_LOGI(EXT_UART_TAG,"not connected, cannot send report");
            } else {
                if (cmdBuffer->buf[1] == 0x00) {   // keyboard report
                    esp_hidd_send_keyboard_value(hid_conn_id,cmdBuffer->buf[0],&cmdBuffer->buf[2],6);
                } else if (cmdBuffer->buf[1] == 0x01) {  // joystick report
                    ESP_LOGI(EXT_UART_TAG,"joystick: buttons: 0x%X:0x%X:0x%X:0x%X",cmdBuffer->buf[2],cmdBuffer->buf[3],cmdBuffer->buf[4],cmdBuffer->buf[5]);
                    //uint8_t joy[HID_JOYSTICK_IN_RPT_LEN];
                    //memcpy(joy,&cmdBuffer->buf[2],HID_JOYSTICK_IN_RPT_LEN);
                    ///@todo esp_hidd_send_joystick_value...
                } else if (cmdBuffer->buf[1] == 0x03) {  // mouse report
                    esp_hidd_send_mouse_value(hid_conn_id,cmdBuffer->buf[2],cmdBuffer->buf[3],cmdBuffer->buf[4],cmdBuffer->buf[5]);
                }
                else ESP_LOGE(EXT_UART_TAG,"Unknown RAW HID packet");
            }
            cmdBuffer->state=CMDSTATE_IDLE;
        }
        break;

    case CMDSTATE_GET_ASCII:
        // collect a command string until CR or LF are received
        if ((character==0x0d) || (character==0x0a))  {
            cmdBuffer->buf[cmdBuffer->bufferLength]=0;
            ESP_LOGI(EXT_UART_TAG,"sending command to parser: %s",cmdBuffer->buf);

            processCommand(cmdBuffer);
            cmdBuffer->state=CMDSTATE_IDLE;
        } else {
            if (cmdBuffer->bufferLength < MAX_CMDLEN-1)
                cmdBuffer->buf[cmdBuffer->bufferLength++]=character;
        }
        break;
    default:
        cmdBuffer->state=CMDSTATE_IDLE;
    }
}


void uart_external_task(void *pvParameters)
{
    char character;
    struct cmdBuf cmdBuffer;


    //Install UART driver, and get the queue.
    esp_err_t ret = ESP_OK;
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    //update UART config
    ret = uart_param_config(EX_UART_NUM, &uart_config);
    if(ret != ESP_OK)
    {
        ESP_LOGE(EXT_UART_TAG,"external UART param config failed");
    }

    //set IO pins
    ret = uart_set_pin(EX_UART_NUM, EX_SERIAL_TXPIN, EX_SERIAL_RXPIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if(ret != ESP_OK)
    {
        ESP_LOGE(EXT_UART_TAG,"external UART set pin failed");
    }
    uart_driver_install(EX_UART_NUM, UART_FIFO_LEN * 2, UART_FIFO_LEN * 2, 0, NULL, 0);

    ESP_LOGI(EXT_UART_TAG,"external UART processing task started");
    cmdBuffer.state=CMDSTATE_IDLE;

    while(1)
    {
        // read & process a single byte
        uart_read_bytes(EX_UART_NUM, (uint8_t*) &character, 1, portMAX_DELAY);
        uart_parse_command(character, &cmdBuffer);
    }
}

void blink_task(void *pvParameter)
{
    // Initialize GPIO pins
    gpio_pad_select_gpio(INDICATOR_LED_PIN);
    gpio_set_direction(INDICATOR_LED_PIN, GPIO_MODE_OUTPUT);
    int blinkTime;

    while(1) {

        if (sec_conn) blinkTime=1000;
        else blinkTime=250;

        /* Blink off (output low) */
        gpio_set_level(INDICATOR_LED_PIN, 0);
        vTaskDelay(blinkTime / portTICK_PERIOD_MS);
        /* Blink on (output high) */
        gpio_set_level(INDICATOR_LED_PIN, 1);
        vTaskDelay(blinkTime / portTICK_PERIOD_MS);
    }
}

void uart_console_task(void *pvParameters)
{
    char character;
    uint8_t kbdcmd[] = {28};
    //use input as HID test OR as input to processCommand (test commands)
    uint8_t hid_or_command = 0;
    struct cmdBuf commands;
    commands.state=CMDSTATE_IDLE;

    //Install UART driver, and get the queue.
    uart_driver_install(CONSOLE_UART_NUM, UART_FIFO_LEN * 2, UART_FIFO_LEN * 2, 0, NULL, 0);

    ESP_LOGI("UART","console UART processing task started");

    while(1)
    {
        // read single byte
        uart_read_bytes(CONSOLE_UART_NUM, (uint8_t*) &character, 1, portMAX_DELAY);

        //enable character forwarding to command parser (used to test the commands)
        if(character == '$' && hid_or_command == 0) hid_or_command = 1;
        //if command mode is enabled, forward to uart_parse_command and continue with next character receiving.
        if(hid_or_command == 1)
        {
            uart_parse_command(character,&commands);
            continue;
        }

        //if not in command mode, issue HID test commands.
        if(sec_conn == false) {
            ESP_LOGI(CONSOLE_UART_TAG,"Not connected, ignoring '%c'", character);
        } else {
            switch (character) {
			case 'm':
				esp_hidd_send_consumer_value(hid_conn_id,HID_CONSUMER_MUTE,true);
				esp_hidd_send_consumer_value(hid_conn_id,HID_CONSUMER_MUTE,false);
				ESP_LOGI(CONSOLE_UART_TAG,"consumer: mute");
				break;
			case 'p':
				esp_hidd_send_consumer_value(hid_conn_id,HID_CONSUMER_VOLUME_UP,true);
				esp_hidd_send_consumer_value(hid_conn_id,HID_CONSUMER_VOLUME_UP,false);
				ESP_LOGI(CONSOLE_UART_TAG,"consumer: volume plus");
				break;
			case 'o':
				esp_hidd_send_consumer_value(hid_conn_id,HID_CONSUMER_VOLUME_DOWN,true);
				esp_hidd_send_consumer_value(hid_conn_id,HID_CONSUMER_VOLUME_DOWN,false);
				ESP_LOGI(CONSOLE_UART_TAG,"consumer: volume minus");
				break;
            case 'a':
                esp_hidd_send_mouse_value(hid_conn_id,0,-MOUSE_SPEED,0,0);
                ESP_LOGI(CONSOLE_UART_TAG,"mouse: a");
                break;
            case 's':
                esp_hidd_send_mouse_value(hid_conn_id,0,0,MOUSE_SPEED,0);
                ESP_LOGI(CONSOLE_UART_TAG,"mouse: s");
                break;
            case 'd':
                esp_hidd_send_mouse_value(hid_conn_id,0,MOUSE_SPEED,0,0);
                ESP_LOGI(CONSOLE_UART_TAG,"mouse: d");
                break;
            case 'w':
                esp_hidd_send_mouse_value(hid_conn_id,0,0,-MOUSE_SPEED,0);
                ESP_LOGI(CONSOLE_UART_TAG,"mouse: w");
                break;
            case 'l':
                esp_hidd_send_mouse_value(hid_conn_id,(1<<0),0,0,0);
                esp_hidd_send_mouse_value(hid_conn_id,0,0,0,0);
                break;
            case 'r':
                esp_hidd_send_mouse_value(hid_conn_id,(1<<1),0,0,0);
                esp_hidd_send_mouse_value(hid_conn_id,0,0,0,0);
                ESP_LOGI(CONSOLE_UART_TAG,"mouse: r");
                break;
            case 'q':
                kbdcmd[0] = 28;
                esp_hidd_send_keyboard_value(hid_conn_id,0,kbdcmd,1);
                kbdcmd[0] = 0;
                esp_hidd_send_keyboard_value(hid_conn_id,0,kbdcmd,1);
                ESP_LOGI(CONSOLE_UART_TAG,"received q: sending key y (z for QWERTZ) for test purposes");
                break;
            default:
                ESP_LOGI(CONSOLE_UART_TAG,"received: %d, no HID action",character);
                break;
            }
        }
    }
}


void app_main(void)
{
    esp_err_t ret;

    // Initialize FreeRTOS elements
    eventgroup_system = xEventGroupCreate();
    if(eventgroup_system == NULL) ESP_LOGE(HID_DEMO_TAG, "Cannot initialize event group");
    //if set in KConfig, pairing is disable by default.
    //User has to enable pairing with $PM1
#if CONFIG_MODULE_BT_PAIRING
    ESP_LOGI(HID_DEMO_TAG,"pairing disabled by default");
    xEventGroupClearBits(eventgroup_system,SYSTEM_PAIRING_ENABLED);
    hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST;
#else
    ESP_LOGI(HID_DEMO_TAG,"pairing enabled by default");
    xEventGroupSetBits(eventgroup_system,SYSTEM_PAIRING_ENABLED);
    hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
#endif

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s initialize controller failed\n", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s enable controller failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
        return;
    }

    if((ret = esp_hidd_profile_init()) != ESP_OK) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
    }

    // Read config
    nvs_handle my_handle;
    ESP_LOGI("MAIN","loading configuration from NVS");
    ret = nvs_open("config_c", NVS_READWRITE, &my_handle);
    if(ret != ESP_OK) ESP_LOGE("MAIN","error opening NVS");
    size_t available_size = MAX_BT_DEVICENAME_LENGTH;
    strcpy(config.bt_device_name, GATTS_TAG);
    nvs_get_str (my_handle, "btname", config.bt_device_name, &available_size);
    if(ret != ESP_OK)
    {
        ESP_LOGI("MAIN","error reading NVS - bt name, setting to default");
        strcpy(config.bt_device_name, GATTS_TAG);
    } else ESP_LOGI("MAIN","bt device name is: %s",config.bt_device_name);

    ret = nvs_get_u8(my_handle, "locale", &config.locale);
    //if(ret != ESP_OK || config.locale >= LAYOUT_MAX)
    ///@todo implement keyboard layouts.
    if(ret != ESP_OK)
    {
        ESP_LOGI("MAIN","error reading NVS - locale, setting to US_INTERNATIONAL");
        //config.locale = LAYOUT_US_INTERNATIONAL;
    } else ESP_LOGI("MAIN","locale code is : %d",config.locale);
    nvs_close(my_handle);
    ///@todo How to handle the locale here? We have the memory for full lookups on the ESP32, but how to communicate this with the Teensy?

    ///register the callback function to the gap module
    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    xTaskCreate(&uart_console_task,  "console", 4096, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(&uart_external_task, "external", 4096, NULL, configMAX_PRIORITIES, NULL);
    ///@todo maybe reduce stack size for blink task? 4k words for blinky :-)?
    xTaskCreate(&blink_task, "blink", 4096, NULL, configMAX_PRIORITIES, NULL);
}

