// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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

// Adaptions done:
// Copyright 2017 Benjamin Aigner <beni@asterics-foundation.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "config.h"

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
#include "keyboard.h"

#define GATTS_TAG "FABI/FLIPMOUSE"

static uint16_t hid_conn_id = 0;
static bool sec_conn = false;
static uint8_t keycode_modifier;
static uint8_t keycode_deadkey_first;
static uint8_t keycode_arr[6];
//static joystick_data_t joystick;//currently unused, no joystick implemented
static config_data_t config;


#define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

const char hid_device_name_fabi[] = "FABI";
const char hid_device_name_flipmouse[] = "FLipMouse";
static uint8_t hidd_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x30,
    .appearance = 0x03c0,       //HID Generic,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
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

/*
void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void gpio_task_example(void* arg)
{
    static uint8_t i = 0;
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
            if(i == 0) {
            ++i;
            }
        }
    }
}

static void gpio_demo_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode        
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio intrrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    //remove isr handler for gpio number.
    gpio_isr_handler_remove(GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin again
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);

}*/



static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    switch(event) {
        case ESP_HIDD_EVENT_REG_FINISH: {
            if (param->init_finish.state == ESP_HIDD_INIT_OK) {
                //esp_bd_addr_t rand_addr = {0x04,0x11,0x11,0x11,0x11,0x05};
                if(config.bt_device_name_index == 1)
                {
                    esp_ble_gap_set_device_name(hid_device_name_flipmouse);
                } else {
                    esp_ble_gap_set_device_name(hid_device_name_fabi);
                }
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
            hid_conn_id = param->connect.conn_id;
            sec_conn = true; //TODO: right here?!?
            ESP_LOGE(GATTS_TAG,"%s(), ESP_HIDD_EVENT_BLE_CONNECT", __func__);
            break;
        }
        case ESP_HIDD_EVENT_BLE_DISCONNECT: {
            sec_conn = false;
            hid_conn_id = 0;
            ESP_LOGE(GATTS_TAG,"%s(), ESP_HIDD_EVENT_BLE_DISCONNECT", __func__);
            esp_ble_gap_start_advertising(&hidd_adv_params);
            break;
        }
        default:
            ESP_LOGE(GATTS_TAG,"%s(), unhandled event: %d", __func__,event);
            break;
    }
    return;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;
     /*case ESP_GAP_BLE_SEC_REQ_EVT:
        for(int i = 0; i < ESP_BD_ADDR_LEN; i++) {
             LOG_DEBUG("%x:",param->ble_security.ble_req.bd_addr[i]);
        }
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
	 break;*/
     case ESP_GAP_BLE_AUTH_CMPL_EVT:
        sec_conn = true;
        if(param->ble_security.auth_cmpl.success)
        {
            ESP_LOGI(GATTS_TAG,"status = success, ESP_GAP_BLE_AUTH_CMPL_EVT");
        } else {
            ESP_LOGI(GATTS_TAG,"status = fail, ESP_GAP_BLE_AUTH_CMPL_EVT");
        }
        break;
    //unused events 
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT: break;
    //do we need this? occurs on win10 connect.
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT: break;
    
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:                           /* passkey request event */
        //esp_ble_passkey_reply(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, true, 0x00);
        ESP_LOGI(GATTS_TAG,"ESP_GAP_BLE_PASSKEY_REQ_EVT");
        break;
    case ESP_GAP_BLE_OOB_REQ_EVT:                                /* OOB request event */
        ESP_LOGI(GATTS_TAG,"ESP_GAP_BLE_OOB_REQ_EVT");
        break;
    case ESP_GAP_BLE_LOCAL_IR_EVT:                               /* BLE local IR event */
        ESP_LOGI(GATTS_TAG,"ESP_GAP_BLE_LOCAL_IR_EVT");
        break;
    case ESP_GAP_BLE_LOCAL_ER_EVT:                               /* BLE local ER event */
        ESP_LOGI(GATTS_TAG,"ESP_GAP_BLE_LOCAL_ER_EVT");
        break;
    case ESP_GAP_BLE_NC_REQ_EVT:
        ESP_LOGI(GATTS_TAG,"ESP_GAP_BLE_NC_REQ_EVT");
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        /* send the positive(true) security response to the peer device to accept the security request.
        If not accept the security request, should sent the security response with negative(false) accept value*/
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        ESP_LOGI(GATTS_TAG,"ESP_GAP_BLE_SEC_REQ_EVT");
        break;
    
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
        ///show the passkey number to the user to input it in the peer deivce.
        ESP_LOGI(GATTS_TAG,"The passkey Notify number:%d", param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_KEY_EVT:
        //shows the ble key info share with peer device to the user.
        ESP_LOGI(GATTS_TAG,"key type = %d", param->ble_security.ble_key.key_type);
        break;
    
    default:
        ESP_LOGW(GATTS_TAG,"unhandled event: %d",event);
        break;
    }
}


void update_config()
{
    nvs_handle my_handle;
    esp_err_t err = nvs_open("fabi_c", NVS_READWRITE, &my_handle);
    if(err != ESP_OK) ESP_LOGE("MAIN","error opening NVS");
    err = nvs_set_u8(my_handle, "btname_i", config.bt_device_name_index);
    if(err != ESP_OK) ESP_LOGE("MAIN","error saving NVS - bt name");
    err = nvs_set_u8(my_handle, "locale", config.locale);
    if(err != ESP_OK) ESP_LOGE("MAIN","error saving NVS - locale");
    printf("Committing updates in NVS ... ");
    err = nvs_commit(my_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    nvs_close(my_handle);
}

void process_uart(uint8_t *input, uint16_t len)
{
    //tested commands:
    //ID_FABI
    //ID_FLIPMOUSE
    //ID
    //PMx (PM0 PM1)
    //GP
    //DPx (number of paired device, 1-x)
    //KW (+layouts)
    //KD/KU
    //KR
    //KL
    
    //untested commands:
    //KC
    //M
    
    //TBD: joystick (everything)
    // KK (if necessary)
    
    if(len < 2) return;
    //easier this way than typecast in each str* function
    const char *input2 = (const char *) input;
    int counter;
    char nl = '\n';
    uint8_t keycode = 0;
    esp_ble_bond_dev_t *btdevlist = NULL;
    #define DEBUG_TAG "UART_PARSER"
    
    /**++++ commands without parameters ++++*/
    //get module ID
    if(strcmp(input2,"ID") == 0) 
    {
        uart_write_bytes(EX_UART_NUM, MODULE_ID, sizeof(MODULE_ID));
        ESP_LOGD(DEBUG_TAG,"sending module id (ID)");
        return;
    }

    //get all BT pairings
    if(strcmp(input2,"GP") == 0)
    {
        counter = esp_ble_get_bond_device_num();
        if(counter > 0)
        {
            btdevlist = (esp_ble_bond_dev_t *) malloc(sizeof(esp_ble_bond_dev_t)*counter);
            if(btdevlist != NULL)
            {
                if(esp_ble_get_bond_device_list(&counter,btdevlist) == ESP_OK)
                {
                    ESP_LOGI(DEBUG_TAG,"bonded devices (starting with index 0):");
                    ESP_LOGI(DEBUG_TAG,"---------------------------------------");
                    for(uint8_t i = 0; i<counter;i++)
                    {
                        //print on monitor & external uart
                        esp_log_buffer_hex(DEBUG_TAG, btdevlist[i].bd_addr, sizeof(esp_bd_addr_t));
                        uart_write_bytes(EX_UART_NUM, (char *)btdevlist[i].bd_addr, sizeof(esp_bd_addr_t));
                        uart_write_bytes(EX_UART_NUM,&nl,sizeof(nl)); //newline
                    }
                    ESP_LOGI(DEBUG_TAG,"---------------------------------------");
                } else ESP_LOGE(DEBUG_TAG,"error getting device list");
            } else ESP_LOGE(DEBUG_TAG,"error allocating memory for device list");
        } else ESP_LOGE(DEBUG_TAG,"error getting bonded devices count or no devices bonded");
        return;
    }
    
    //joystick: update data (send a report)
    if(strcmp(input2,"JU") == 0) 
    {
        //TBD: joystick
        ESP_LOGD(DEBUG_TAG,"TBD! joystick: send report (JU)");
        return;
    }
    
    //keyboard: release all
    if(strcmp(input2,"KR") == 0)
    {
        for(uint8_t i = 0; i < sizeof(keycode_arr); i++) keycode_arr[i] = 0;
        keycode_modifier = 0;
        keycode_deadkey_first = 0;
        esp_hidd_send_keyboard_value(hid_conn_id,keycode_modifier,keycode_arr,sizeof(keycode_arr));
        ESP_LOGD(DEBUG_TAG,"keyboard: release all (KR)");
        return;
    }
    
    /**++++ commands with parameters ++++*/
    switch(input[0])
    {
        case 'K': //keyboard
            //key up
            if(input[1] == 'U' && len == 3) 
            {
                remove_keycode(input[2],keycode_arr);
                esp_hidd_send_keyboard_value(hid_conn_id,keycode_modifier,keycode_arr,sizeof(keycode_arr));
                return;
            }
            //key down
            if(input[1] == 'D' && len == 3) 
            {
                add_keycode(input[2],keycode_arr);
                esp_hidd_send_keyboard_value(hid_conn_id,keycode_modifier,keycode_arr,sizeof(keycode_arr));
                return;
            }
            //keyboard, set locale
            if(input[1] == 'L' && len == 3) 
            { 
                if(input[2] < LAYOUT_MAX) 
                {
                    config.locale = input[2];
                    update_config();
                } else ESP_LOGE(DEBUG_TAG,"Locale out of range");
                return;
            }
            
            //keyboard, write
            if(input[1] == 'W')
            {
                ESP_LOGI(DEBUG_TAG,"sending keyboard write, len: %d (bytes, not characters!)",len-2);
                for(uint16_t i = 2; i<len; i++)
                {
                    if(input[i] == '\0') 
                    {
                        ESP_LOGI(DEBUG_TAG,"terminated string, ending KW");
                        break;
                    }
                    keycode = parse_for_keycode(input[i],config.locale,&keycode_modifier,&keycode_deadkey_first); //send current byte to parser
                    if(keycode == 0) 
                    {
                        ESP_LOGI(DEBUG_TAG,"keycode is 0 for 0x%X, skipping to next byte",input[i]);
                        continue; //if no keycode is found,skip to next byte (might be a 16bit UTF8)
                    }
                    ESP_LOGI(DEBUG_TAG,"keycode: %d, modifier: %d, deadkey: %d",keycode,keycode_modifier,keycode_deadkey_first);
                    //TODO: do deadkey sequence...
                    
                    //if a keycode is found, add to keycodes for HID
                    add_keycode(keycode,keycode_arr);
                    ESP_LOGI(DEBUG_TAG,"keycode arr, adding and removing:");
                    esp_log_buffer_hex(DEBUG_TAG,keycode_arr,6);
                    //send to device
                    esp_hidd_send_keyboard_value(hid_conn_id,keycode_modifier,keycode_arr,sizeof(keycode_arr));
                    //wait 1 tick to release key
                    vTaskDelay(1); //suspend for 1 tick
                    //remove key & send report
                    remove_keycode(keycode,keycode_arr);
                    esp_log_buffer_hex(DEBUG_TAG,keycode_arr,6);
                    esp_hidd_send_keyboard_value(hid_conn_id,keycode_modifier,keycode_arr,sizeof(keycode_arr));
                    vTaskDelay(1); //suspend for 1 tick
                }
                return;
            }
            
            //keyboard, get keycode for unicode bytes
            if(input[1] == 'C' && len == 4) 
            {
                keycode = get_keycode(input[2],config.locale,&keycode_modifier,&keycode_deadkey_first);
                //if the first byte is not sufficient, try with second byte.
                if(keycode == 0) 
                { 
                    keycode = get_keycode(input[3],config.locale,&keycode_modifier,&keycode_deadkey_first);
                }
                //first the keycode + modifier are sent.
                //deadkey starting key is sent afterwards (0 if not necessary)
                uart_write_bytes(EX_UART_NUM, (char *)&keycode, sizeof(keycode));
                uart_write_bytes(EX_UART_NUM, (char *)&keycode_modifier, sizeof(keycode_modifier));
                uart_write_bytes(EX_UART_NUM, (char *)&keycode_deadkey_first, sizeof(keycode_deadkey_first));
                uart_write_bytes(EX_UART_NUM, &nl, sizeof(nl));
                return;
            }
            //keyboard, get unicode mapping between 2 locales
            if(input[1] == 'K' && len == 6) 
            { 
                //cpoint = get_cpoint((input[2] << 8) || input[3],input[4],input[5]);
                //uart_write_bytes(EX_UART_NUM, (char *)&cpoint, sizeof(cpoint));
                //uart_write_bytes(EX_UART_NUM, &nl, sizeof(nl));
                //TODO: is this necessary or useful anyway??
                return;
            }
            break;
        case 'J': //joystick
            //joystick, set X,Y,Z (each 0-1023)
            if(input[1] == 'S' && len == 8) { }
            //joystick, set Z rotate, slider left, slider right (each 0-1023)
            if(input[1] == 'T' && len == 8) { }
            //joystick, set buttons (bitmap for button nr 1-16)
            if(input[1] == 'B' && len == 4) { }
            //joystick, set hat (0-360°)
            if(input[1] == 'H' && len == 4) { }
            //TBD: joystick
            ESP_LOGD(DEBUG_TAG,"TBD! joystick");
            break;
        
        default: //test for management commands
            break;
    }
    //mouse input
    //M<buttons><X><Y><wheel>
    if(input[0] == 'M' && len == 5)
    {
        esp_hidd_send_mouse_value(hid_conn_id,input[1],input[2],input[3],input[4]);
        ESP_LOGD(DEBUG_TAG,"mouse movement");
        return;
    }
    //BT: delete one pairing
    if(input[0] == 'D' && input[1] == 'P' && len == 3)
    {
        counter = esp_ble_get_bond_device_num();
        if(counter == 0)
        {
            ESP_LOGE(DEBUG_TAG,"error deleting device, no paired devices");
            return; 
        }
        
        if(input[2] >= '0' && input[2] <= '9') input[2] -= '0';
        if(input[2] >= counter)
        {
            ESP_LOGE(DEBUG_TAG,"error deleting device, number out of range");
            return;
        }
        if(counter >= 0)
        {
            btdevlist = (esp_ble_bond_dev_t *) malloc(sizeof(esp_ble_bond_dev_t)*counter);
            if(btdevlist != NULL)
            {
                if(esp_ble_get_bond_device_list(&counter,btdevlist) == ESP_OK)
                {
                    esp_ble_remove_bond_device(btdevlist[input[2]].bd_addr);
                } else ESP_LOGE(DEBUG_TAG,"error getting device list");
            } else ESP_LOGE(DEBUG_TAG,"error allocating memory for device list");
        } else ESP_LOGE(DEBUG_TAG,"error getting bonded devices count");
        return;
    }
    //BT: enable/disable discoverable/pairing
    if(input[0] == 'P' && input[1] == 'M' && len == 3)
    {
        if(input[2] == 0 || input[2] == '0')
        {
            if(esp_ble_gap_stop_advertising() != ESP_OK)
            {
                ESP_LOGE(DEBUG_TAG,"error stopping advertising");
            }
        } else if(input[2] == 1 || input[2] == '1') {
            if(esp_ble_gap_start_advertising(&hidd_adv_params) != ESP_OK)
            {
                ESP_LOGE(DEBUG_TAG,"error starting advertising");
            } else {
                //TODO: terminate any connection to be pairable?
            }
        } else ESP_LOGE(DEBUG_TAG,"parameter error, either 0/1 or '0'/'1'");
        ESP_LOGD(DEBUG_TAG,"management: pairing %d (PM)",input[2]);
        return;
    }
    
    //set BT names (either FABI or FLipMouse)
    if(strcmp(input2,"ID_FABI") == 0)
    {
        config.bt_device_name_index = 0;
        update_config();
        ESP_LOGD(DEBUG_TAG,"management: set device name to FABI (ID_FABI)");
        return;
    }
    if(strcmp(input2,"ID_FLIPMOUSE") == 0) 
    {
        config.bt_device_name_index = 1;
        update_config();
        ESP_LOGD(DEBUG_TAG,"management: set device name to FLipMouse (ID_FLIPMOUSE)");
        return;
    }
    
    
    ESP_LOGE(DEBUG_TAG,"No command executed with: %s ; len= %d\n",input,len);
}


void uart_stdin(void *pvParameters)
{
    static uint8_t command[50];
    static uint8_t cpointer = 0;
    static uint8_t keycode = 0;
    char character;
    /** demo mouse speed */
    #define MOUSE_SPEED 30
    
    //Install UART driver, and get the queue.
    uart_driver_install(CONFIG_CONSOLE_UART_NUM, UART_FIFO_LEN * 2, UART_FIFO_LEN * 2, 0, NULL, 0);
    
    
    while(1)
    {
        // read single byte
        uart_read_bytes(CONFIG_CONSOLE_UART_NUM, (uint8_t*) &character, 1, portMAX_DELAY);
		
        //sum up characters to one \n terminated command and send it to
        //UART parser
        if(character == '\n' || character == '\r')
        {
            printf("received enter, forward command to UART parser\n");
            command[cpointer] = 0x00;
            process_uart(command, cpointer);
            cpointer = 0;
        } else {
            if(cpointer < 50)
            {
                command[cpointer] = character;
                cpointer++;
            }
        }

        if (!sec_conn) {
            printf("Not connected, ignoring '%c'\n", character);
        } else {
            switch (character){
                case 'a':
                    esp_hidd_send_mouse_value(hid_conn_id,0,-MOUSE_SPEED,0,0);
                    break;
                case 's':
                    esp_hidd_send_mouse_value(hid_conn_id,0,0,MOUSE_SPEED,0);
                    break;
                case 'd':
                    esp_hidd_send_mouse_value(hid_conn_id,0,MOUSE_SPEED,0,0);
                    break;
                case 'w':
                    esp_hidd_send_mouse_value(hid_conn_id,0,0,-MOUSE_SPEED,0);
                    break;
                case 'l':
                    esp_hidd_send_mouse_value(hid_conn_id,0x01,0,0,0);
                    esp_hidd_send_mouse_value(hid_conn_id,0x00,0,0,0);
                    break;
                case 'r':
                    esp_hidd_send_mouse_value(hid_conn_id,0x02,0,0,0);
                    esp_hidd_send_mouse_value(hid_conn_id,0x00,0,0,0);
                    break;
                case 'y':
                case 'z':
                    printf("Received: %d\n",character);
                    break;
                case 'Q':
                    //send only lower characters
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    keycode = 28;
                    esp_hidd_send_keyboard_value(hid_conn_id,0,&keycode,1);
                    keycode = 0;
                    esp_hidd_send_keyboard_value(hid_conn_id,0,&keycode,1);
                    break;
            }
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}


void app_main()
{
    esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    
    // Read config
    nvs_handle my_handle;
    ret = nvs_open("fabi_c", NVS_READWRITE, &my_handle);
    if(ret != ESP_OK) ESP_LOGE("MAIN","error opening NVS");
    ret = nvs_get_u8(my_handle, "btname_i", &config.bt_device_name_index);
    if(ret != ESP_OK) 
    {
        ESP_LOGE("MAIN","error reading NVS - bt name, setting to FABI");
        config.bt_device_name_index = 0;
    }
    ret = nvs_get_u8(my_handle, "locale", &config.locale);
    if(ret != ESP_OK || config.locale >= LAYOUT_MAX) 
    {
        ESP_LOGE("MAIN","error reading NVS - locale, setting to US_INTERNATIONAL");
        config.locale = LAYOUT_US_INTERNATIONAL;
    }
    nvs_close(my_handle);
    

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed\n", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TAG,"%s init bluedroid failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG,"%s init bluedroid failed\n", __func__);
        return;
    }
    
    //load HID country code for locale before initialising HID
    hidd_set_countrycode(get_hid_country_code(config.locale));

    if((ret = esp_hidd_profile_init()) != ESP_OK) {
        ESP_LOGE(GATTS_TAG,"%s init bluedroid failed\n", __func__);
    }

    ///register the callback function to the gap module
    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;     //bonding with peer device after authentication
    /** Do not use "NONE", HID over GATT requires something more than NONE */
    //esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    /** CAP_OUT & CAP_IO work with Winsh***t, but you need to enter a pin which is shown in "make monitor" */
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;           //set the IO capability to No output No input
    //esp_ble_io_cap_t iocap = ESP_IO_CAP_IO;           //set the IO capability to No output No input
    /** CAP_IN: host shows you a pin, you have to enter it (unimplemented now) */
    //esp_ble_io_cap_t iocap = ESP_IO_CAP_IN;           //set the IO capability to No output No input
    
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

    //init the gpio pin (not needing GPIOs by now...)
    //gpio_demo_init();
    
    xTaskCreate(&uart_stdin, "stdin", 2048, NULL, 5, NULL);

}

