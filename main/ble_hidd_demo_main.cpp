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
 * Copyright 2017 Benjamin Aigner <beni@asterics-foundation.org>
 */


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
#include "HID_kbdmousejoystick.h"

//#include "esp_hidd_prf_api.h"
/*#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"*/
#include "driver/gpio.h"
#include "driver/uart.h"
//#include "hid_dev.h"
#include "keyboard.h"

#define GATTS_TAG "FABI/FLIPMOUSE"

static uint8_t keycode_modifier;
static uint8_t keycode_deadkey_first;
//static joystick_data_t joystick;//currently unused, no joystick implemented
static config_data_t config;
mouse_command_t mouseCmd;
keyboard_command_t keyboardCmd;
joystick_command_t joystickCmd;


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
    uint8_t keycode;
    const char nl = '\r';
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
        /*
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
        } else ESP_LOGE(DEBUG_TAG,"error getting bonded devices count or no devices bonded");*/
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
        keyboardCmd.type = RELEASE_ALL;
        xQueueSend(keyboard_q,(void *)&keyboardCmd, (TickType_t) 0);
        ESP_LOGD(DEBUG_TAG,"keyboard: release all (KR)");
        return;
    }
    
    /**++++ commands with parameters ++++*/
    
    switch(input[0])
    {
        //Raw HID mode, API compatibility for EZKey
        case 0xFD:
            //2nd byte is modifier or 0 for mouse
            //3rd byte is used to determine the interface
            switch(input[2])
            {
                //keyboard
                case 0x00:
                    uint8_t k[8];
                    //assignement is based on FLipMouse/FlipWare/bluetooth.cpp
                    k[0] = input[1];
                    k[1] = 0;
                    k[2] = input[3];
                    k[3] = input[4];
                    k[4] = input[5];
                    k[5] = input[6];
                    k[6] = input[7];
                    k[7] = input[8];
                    if(HID_kbdmousejoystick_rawKeyboard(k,sizeof(k)) != ESP_OK)
                    {
                        ESP_LOGE(DEBUG_TAG,"Error sending raw kbd");
                    } else ESP_LOGI(DEBUG_TAG,"Keyboard sent");
                    return;
                
                //mouse
                case 0x03:
                    uint8_t a[4];
                    //assignement is based on FLipMouse/FlipWare/bluetooth.cpp
                    a[0] = input[3];
                    a[1] = input[4];
                    a[2] = input[5];
                    a[3] = input[6];
                    if(HID_kbdmousejoystick_rawMouse(a,sizeof(a)) != ESP_OK)
                    {
                        ESP_LOGE(DEBUG_TAG,"Error sending raw mouse");
                    } else ESP_LOGI(DEBUG_TAG,"Mouse sent");
                    return;
                default: ESP_LOGE(DEBUG_TAG,"Unknown RAW HID packet");
            }
            break;
        case 'K': //keyboard
            //key up
            if(input[1] == 'U' && len == 3) 
            {
                keyboardCmd.type = RELEASE;
                keyboardCmd.keycode = input[2];
                xQueueSend(keyboard_q,(void *)&keyboardCmd, (TickType_t) 0);
                return;
            }
            //key down
            if(input[1] == 'D' && len == 3) 
            {
                keyboardCmd.type = PRESS;
                keyboardCmd.keycode = input[2];
                xQueueSend(keyboard_q,(void *)&keyboardCmd, (TickType_t) 0);
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
                    keyboardCmd.type = PRESS_RELEASE;
                    keyboardCmd.keycode = keycode;
                    xQueueSend(keyboard_q,(void *)&keyboardCmd, (TickType_t) 0);
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
        mouseCmd.buttons = input[1];
        mouseCmd.x = input[2];
        mouseCmd.y = input[3];
        mouseCmd.wheel = input[4];
        xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
        ESP_LOGD(DEBUG_TAG,"mouse movement");
        return;
    }
    //BT: delete one pairing
    if(input[0] == 'D' && input[1] == 'P' && len == 3)
    {
        /*counter = esp_ble_get_bond_device_num();
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
        } else ESP_LOGE(DEBUG_TAG,"error getting bonded devices count");*/
        ESP_LOGE(DEBUG_TAG,"TBD: delete pairing");
        return;
    }
    //BT: enable/disable discoverable/pairing
    if(input[0] == 'P' && input[1] == 'M' && len == 3)
    {
        if(input[2] == 0 || input[2] == '0')
        {
            ESP_LOGE(DEBUG_TAG,"TBD: stopping advertising");
        } else if(input[2] == 1 || input[2] == '1') {
            if(HID_kbdmousejoystick_activatePairing() != ESP_OK)
            {
                ESP_LOGE(DEBUG_TAG,"error starting advertising");
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
    if(CONSOLE_UART_NUM != UART_NUM_0)
    {
        esp_err_t ret = ESP_OK;
        const uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
        };
        
        //update UART config
        ret = uart_param_config(CONSOLE_UART_NUM, &uart_config);
        if(ret != ESP_OK) 
        {
            ESP_LOGE("UART","UART param config failed"); 
        }
        
        //set IO pins
        ret = uart_set_pin(CONSOLE_UART_NUM, EX_SERIAL_TXPIN, EX_SERIAL_RXPIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if(ret != ESP_OK)
        {
            ESP_LOGE("UART","UART set pin failed"); 
        }
    }
    uart_driver_install(CONSOLE_UART_NUM, UART_FIFO_LEN * 2, UART_FIFO_LEN * 2, 0, NULL, 0);
    
    ESP_LOGI("UART","UART processing task started");
    
    while(1)
    {
        // read single byte
        uart_read_bytes(CONSOLE_UART_NUM, (uint8_t*) &character, 1, portMAX_DELAY);
		
        //sum up characters to one \n terminated command and send it to
        //UART parser
        if(character == '\n' || character == '\r')
        {
            ESP_LOGI("UART","received enter, forward command to UART parser");
            command[cpointer] = 0x00;
            process_uart(command, cpointer);
            cpointer = 0;
        } else {
            if(cpointer < 50)
            {
                command[cpointer] = character;
                cpointer++;
            }
            
            //do some "pre-parsing" to detect EZKey commands, which
            //do NOT have line-ending characters
            if(command[0] == 0xFD && cpointer == 9)
            {
                process_uart(command, cpointer);
                cpointer = 0;
            }
            
        }

        if(HID_kbdmousejoystick_isConnected() == 0) {
            ESP_LOGI("UART","Not connected, ignoring '%c'", character);
        } else {
            //Do not send anything if queues are uninitialized
            if(mouse_q == NULL || keyboard_q == NULL || joystick_q == NULL)
            {
                ESP_LOGE("UART","queues not initialized");
                continue;
            }
            switch (character){
                case 'a':
                    mouseCmd.x = -MOUSE_SPEED;
                    mouseCmd.y = 0;
                    mouseCmd.buttons = 0;
                    mouseCmd.wheel = 0;
                    xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
                    ESP_LOGI("UART","mouse: a");
                    break;
                case 's':
                    mouseCmd.x = 0;
                    mouseCmd.y = MOUSE_SPEED;
                    mouseCmd.buttons = 0;
                    mouseCmd.wheel = 0;
                    xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
                    ESP_LOGI("UART","mouse: s");
                    break;
                case 'd':
                    mouseCmd.x = MOUSE_SPEED;
                    mouseCmd.y = 0;
                    mouseCmd.buttons = 0;
                    mouseCmd.wheel = 0;
                    xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
                    ESP_LOGI("UART","mouse: d");
                    break;
                case 'w':
                    mouseCmd.x = 0;
                    mouseCmd.y = -MOUSE_SPEED;
                    mouseCmd.buttons = 0;
                    mouseCmd.wheel = 0;
                    xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
                    ESP_LOGI("UART","mouse: w");
                    break;
                case 'l':
                    mouseCmd.x = 0;
                    mouseCmd.y = 0;
                    mouseCmd.buttons = 1;
                    mouseCmd.wheel = 0;
                    xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
                    mouseCmd.x = 0;
                    mouseCmd.y = 0;
                    mouseCmd.buttons = 0;
                    mouseCmd.wheel = 0;
                    xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
                    ESP_LOGI("UART","mouse: l");
                    break;
                case 'r':
                    mouseCmd.x = 0;
                    mouseCmd.y = 0;
                    mouseCmd.buttons = 2;
                    mouseCmd.wheel = 0;
                    xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
                    mouseCmd.x = 0;
                    mouseCmd.y = 0;
                    mouseCmd.buttons = 0;
                    mouseCmd.wheel = 0;
                    xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
                    ESP_LOGI("UART","mouse: r");
                    break;
                case 'y':
                case 'z':
                    ESP_LOGI("UART","Received: %d",character);
                    break;
                case 'Q':
                    //send only lower characters
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    keyboardCmd.keycode = 28;
                    keyboardCmd.type = PRESS_RELEASE;
                    xQueueSend(keyboard_q,(void *)&keyboardCmd, (TickType_t) 0);
                    ESP_LOGI("UART","keyboard: Q");
                    break;
            }
        }

        //vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

extern "C" void app_main()
{
    esp_err_t ret;

    //activate mouse & keyboard normally (not in testmode)
    //joystick is not working yet.
    HID_kbdmousejoystick_init(1,1,0,0);
    ESP_LOGI("HIDD","MAIN finished...");
    
    esp_log_level_set("*", ESP_LOG_INFO); 

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

    
    //load HID country code for locale before initialising HID
    ///@todo Apply country code
    //hidd_set_countrycode(get_hid_country_code(config.locale));


    xTaskCreate(&uart_stdin, "stdin", 2048, NULL, configMAX_PRIORITIES, NULL);
}

