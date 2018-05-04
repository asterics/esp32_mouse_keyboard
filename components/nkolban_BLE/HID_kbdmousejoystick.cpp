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
 * 
 * Mainly based on SampleHIDDevice.cpp by Neil Kolban.
 */

/** @file
 * @brief This file is the implementation for Neil Kolbans CPP utils
 * 
 * It initializes 3 queues for sending mouse, keyboard and joystick data
 * from within the C-side. C++ classes are instantiated here.
 * If you want to have a different BLE HID device, you need to adapt this file.
 * 
 * @note Thank you very much Neil Kolban for this impressive work!
*/

#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "SampleKeyboardTypes.h"
#include <esp_log.h>
#include <string>
#include <Task.h>
#include "HID_kbdmousejoystick.h"

#include "sdkconfig.h"

static char LOG_TAG[] = "HAL_BLE";

QueueHandle_t joystick_q;
QueueHandle_t keyboard_q;
QueueHandle_t mouse_q;

static BLEHIDDevice* hid;

class KeyboardTask : public Task {
	void run(void*){
    vTaskDelay(2000/portTICK_PERIOD_MS);
    	const char* hello = "Hello world from esp32 hid keyboard!!!";
		while(*hello){
			KEYMAP map = keymap[(uint8_t)*hello];
			uint8_t a[] = {map.modifier, 0x0, map.usage, 0x0,0x0,0x0,0x0,0x0};
      BLECharacteristic* test = hid->inputReport(NULL);
      test->setValue(a,sizeof(a));
      //test->executeCreate(hid->hidService());
      test->notify();
			//hid->inputReport(NULL)->setValue(a,sizeof(a));
			//hid->inputReport(NULL)->notify();

			hello++;
		}
			uint8_t v[] = {0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0};
			hid->inputReport(NULL)->setValue(v, sizeof(v));
			hid->inputReport(NULL)->notify();
		vTaskDelete(NULL);
	}
};
class MouseTask : public Task {
	void run(void*){
    uint8_t step = 0;
    //<report id = 2>, <button>, <x>, <y>
    uint8_t a[] = {0x00,(uint8_t) -0, (uint8_t) -0};
    vTaskDelay(5000/portTICK_PERIOD_MS);
		while(1){
      //disabled until at least kbd works...
      vTaskDelay(500000/portTICK_PERIOD_MS);
      switch(step) {
        case 0:
          a[0] = 0;
          a[1] = 10;
          step++;
          break;
        case 1:
          a[1] = -10;
          step++;
          break;
        case 2:
          a[2] = 10;
          step++;
          break;
        case 3:
          step++;
          a[2] = -10;
          break;
        default:
          a[0] = (1<<0);
          step = 0;
          break;
      }
      ESP_LOGI(LOG_TAG,"Sending mouse report: ");
      ESP_LOG_BUFFER_HEXDUMP(LOG_TAG,a,sizeof(a),ESP_LOG_INFO);
			hid->inputReport(NULL)->setValue(a,sizeof(a));
			hid->inputReport(NULL)->notify();
      vTaskDelay(2000/portTICK_PERIOD_MS);
		}
    //finally: send an empty report
    uint8_t v[] = {0x0,(uint8_t) -0, (uint8_t) -0};
    hid->inputReport(NULL)->setValue(v, sizeof(v));
		hid->inputReport(NULL)->notify();
		vTaskDelete(NULL);
	}
};
class JoystickTask : public Task {
	void run(void*){
    vTaskDelay(1000/portTICK_PERIOD_MS);
		while(1){
        vTaskDelay(10000/portTICK_PERIOD_MS);
		}
			/*uint8_t v[] = {0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0};
			hid->inputReport(NULL)->setValue(v, sizeof(v));
			hid->inputReport(NULL)->notify();*/
		vTaskDelete(NULL);
	}
};

KeyboardTask *kbd;
MouseTask *mouse;
JoystickTask *joystick;
BLEServer *pServer;

class CBs: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
   	kbd->start();
   	mouse->start();
   	joystick->start();
  }

  void onDisconnect(BLEServer* pServer){
    kbd->stop();
    mouse->stop();
    joystick->stop();
  }
};

uint32_t passKey = 0;
class CB_Security: public BLESecurityCallbacks {
  /*~CB_Security() {
    ESP_LOGE(LOG_TAG,"Sec dtor");
  }*/
  
  uint32_t onPassKeyRequest(){
    ESP_LOGE(LOG_TAG, "The passkey request %d", passKey);
    vTaskDelay(25000);
    return passKey;
  }
  
  void onPassKeyNotify(uint32_t pass_key){
    ESP_LOGE(LOG_TAG, "The passkey Notify number:%d", pass_key);
    passKey = pass_key;
  }
  
  bool onSecurityRequest(){
    return true;
  }

  /*CB_Security() {
    ESP_LOGE(LOG_TAG,"Sec ctor");
  }*/
  
  void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl){
    if(auth_cmpl.success){
      ESP_LOGI(LOG_TAG, "remote BD_ADDR:");
      esp_log_buffer_hex(LOG_TAG, auth_cmpl.bd_addr, sizeof(auth_cmpl.bd_addr));
      ESP_LOGI(LOG_TAG, "address type = %d", auth_cmpl.addr_type);
    }
      ESP_LOGI(LOG_TAG, "pair status = %s", auth_cmpl.success ? "success" : "fail");
  }
  
  bool onConfirmPIN(uint32_t pin)
  {
    ESP_LOGE(LOG_TAG, "Confirm pin: %d", pin);
    return true;
  }
  
};


class BLE_HOG: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "Initialising BLE HID device.");

		kbd = new KeyboardTask();
		kbd->setStackSize(8096);
		mouse = new MouseTask();
		mouse->setStackSize(8096);
		joystick = new JoystickTask();
		joystick->setStackSize(8096);
    
		BLEDevice::init("FABI/FLipMouse");
		pServer = BLEDevice::createServer();
		pServer->setCallbacks(new CBs());
		BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);
		BLEDevice::setSecurityCallbacks(new CB_Security());

		/*
		 * Instantiate hid device
		 */
		hid = new BLEHIDDevice(pServer);

		/*
		 * Set manufacturer name
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.manufacturer_name_string.xml
		 */
		std::string name = "AsTeRICS Foundation";
		hid->manufacturer()->setValue(name);

		/*
		 * Set pnp parameters
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.pnp_id.xml
		 */
		const uint8_t pnp[] = {0x01,0x02,0xe5,0xab,0xcd,0x01,0x10};
    //hid->pnp(0x01,0x02E5,0xABCD,0x0110);
		//hid->pnp()->setValue((uint8_t*)pnp, sizeof(pnp));

		/*
		 * Set hid informations
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hid_information.xml
		 */
		//const uint8_t val1[] = {0x01,0x11,0x00,0x03};
		//hid->hidInfo()->setValue((uint8_t*)val1, 4);
    hid->hidInfo(0x00,0x03);

		/*
		 * Mouse
		 */
		const uint8_t reportMap2[] = {
			USAGE_PAGE(1), 			0x01,
			USAGE(1), 				0x02,
			 COLLECTION(1),			0x01,
			 USAGE(1),				0x01,
			 COLLECTION(1),			0x00,
			 USAGE_PAGE(1),			0x09,
			 USAGE_MINIMUM(1),		0x1,
			 USAGE_MAXIMUM(1),		0x3,
			 LOGICAL_MINIMUM(1),	0x0,
			 LOGICAL_MAXIMUM(1),	0x1,
			 REPORT_COUNT(1),		0x3,
			 REPORT_SIZE(1),		0x1,
			 INPUT(1), 				0x2,		// (Data, Variable, Absolute), ;3 button bits
			 REPORT_COUNT(1),		0x1,
			 REPORT_SIZE(1),		0x5,
			 INPUT(1), 				0x1,		//(Constant), ;5 bit padding
			 USAGE_PAGE(1), 		0x1,		//(Generic Desktop),
			 USAGE(1),				0x30,
			 USAGE(1),				0x31,
			 LOGICAL_MINIMUM(1),	0x81,
			 LOGICAL_MAXIMUM(1),	0x7f,
			 REPORT_SIZE(1),		0x8,
			 REPORT_COUNT(1),		0x2,
			 INPUT(1), 				0x6,		//(Data, Variable, Relative), ;2 position bytes (X & Y)
			 END_COLLECTION(0),
			END_COLLECTION(0)
		};
    
    const uint8_t reportMapKbd[] = {
			USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
			USAGE(1),           0x06,       // Keyboard
			COLLECTION(1),      0x01,       // Application
			USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
			USAGE_MINIMUM(1),   0xE0,
			USAGE_MAXIMUM(1),   0xE7,
			LOGICAL_MINIMUM(1), 0x00,
			LOGICAL_MAXIMUM(1), 0x01,
			REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
			REPORT_COUNT(1),    0x08,
			INPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
			REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
			REPORT_SIZE(1),     0x08,
			INPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
			REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
			REPORT_SIZE(1),     0x01,
			USAGE_PAGE(1),      0x08,       //   LEDs
			USAGE_MINIMUM(1),   0x01,       //   Num Lock
			USAGE_MAXIMUM(1),   0x05,       //   Kana
			OUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
			REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
			REPORT_SIZE(1),     0x03,
			OUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
			REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
			REPORT_SIZE(1),     0x08,
			LOGICAL_MINIMUM(1), 0x00,
			LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
			USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
			USAGE_MINIMUM(1),   0x00,
			USAGE_MAXIMUM(1),   0x65,
			INPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
			END_COLLECTION(0)
		};
		/*
		 * Keyboard + mouse (+ joystick [tbd] )
		 */
		const uint8_t reportMap[] = {
      /*++++ Keyboard - report id 1 ++++*/
			USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
			USAGE(1),           0x06,       // Keyboard
			COLLECTION(1),      0x01,       // Application
        REPORT_ID(1),       0x01,
        REPORT_COUNT(1),    0x08,
        REPORT_SIZE(1),     0x01,
        USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
        USAGE_MINIMUM(1),   0xE0,
        USAGE_MAXIMUM(1),   0xE7,
        LOGICAL_MINIMUM(1), 0x00,
        LOGICAL_MAXIMUM(1), 0x01,
        REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
        REPORT_COUNT(1),    0x08,
        INPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
        REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
        REPORT_SIZE(1),     0x08,
        INPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
        REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
        REPORT_SIZE(1),     0x01,
        USAGE_PAGE(1),      0x08,       //   LEDs
        USAGE_MINIMUM(1),   0x01,       //   Num Lock
        USAGE_MAXIMUM(1),   0x05,       //   Kana
        OUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
        REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
        REPORT_SIZE(1),     0x03,
        OUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
        REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
        REPORT_SIZE(1),     0x08,
        LOGICAL_MINIMUM(1), 0x00,
        LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
        USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
        USAGE_MINIMUM(1),   0x00,
        USAGE_MAXIMUM(1),   0x65,
        INPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
			END_COLLECTION(0),
      /*++++ Mouse - report id 2 ++++*/
      USAGE_PAGE(1), 			0x01,
			USAGE(1), 				  0x02,
      COLLECTION(1),			0x01,
        REPORT_ID(1),       0x02,
        REPORT_COUNT(1),    0x08,
        REPORT_SIZE(1),     0x01,
        USAGE(1),				    0x01,
        COLLECTION(1),			0x00,
          USAGE_PAGE(1),			0x09,
          USAGE_MINIMUM(1),		0x1,
          USAGE_MAXIMUM(1),		0x3,
          LOGICAL_MINIMUM(1),	0x0,
          LOGICAL_MAXIMUM(1),	0x1,
          REPORT_COUNT(1),		0x3,
          REPORT_SIZE(1),	  	0x1,
          INPUT(1), 				  0x2,		// (Data, Variable, Absolute), ;3 button bits
          REPORT_COUNT(1),		0x1,
          REPORT_SIZE(1),		  0x5,
          INPUT(1), 				  0x1,		//(Constant), ;5 bit padding
          USAGE_PAGE(1), 		  0x1,		//(Generic Desktop),
          USAGE(1),				    0x30,
          USAGE(1),				    0x31,
          LOGICAL_MINIMUM(1),	0x81,
          LOGICAL_MAXIMUM(1),	0x7f,
          REPORT_SIZE(1),	  	0x8,
          REPORT_COUNT(1),		0x2,
          INPUT(1), 				  0x6,		//(Data, Variable, Relative), ;2 position bytes (X & Y)
        END_COLLECTION(0),
			END_COLLECTION(0)
		};
		/*
		 * Set report map (here is initialized device driver on client side)
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report_map.xml
		 */
		//hid->setReportMap((uint8_t*)reportMap, sizeof(reportMap));
		hid->reportMap((uint8_t*)reportMapKbd, sizeof(reportMapKbd));

		/*
		 * We are prepared to start hid device services. Before this point we can change all values and/or set parameters we need.
		 * Also before we start, if we want to provide battery info, we need to prepare battery service.
		 * We can setup characteristics authorization
		 */
		hid->startServices();

		/*
		 * Its good to setup advertising by providing appearance and advertised service. This will let clients find our device by type
		 */
		BLEAdvertising *pAdvertising = pServer->getAdvertising();
		pAdvertising->setAppearance(HID_KEYBOARD);
		//pAdvertising->setAppearance(GENERIC_HID);
		pAdvertising->addServiceUUID(hid->hidService()->getUUID());
		pAdvertising->start();


		BLESecurity *pSecurity = new BLESecurity();
		pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
		pSecurity->setCapability(ESP_IO_CAP_NONE);
		pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

		ESP_LOGD(LOG_TAG, "Advertising started!");
    while(1) { delay(1000000); }
	}
};


extern "C" {
  
  /** @brief Main init function to start HID interface
   * 
   * @note After init, just use the queues! */
  esp_err_t HID_kbdmousejoystick_init(void)
  {
    //init FreeRTOS queues
    mouse_q = xQueueCreate(32,sizeof(mouse_command_t));
    keyboard_q = xQueueCreate(32,sizeof(keyboard_command_t));
    joystick_q = xQueueCreate(32,sizeof(joystick_command_t));
    
    //init Neil Kolban's HOG task
    BLE_HOG* blehid = new BLE_HOG();
    blehid->setStackSize(16192);
    blehid->start();
    return ESP_OK;
  }
}

