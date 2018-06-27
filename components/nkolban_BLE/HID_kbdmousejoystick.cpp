/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
* 
*   http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
* 
* Copyright Neil Kolban
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

/// @brief Input queue for sending joystick reports
QueueHandle_t joystick_q;
/// @brief Input queue for sending keyboard reports
QueueHandle_t keyboard_q;
/// @brief Input queue for sending mouse reports
QueueHandle_t mouse_q;

/// @brief Is the BLE currently connected?
uint8_t isConnected = 0;

///@brief Is Keyboard interface active?
uint8_t activateKeyboard = 0;
///@brief Is Mouse interface active?
uint8_t activateMouse = 0;
///@brief Is Joystick interface active?
uint8_t activateJoystick = 0;
///@brief Is this device in testmode (automatically sending HID reports)?
uint8_t testmode = 0;

///@brief The BT Devicename for advertising
char btname[40];


//static BLEHIDDevice class instance for communication (sending reports)
static BLEHIDDevice* hid;
//BLE server handle
BLEServer *pServer;
//characteristic for sending keyboard reports to the host
BLECharacteristic* inputKbd;
//characteristic for sending mouse reports to the host
BLECharacteristic* inputMouse;
//characteristic for sending mouse reports to the host
BLECharacteristic* inputJoystick;
//characteristic for receiving keyboard reports from the host (status LEDs)
BLECharacteristic* outputKbd;

/** @brief Constant report map for keyboard
 * 
 * This report map will be used on init do build a report map according
 * to init functions (with activated interfaces).
 * 
 * @note Report id is on all reports in offset 7.
 * */
const uint8_t reportMapKeyboard[] = {
  USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
  USAGE(1),           0x06,       // Keyboard
  COLLECTION(1),      0x01,       // Application
    REPORT_ID(1),       0x01,
    //report equal to usb_bridge (https://github.com/benjaminaigner/usb_bridge
    REPORT_SIZE(1),     0x01,
    REPORT_COUNT(1),    0x08,
    USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
    USAGE_MINIMUM(1),   0xE0,
    USAGE_MAXIMUM(1),   0xE7,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    INPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x08,
    INPUT(1),           0x03,
    REPORT_COUNT(1),    0x05,
    REPORT_SIZE(1),     0x01,
    USAGE_PAGE(1),      0x08,       //   LEDs
    USAGE_MINIMUM(1),   0x01,       //   Num Lock
    USAGE_MAXIMUM(1),   0x05,       //   Kana
    OUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
    REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
    REPORT_SIZE(1),     0x03,
    OUTPUT(1),          0x03,
    REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
    REPORT_SIZE(1),     0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 104,       //   104 keys
    USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
    USAGE_MINIMUM(1),   0x00,       //   Num Lock
    USAGE_MAXIMUM(1),   104,       //   Kana
    INPUT(1),           0x00,
  END_COLLECTION(0)
};

/** @brief Constant report map for mouse
 * 
 * This report map will be used on init do build a report map according
 * to init functions (with activated interfaces).
 * 
 * @note Report id is on all reports in offset 7.
 * */
const uint8_t reportMapMouse[] = {
  USAGE_PAGE(1), 			0x01,
  USAGE(1), 				  0x02,
  COLLECTION(1),			0x01,
    REPORT_ID(1),       0x02,
    USAGE(1),				    0x01,
    COLLECTION(1),			0x00,
      USAGE_PAGE(1),			0x09,
      USAGE_MINIMUM(1),		0x1,
      USAGE_MAXIMUM(1),		0x3,
      LOGICAL_MINIMUM(1),	0x0,
      LOGICAL_MAXIMUM(1),	0x1,
      REPORT_COUNT(1),		0x3,
      REPORT_SIZE(1),	  	0x1,
      INPUT(1), 				  0x2,		// (Data, Variable, Absolute), ;8 button bits
      REPORT_COUNT(1),		0x1,
      REPORT_SIZE(1),		  0x5,
      INPUT(1), 				  0x1,		//(Constant), ;5 bit padding
      USAGE_PAGE(1), 		  0x1,		//(Generic Desktop),
      USAGE(1),				    0x30,   //X
      USAGE(1),				    0x31,   //Y
      USAGE(1),				    0x38,   //wheel
      LOGICAL_MINIMUM(1),	0x81,
      LOGICAL_MAXIMUM(1),	0x7f,
      REPORT_SIZE(1),	  	0x8,
      REPORT_COUNT(1),		0x03,   //3 bytes: X/Y/wheel
      INPUT(1), 				  0x6,		//(Data, Variable, Relative), ;3 position bytes (X & Y & wheel)
    END_COLLECTION(0),
  END_COLLECTION(0)
};


/** @brief Constant report map for joystick
 * 
 * This report map will be used on init do build a report map according
 * to init functions (with activated interfaces).
 * 
 * @note Report id is on all reports in offset 7.
 * */
const uint8_t reportMapJoystick[] = {
  USAGE_PAGE(1), 			0x01,
  USAGE(1), 				  0x04,
  COLLECTION(1),			0x01,
    REPORT_ID(1),       0x03,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_COUNT(1),		0x20, /* 32 */
    REPORT_SIZE(1),	  	0x01,
    USAGE_PAGE(1),      0x09,
    USAGE_MINIMUM(1),   0x01,
    USAGE_MAXIMUM(1),   0x20, /* 32 */
    INPUT(1),           0x02, // variable | absolute
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x07,
    PHYSICAL_MINIMUM(1),0x01,
    PHYSICAL_MAXIMUM(2),(315 & 0xFF), ((315>>8) & 0xFF),
    REPORT_SIZE(1),	  	0x04,
    REPORT_COUNT(1),	  0x01,
    UNIT(1),            20,
    USAGE_PAGE(1), 			0x01,
    USAGE(1), 				  0x39,  //hat switch
    INPUT(1),           0x42, //variable | absolute | null state
    USAGE_PAGE(1), 			0x01,
    USAGE(1), 				  0x01,  //generic pointer
    COLLECTION(1),      0x00,
      LOGICAL_MINIMUM(1), 0x00,
      //LOGICAL_MAXIMUM(4), (1023 & 0xFF), ((1023>>8) & 0xFF), ((1023>>16) & 0xFF), ((1023>>24) & 0xFF),
      LOGICAL_MAXIMUM(2), (1023 & 0xFF), ((1023>>8) & 0xFF),
      REPORT_COUNT(1),		0x04,
      REPORT_SIZE(1),	  	0x0A, /* 10 */
      USAGE(1), 				  0x30,  // X axis
      USAGE(1), 				  0x31,  // Y axis
      USAGE(1), 				  0x32,  // Z axis
      USAGE(1), 				  0x35,  // Z-rotator axis
      INPUT(1),           0x02, // variable | absolute
    END_COLLECTION(0),
    LOGICAL_MINIMUM(1), 0x00,
    //LOGICAL_MAXIMUM(4), (1023 & 0xFF), ((1023>>8) & 0xFF), ((1023>>16) & 0xFF), ((1023>>24) & 0xFF),
    LOGICAL_MAXIMUM(2), (1023 & 0xFF), ((1023>>8) & 0xFF),
    REPORT_COUNT(1),		0x02,
    REPORT_SIZE(1),	  	0x0A, /* 10 */
    USAGE(1), 				  0x36,  // generic slider
    USAGE(1), 				  0x36,  // generic slider
    INPUT(1),           0x02, // variable | absolute
  END_COLLECTION(0)
};

class kbdOutputCB : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic* me){
		uint8_t* value = (uint8_t*)(me->getValue().c_str());
		ESP_LOGI(LOG_TAG, "special keys: %d", *value);
	}
};

class KeyboardTask : public Task {
	void run(void*){
    //if we are in test mode, we will just send this string.
    if(testmode)
    {
      vTaskDelay(2000/portTICK_PERIOD_MS);
    	const char* hello = "Hello world from esp32 hid keyboard!!!";
      while(*hello){
        KEYMAP map = keymap[(uint8_t)*hello];
        //send key down
        uint8_t a[] = {map.modifier, 0x0, map.usage, 0x0,0x0,0x0,0x0,0x0};
        ESP_LOGI(LOG_TAG,"Kbd: %c, keycode: 0x%2X, modifier: 0x%2X",hello[0],map.usage,map.modifier);
        inputKbd->setValue(a,sizeof(a));
        inputKbd->notify();
        //send key up
        uint8_t b[] = {0x0, 0x0, 0x00, 0x0,0x0,0x0,0x0,0x0};
        inputKbd->setValue(b,sizeof(b));
        inputKbd->notify();
        
        //next character
        hello++;
        //wait 1 tick
        vTaskDelay(1);
      }
    }
    //if we are not in testmode, wait for data to arrive in queue
    //and process accordingly (key down, key up, auto down/up)
    if(!testmode)
    {
      uint8_t a[8] = {0,0,0,0,0,0,0,0};
      keyboard_command_t cmd;
      while(1)
      {
        if(xQueueReceive(keyboard_q,&cmd,10000))
        {
          ESP_LOGI(LOG_TAG,"Keyboard received: %d/%d",cmd.type,cmd.keycode);
          //save the modifier
          a[0] = (cmd.keycode & 0xFF00)>> 8;
          //do either press, release or both
          switch(cmd.type)
          {
            case PRESS:
              add_keycode(cmd.keycode & 0xFF, &a[2]);
              break;
            case PRESS_RELEASE:
              add_keycode(cmd.keycode & 0xFF, &a[2]);
              //send first report with added keycode, release one is sent later
              inputKbd->setValue(a,sizeof(a));
              inputKbd->notify();
              remove_keycode(cmd.keycode & 0xFF,&a[2]);
              break;
            case RELEASE:
              remove_keycode(cmd.keycode & 0xFF,&a[2]);
              break;
            case RELEASE_ALL:
              memset(a,0,8);
            default:
              ESP_LOGE(LOG_TAG,"Unknown type of keyboard command");
              break;
          }
          inputKbd->setValue(a,sizeof(a));
          inputKbd->notify();
        }
      }
    }
	}
};
KeyboardTask *kbd; //instance for this task

class MouseTask : public Task {
	void run(void*){
    //if we are in test mode, move the mouse around every 2s
    if(testmode)
    {
      uint8_t step = 0;
      //<button>, <x>, <y>, <wheel>
      uint8_t a[] = {0x00,(uint8_t) -0, (uint8_t) -0,(uint8_t)-0};
      vTaskDelay(10000/portTICK_PERIOD_MS);
      while(1){
        //do 5 different mouse operations (X,Y movement + left click)
        switch(step) {
          case 0:
            a[0] = 0;
            a[1] = 50;
            step++;
            break;
          case 1:
            a[1] = -50;
            step++;
            break;
          case 2:
            a[2] = 50;
            step++;
            break;
          case 3:
            step++;
            a[2] = -50;
            break;
          default:
            a[0] = (1<<0);
            a[1] = 0;
            a[2] = 0;
            step = 0;
            break;
        }
        ESP_LOGI(LOG_TAG,"Sending mouse report: ");
        ESP_LOG_BUFFER_HEXDUMP(LOG_TAG,a,sizeof(a),ESP_LOG_INFO);
        inputMouse->setValue(a,sizeof(a));
        inputMouse->notify();
        vTaskDelay(2000/portTICK_PERIOD_MS);
      }
    }
    if(!testmode)
    {
      mouse_command_t cmd;
      while(1)
      {
        //wait for a new mouse command
        if(xQueueReceive(mouse_q,&cmd,10000))
        {
          ESP_LOGI(LOG_TAG,"Mouse received: %d/%d/%d/%d",cmd.buttons,cmd.x,cmd.y,cmd.wheel);
          uint8_t a[4] = {0,0,0,0};
          a[0] = cmd.buttons;
          a[1] = cmd.x;
          a[2] = cmd.y;
          a[3] = cmd.wheel;
          inputMouse->setValue(a,sizeof(a));
          inputMouse->notify();
        }
      }
    }
	}
};
MouseTask *mouse; //instance for this task


class JoystickTask : public Task {
	void run(void*){
    vTaskDelay(1000/portTICK_PERIOD_MS);
		while(1){
        vTaskDelay(10000/portTICK_PERIOD_MS);
        //TODO: implement joystick testmode & normal mode via queue
		}
			/*uint8_t v[] = {0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0};
			inputJoystick->setValue(v, sizeof(v));
			inputJoystick->notify();*/
		vTaskDelete(NULL);
	}
};
JoystickTask *joystick; //instance for this task

class CBs: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    BLE2902* desc;
    
    isConnected = 1;
    
    //enable notifications for input service, suggested by chegewara to support iOS/Win10
    //https://github.com/asterics/esp32_mouse_keyboard/issues/4#issuecomment-386558158
    if(activateKeyboard)
    {
      desc = (BLE2902*) inputKbd->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      desc->setNotifications(true);
      kbd->start();
    }
    
    if(activateMouse)
    {
      desc = (BLE2902*) inputMouse->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      desc->setNotifications(true);
      mouse->start();
    }

    if(activateJoystick)
    {
      desc = (BLE2902*) inputJoystick->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      desc->setNotifications(true);
      joystick->start();
    }
    ESP_LOGI(LOG_TAG,"Client connected ! ");
  }

  void onDisconnect(BLEServer* pServer){
    BLE2902* desc;
    
    isConnected = 0;
    
    //disable notifications for input service, suggested by chegewara to
    //reduce power & memory usage
    //https://github.com/asterics/esp32_mouse_keyboard/commit/a1796ce91155ec7db62af4a53dbdef32bc4adf08#commitcomment-28888676
    if(activateKeyboard)
    {
      desc = (BLE2902*) inputKbd->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      desc->setNotifications(false);
      kbd->stop();
    }
    
    if(activateMouse)
    {
      desc = (BLE2902*) inputMouse->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      desc->setNotifications(false);
      mouse->stop();
    }
    
    if(activateJoystick)
    {
      desc = (BLE2902*) inputJoystick->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      desc->setNotifications(false);
      joystick->stop();
    }
    
    //restart advertising
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
    ESP_LOGI(LOG_TAG,"Client disconnected, restarting advertising");
  }
};

uint32_t passKey = 1307;
/** @brief security callback
 * 
 * This class is passed to the BLEServer as callbacks for security
 * related actions. Depending on IO_CAP configuration & host, different
 * types of security actions are required for bonding this device to a
 * host. */
class CB_Security: public BLESecurityCallbacks {
  
  // Request a pass key to be typed in on the host
  uint32_t onPassKeyRequest(){
    ESP_LOGE(LOG_TAG, "The passkey request %d", passKey);
    vTaskDelay(25000);
    return passKey;
  }
  
  // The host sends a pass key to the ESP32 which needs to be displayed
  //and typed into the host PC
  void onPassKeyNotify(uint32_t pass_key){
    ESP_LOGE(LOG_TAG, "The passkey Notify number:%d", pass_key);
    passKey = pass_key;
  }
  
  // CB for testing if a host is allowed to connect, in our case always yes.
  bool onSecurityRequest(){
    return true;
  }

  // CB on a completed authentication (not depending on status)
  void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl){
    if(auth_cmpl.success){
      ESP_LOGI(LOG_TAG, "remote BD_ADDR:");
      esp_log_buffer_hex(LOG_TAG, auth_cmpl.bd_addr, sizeof(auth_cmpl.bd_addr));
      ESP_LOGI(LOG_TAG, "address type = %d", auth_cmpl.addr_type);
    }
      ESP_LOGI(LOG_TAG, "pair status = %s", auth_cmpl.success ? "success" : "fail");
  }
  
  // You need to confirm the given pin
  bool onConfirmPIN(uint32_t pin)
  {
    ESP_LOGE(LOG_TAG, "Confirm pin: %d", pin);
    return true;
  }
  
};

/** @brief Main BLE HID-over-GATT task
 * 
 * This task is used to initialize 3 tasks for keyboard, mouse and joystick
 * as well as intializing the BLE device:
 * * Init device
 * * Create a new server
 * * Attach a HID over GATT implementation to the server
 * * Create the input & output reports for the different HID devices
 * * Create & add the HID report map
 * * Finally start the server & services and start advertising
 * */
class BLE_HOG: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "Initialising BLE HID device.");

    /*
     * Create new task instances, if necessary
     */
    if(activateKeyboard)
    {
      kbd = new KeyboardTask();
      kbd->setStackSize(8096);
    }
    if(activateMouse)
    {
      mouse = new MouseTask();
      mouse->setStackSize(8096);
    }
    if(activateJoystick)
    {
      joystick = new JoystickTask();
      joystick->setStackSize(8096);
    }
    
		BLEDevice::init(btname);
		pServer = BLEDevice::createServer();
		pServer->setCallbacks(new CBs());
		//BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);
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
    //hid->pnp(0x01,0xE502,0xA111,0x0210); //BT SIG assigned VID
    hid->pnp(0x02,0xE502,0xA111,0x0210); //USB assigned VID

		/*
		 * Set hid informations
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hid_information.xml
		 */
		//const uint8_t val1[] = {0x01,0x11,0x00,0x03};
		//hid->hidInfo()->setValue((uint8_t*)val1, 4);
    hid->hidInfo(0x00,0x01);

    /*
     * Build a report map, depending on init function.
     * For each enabled interface (keyboard, mouse, joystick) the
     * corresponding report map is copied to a new map which is 
     * used for initializing the BLEHID class.
     * Report IDs are changed accordingly.
     */
    size_t reportMapSize = 0;
    if(activateKeyboard) reportMapSize += sizeof(reportMapKeyboard);
    if(activateMouse) reportMapSize += sizeof(reportMapMouse);
    if(activateJoystick) reportMapSize += sizeof(reportMapJoystick);
    
    uint8_t *reportMap = (uint8_t *)malloc(reportMapSize);
    uint8_t *reportMapCurrent = reportMap;
    uint8_t reportID = 1;
    
    if(reportMap == nullptr)
    {
      ESP_LOGE(LOG_TAG,"Cannot allocate memory for the report map, cannot start HID");
    } else {
      //copy report map for keyboard to allocated full report map, if activated
      if(activateKeyboard)
      {
        //create report map for keyboard
        memcpy(reportMapCurrent,reportMapKeyboard,sizeof(reportMapKeyboard));
        reportMap[7] = reportID;
        reportMapCurrent += sizeof(reportMapKeyboard);
        
        //create in&out characteristics/reports for keyboard
        inputKbd = hid->inputReport(reportID);
        outputKbd = hid->outputReport(reportID);
        outputKbd->setCallbacks(new kbdOutputCB());
        
        ESP_LOGI(LOG_TAG,"Keyboard added @report ID %d, current report Map:", reportID);
        ESP_LOG_BUFFER_HEXDUMP(LOG_TAG,reportMap,(uint16_t)(reportMapCurrent-reportMap),ESP_LOG_INFO);
        
        reportID++; //increase report id for next interface
      }
      //copy report map for joystick to allocated full report map, if activated
      if(activateJoystick)
      {
        memcpy(reportMapCurrent,reportMapJoystick,sizeof(reportMapJoystick));
        reportMapCurrent[7] = reportID;
        reportMapCurrent += sizeof(reportMapJoystick);
        
        //create in characteristics/reports for joystick
        inputJoystick = hid->inputReport(reportID);
        
        ESP_LOGI(LOG_TAG,"Joystick added @report ID %d, current report Map:", reportID);
        ESP_LOG_BUFFER_HEXDUMP(LOG_TAG,reportMap,(uint16_t)(reportMapCurrent-reportMap),ESP_LOG_INFO);
        
        reportID++; //increase report id for next interface
      }
      //copy report map for mouse to allocated full report map, if activated
      if(activateMouse)
      {
        memcpy(reportMapCurrent,reportMapMouse,sizeof(reportMapMouse));
        reportMapCurrent[7] = reportID;
        reportMapCurrent += sizeof(reportMapMouse);
        
        //create in characteristics/reports for mouse
        inputMouse = hid->inputReport(reportID);
        
        ESP_LOGI(LOG_TAG,"Mouse added @report ID %d, current report Map:", reportID);
        ESP_LOG_BUFFER_HEXDUMP(LOG_TAG,reportMap,(uint16_t)(reportMapCurrent-reportMap),ESP_LOG_INFO);
        
        reportID++; //increase report id for next interface
      }
      
      ESP_LOGI(LOG_TAG,"Final report map size: %d B",reportMapSize);
          
      /*
       * Set report map (here is initialized device driver on client side)
       * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report_map.xml
       */
      hid->reportMap((uint8_t*)reportMap, reportMapSize);
  
      /*
       * We are prepared to start hid device services. Before this point we can change all values and/or set parameters we need.
       * Also before we start, if we want to provide battery info, we need to prepare battery service.
       * We can setup characteristics authorization
       */
      hid->startServices();
    }

		/*
		 * Its good to setup advertising by providing appearance and advertised service. This will let clients find our device by type
		 */
		BLEAdvertising *pAdvertising = pServer->getAdvertising();
		//pAdvertising->setAppearance(HID_KEYBOARD);
		pAdvertising->setAppearance(GENERIC_HID);
    pAdvertising->setMinInterval(400); //250ms minimum
    pAdvertising->setMaxInterval(800); //500ms maximum
		pAdvertising->addServiceUUID(hid->hidService()->getUUID());
		pAdvertising->start();
    


		BLESecurity *pSecurity = new BLESecurity();
//		pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
		//pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND );
 
		pSecurity->setCapability(ESP_IO_CAP_NONE);
		//pSecurity->setCapability(ESP_IO_CAP_OUT);
		//pSecurity->setCapability(ESP_IO_CAP_KBDISP);
		//pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

		ESP_LOGI(LOG_TAG, "Advertising started!");
    while(1) { delay(1000000); }
	}
};


extern "C" {
  
    
  /** @brief Directly send a HID keyboard report
   * 
   * @param a Pointer to report buffer
   * @param len Size of report buffer
   * @note Buffer lenght must equal 8 Bytes!
   * @note 1st Byte is modifier, 2nd is empty, 3 to 8 are keycodes
   * */
  esp_err_t HID_kbdmousejoystick_rawKeyboard(uint8_t *a, uint8_t len)
  {
    if(len != 8 || a == nullptr) return ESP_FAIL;
    inputKbd->setValue(a,len);
    inputKbd->notify();
    return ESP_OK;
  }
  
  /** @brief Directly send a HID mouse report
   * 
   * @param a Pointer to report buffer
   * @param len Size of report buffer
   * @note Buffer lenght must equal 4 Bytes!
   * @note 1st Byte is button mask, 2nd/3rd are X/Y, 4th is wheel
   * */
  esp_err_t HID_kbdmousejoystick_rawMouse(uint8_t *a, uint8_t len)
  {
    if(len != 4 || a == nullptr) return ESP_FAIL;
    inputMouse->setValue(a,len);
    inputMouse->notify();
    return ESP_OK;
  }
    
  esp_err_t HID_kbdmousejoystick_activatePairing(void)
  {
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
	pAdvertising->setAppearance(GENERIC_HID);
	pAdvertising->addServiceUUID(hid->hidService()->getUUID());
	pAdvertising->start();
    return ESP_OK;
  }
  
  esp_err_t HID_kbdmousejoystick_deactivatePairing(void)
  {
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
	pAdvertising->setAppearance(GENERIC_HID);
	pAdvertising->addServiceUUID(hid->hidService()->getUUID());
	pAdvertising->stop();
    return ESP_OK;
  }

  uint8_t HID_kbdmousejoystick_isConnected(void)
  {
    return isConnected;
  }
  
  /** @brief Main init function to start HID interface (C interface)
   * 
   * @note After init, just use the queues! */
  esp_err_t HID_kbdmousejoystick_init(uint8_t enableKeyboard, uint8_t enableMouse, uint8_t enableJoystick, uint8_t testmode, char * name)
  {
    //init FreeRTOS queues
    //initialise queues, even if they might not be used.
    mouse_q = xQueueCreate(32,sizeof(mouse_command_t));
    keyboard_q = xQueueCreate(32,sizeof(keyboard_command_t));
    joystick_q = xQueueCreate(32,sizeof(joystick_command_t));
    
    strncpy(btname, name, sizeof(btname)-1);
    
    //save enabled interfaces
    activateMouse = enableMouse;
    activateKeyboard = enableKeyboard;
    activateJoystick = enableJoystick;
    testmode = testmode;
    
    //init Neil Kolban's HOG task
    BLE_HOG* blehid = new BLE_HOG();
    blehid->setStackSize(16192);
    blehid->start();
    return ESP_OK;
  }
}

