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
#include "driver/gpio.h"

#include "config.h"
#include "HID_kbdmousejoystick.h"

#include "esp_gap_ble_api.h"
//#include "esp_hidd_prf_api.h"
/*#include "esp_bt_defs.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"*/
#include "driver/gpio.h"
#include "driver/uart.h"
//#include "hid_dev.h"
#include "keyboard.h"

/** demo mouse speed */
#define MOUSE_SPEED 30
#define MAX_CMDLEN  100

#define EXT_UART_TAG "EXT_UART"
#define CONSOLE_UART_TAG "CONSOLE_UART"


static uint8_t keycode_modifier=0;
static uint8_t keycode_deadkey_first=0;
//static joystick_data_t joystick;//currently unused, no joystick implemented
static config_data_t config;
// joystick_command_t joystickCmd;


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


void sendKeyCode(uint8_t c, keyboard_action type) {
	keyboard_command_t keyboardCmd;
	keyboardCmd.type = type;
	keyboardCmd.keycode = c + ((uint16_t)keycode_modifier << 8);
	xQueueSend(keyboard_q,(void *)&keyboardCmd, (TickType_t) 0);
}

void sendKey(uint8_t c, keyboard_action type) {
    uint8_t keycode;
	keycode = parse_for_keycode(c,config.locale,&keycode_modifier,&keycode_deadkey_first); //send current byte to parser
	if(keycode == 0) 
	{
		ESP_LOGI(EXT_UART_TAG,"keycode is 0 for 0x%X, skipping to next byte",c);
		return; //if no keycode is found,skip to next byte (might be a 16bit UTF8)
	}
	ESP_LOGI(EXT_UART_TAG,"keycode: %d, modifier: %d, deadkey: %d",keycode,keycode_modifier,keycode_deadkey_first);
	//TODO: do deadkey sequence...
		//if a keycode is found, add to keycodes for HID
	sendKeyCode(keycode, type);
}





#define CMDSTATE_IDLE 0
#define CMDSTATE_GET_RAW 1
#define CMDSTATE_GET_ASCII 2

struct cmdBuf {
    int state;
	int expectedBytes;
	int bufferLength;
	uint8_t buf[MAX_CMDLEN];
} ;

uint8_t uppercase(uint8_t c) 
{
	if ((c>='a') && (c<='z')) return (c-'a'+'A');
	return(c);
}

int get_int(const char * input, int index, int * value) 
{
  int sign=1, result=0, valid=0;

  while (input[index]==' ') index++;   // skip leading spaces
  if (input[index]=='-') { sign=-1; index++;}
  while ((input[index]>='0') && (input[index]<='9'))
  {
	  result= result*10+input[index]-'0';
	  valid=1;
	  index++;
  }
  while (input[index]==' ') index++;  // skip trailing spaces
  if (input[index]==',') index++;     // or a comma
  
  if (valid) { *value = result*sign; return (index);}
  return(0);  
}


void processCommand(struct cmdBuf *cmdBuffer)
{
    //commands:
    // $ID
    // $PMx (0 or 1)
    // $GP
    // $DPx (number of paired device, starting with 0)
    // $KW (+layouts)
    // $KD/KU
    // $KR
    // $KL
    // $M
    // $KC
    
    // TBD:
    // $KK (if necessary)
    // joystick (everything)
    
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
                    for(uint8_t i = 0; i<counter;i++)
                    {
                        //print on monitor & external uart
                        esp_log_buffer_hex(EXT_UART_TAG, btdevlist[i].bd_addr, sizeof(esp_bd_addr_t));
                        for (int t=0; t<sizeof(esp_bd_addr_t);t++) {
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
    
    //joystick: update data (send a report)
    if(strcmp(input,"JU") == 0) 
    {
        //TBD: joystick
        ESP_LOGD(EXT_UART_TAG,"TBD! joystick: send report (JU)");
        return;
    }
    
    //keyboard: release all
    if(strcmp(input,"KR") == 0)
    {
		sendKeyCode(0, RELEASE_ALL);
        ESP_LOGD(EXT_UART_TAG,"keyboard: release all (KR)");
        return;
    }
    
    /**++++ commands with parameters ++++*/    
    switch(input[0])
    {
        case 'K': //keyboard
        {
			int param=0;
			if ((input[1] == 'U') || (input[1] == 'D') || (input[1] == 'L'))
			{
				if (!get_int(input,2,&param)) {
					ESP_LOGE(EXT_UART_TAG,"Keyboad command parameter needs integer format");
					break;
				}
			}

            //key up
            if(input[1] == 'U')
            {
                sendKeyCode(param, RELEASE);
                return;
            }
            //key down
            if(input[1] == 'D') 
            {
                sendKeyCode(param, PRESS);
                return;
            }
            //keyboard, set locale
            if(input[1] == 'L') 
            { 
				if(param < LAYOUT_MAX)   {
					config.locale = param;
					update_config();
				} else ESP_LOGE(EXT_UART_TAG,"Locale number out of range");
                return;
            }
            
            //keyboard, write
            if(input[1] == 'W')
            {
                ESP_LOGI(EXT_UART_TAG,"sending keyboard write, len: %d (bytes, not characters!)",len-2);
                for(uint16_t i = 2; i<len; i++)
                {
                    if(input[i] == '\0') 
                    {
                        ESP_LOGI(EXT_UART_TAG,"terminated string, ending KW");
                        break;
                    }
                    sendKey(input[i],PRESS_RELEASE);
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
		}
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
            ESP_LOGD(EXT_UART_TAG,"TBD! joystick");
            break;
        
        default: //test for management commands
            break;
    }

    // M <buttons> <X> <Y> <wheel>
    // create mouse activity
    if(input[0] == 'M')
    {
		int index=1, value;
		mouse_command_t mouseCmd;

		if ((index=get_int(input,index,&value))) mouseCmd.buttons = value; else return;
		if ((index=get_int(input,index,&value))) mouseCmd.x = value; else return;
		if ((index=get_int(input,index,&value))) mouseCmd.y = value; else return;
		if ((index=get_int(input,index,&value))) mouseCmd.wheel = value; else return;

        xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
        ESP_LOGI(EXT_UART_TAG,"mouse command: %d,%d,%d,%d",mouseCmd.buttons,mouseCmd.x,mouseCmd.y,mouseCmd.wheel);
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
                } else ESP_LOGE(EXT_UART_TAG,"error getting device list");
                free (btdevlist);
            } else ESP_LOGE(EXT_UART_TAG,"error allocating memory for device list");
        } else ESP_LOGE(EXT_UART_TAG,"error getting bonded devices count");
        ESP_LOGE(EXT_UART_TAG,"TBD: delete pairing");
        return;
    }

    //PM: enable/disable advertisting/pairing
    if(input[0] == 'P' && input[1] == 'M')
    {
		int param;
		if (!(get_int(input,2,&param))) { 
			ESP_LOGE(EXT_UART_TAG,"PM: invalid parameter, need integer");
			return;
		}
		
        if(param == 0)
        {
            HID_kbdmousejoystick_deactivatePairing();
            ESP_LOGI(EXT_UART_TAG,"PM: pairing deactivated");

        } else if(param == 1) 
        {
            HID_kbdmousejoystick_activatePairing();
            ESP_LOGI(EXT_UART_TAG,"PM: pairing activated");

        } else ESP_LOGE(EXT_UART_TAG,"PM: parameter error, either 0 or 1");
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
    static uint8_t keycode = 0;
    
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
				else sendKey(character, PRESS_RELEASE);         // treat whatever comes in as keyboard output
				break;
							
		case CMDSTATE_GET_RAW:
				cmdBuffer->buf[cmdBuffer->bufferLength]=character;
				//if ((cmdBuffer->bufferLength == 1) && (character==0x00))  // we have a keyboard report: increase by 2 bytes
				  // cmdBuffer->expectedBytes += 2;

				cmdBuffer->bufferLength++;
				cmdBuffer->expectedBytes--;
				if (!cmdBuffer->expectedBytes) {
					if (cmdBuffer->buf[1] == 0x00) {   // keyboard report				
						// TBD: synchonize with semaphore!	
						if(HID_kbdmousejoystick_rawKeyboard(cmdBuffer->buf,8) != ESP_OK)
						{
							ESP_LOGE(EXT_UART_TAG,"Error sending raw kbd");
						} else ESP_LOGI(EXT_UART_TAG,"Keyboard sent");
					} else if (cmdBuffer->buf[1] == 0x03) {  // mouse report
						// TBD: synchonize with semaphore!	
						if(HID_kbdmousejoystick_rawMouse(&(cmdBuffer->buf[2]),4) != ESP_OK)
						{
							ESP_LOGE(EXT_UART_TAG,"Error sending raw mouse");
						} else ESP_LOGI(EXT_UART_TAG,"Mouse sent");

					}
                    else ESP_LOGE(EXT_UART_TAG,"Unknown RAW HID packet");
                    cmdBuffer->state=CMDSTATE_IDLE;
				}
				break;
				
		case CMDSTATE_GET_ASCII:
				if (character=='$')  {   //  key '$' can be created by sending "$$" 
					sendKey('$', PRESS_RELEASE);
					cmdBuffer->state=CMDSTATE_IDLE;
				} else {   // collect a command string until CR or LF are received
					if ((character==0x0d) || (character==0x0a))  {
						cmdBuffer->buf[cmdBuffer->bufferLength]=0;
						ESP_LOGI(EXT_UART_TAG,"sending command to parser: %s",cmdBuffer->buf);

						processCommand(cmdBuffer);
						cmdBuffer->state=CMDSTATE_IDLE;
					} else {
						if (cmdBuffer->bufferLength < MAX_CMDLEN-1) 
						  cmdBuffer->buf[cmdBuffer->bufferLength++]=character;
					}
				}
				break;
		default: 
 			cmdBuffer->state=CMDSTATE_IDLE;
	}
} 
					


void uart_console(void *pvParameters)
{
    char character;
	mouse_command_t mouseCmd;
	keyboard_command_t keyboardCmd;
    
    //Install UART driver, and get the queue.
    uart_driver_install(CONSOLE_UART_NUM, UART_FIFO_LEN * 2, UART_FIFO_LEN * 2, 0, NULL, 0);

    ESP_LOGI("UART","console UART processing task started");
    
    while(1)
    {
        // read single byte
        uart_read_bytes(CONSOLE_UART_NUM, (uint8_t*) &character, 1, portMAX_DELAY);
		// uart_parse_command(character, &cmdBuffer);	      	

		if(HID_kbdmousejoystick_isConnected() == 0) {
			ESP_LOGI(CONSOLE_UART_TAG,"Not connected, ignoring '%c'", character);
		} else {
			//Do not send anything if queues are uninitialized
			if(mouse_q == NULL || keyboard_q == NULL || joystick_q == NULL)
			{
				ESP_LOGE(CONSOLE_UART_TAG,"queues not initialized");
				continue;
			}
			switch (character){
				case 'a':
					mouseCmd.x = -MOUSE_SPEED;
					mouseCmd.y = 0;
					mouseCmd.buttons = 0;
					mouseCmd.wheel = 0;
					xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
					ESP_LOGI(CONSOLE_UART_TAG,"mouse: a");
					break;
				case 's':
					mouseCmd.x = 0;
					mouseCmd.y = MOUSE_SPEED;
					mouseCmd.buttons = 0;
					mouseCmd.wheel = 0;
					xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
					ESP_LOGI(CONSOLE_UART_TAG,"mouse: s");
					break;
				case 'd':
					mouseCmd.x = MOUSE_SPEED;
					mouseCmd.y = 0;
					mouseCmd.buttons = 0;
					mouseCmd.wheel = 0;
					xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
					ESP_LOGI(CONSOLE_UART_TAG,"mouse: d");
					break;
				case 'w':
					mouseCmd.x = 0;
					mouseCmd.y = -MOUSE_SPEED;
					mouseCmd.buttons = 0;
					mouseCmd.wheel = 0;
					xQueueSend(mouse_q,(void *)&mouseCmd, (TickType_t) 0);
					ESP_LOGI(CONSOLE_UART_TAG,"mouse: w");
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
					ESP_LOGI(CONSOLE_UART_TAG,"mouse: l");
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
					ESP_LOGI(CONSOLE_UART_TAG,"mouse: r");
					break;
				case 'q':
					ESP_LOGI(CONSOLE_UART_TAG,"received q: sending key y for test purposes");
					keyboardCmd.keycode = 28;
					keyboardCmd.type = PRESS_RELEASE;
					xQueueSend(keyboard_q,(void *)&keyboardCmd, (TickType_t) 0);
					break;
				default:
					ESP_LOGI(CONSOLE_UART_TAG,"received: %d",character);
					break;
			}
		}
    }
}


void uart_external(void *pvParameters)
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
        // read single byte
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
		
		if (HID_kbdmousejoystick_isConnected()) 
			blinkTime=1000;
		else blinkTime=250;
		
		
        /* Blink off (output low) */
        gpio_set_level(INDICATOR_LED_PIN, 0);
        vTaskDelay(blinkTime / portTICK_PERIOD_MS);
        /* Blink on (output high) */
        gpio_set_level(INDICATOR_LED_PIN, 1);
        vTaskDelay(blinkTime / portTICK_PERIOD_MS);
    }
}

extern "C" void app_main()
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
	ESP_LOGI("MAIN","loading configuration from NVS");
    ret = nvs_open("config_c", NVS_READWRITE, &my_handle);
    if(ret != ESP_OK) ESP_LOGE("MAIN","error opening NVS");
    size_t available_size = MAX_BT_DEVICENAME_LENGTH;
    strcpy(config.bt_device_name, GATTS_TAG);
    nvs_get_str (my_handle, "btname", config.bt_device_name, &available_size);
    if(ret != ESP_OK) 
    {
        ESP_LOGE("MAIN","error reading NVS - bt name, setting to default");
        strcpy(config.bt_device_name, GATTS_TAG);
    } else ESP_LOGI("MAIN","bt device name is: %s",config.bt_device_name);

    ret = nvs_get_u8(my_handle, "locale", &config.locale);
    if(ret != ESP_OK || config.locale >= LAYOUT_MAX) 
    {
        ESP_LOGE("MAIN","error reading NVS - locale, setting to US_INTERNATIONAL");
        config.locale = LAYOUT_US_INTERNATIONAL;
    } else ESP_LOGI("MAIN","locale code is : %d",config.locale);
    nvs_close(my_handle);

    // TBD: apply country code
    // load HID country code for locale before initialising HID
    // hidd_set_countrycode(get_hid_country_code(config.locale));


    //activate mouse & keyboard BT stack (joystick is not working yet)
    HID_kbdmousejoystick_init(1,1,0,0,config.bt_device_name);
    ESP_LOGI("HIDD","MAIN finished...");
    
    esp_log_level_set("*", ESP_LOG_INFO); 

  
    // now start the tasks for processing UART input and indicator LED  
 
    xTaskCreate(&uart_console,  "console", 4096, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(&uart_external, "external", 4096, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(&blink_task, "blink", 4096, NULL, configMAX_PRIORITIES, NULL);
    
}
