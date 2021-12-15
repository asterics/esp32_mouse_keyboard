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
 * Copyright 2021:
 * Benjamin Aigner <beni@asterics-foundation.org>,<aignerb@technikum-wien.at>
 *
 * This file is part of the esp32_mouse_keyboard project.
 * 
 * The audio_streaming module is used for:
 * * reading the CMM4030-D MEMS microphone on the PCB
 * * connecting to a speech processor device (websocket) via Wifi
 * * sending speech data via UART to the host device (FLipMouse/FABI)
 * 
 * There are 3 states involved when operating:
 * 
 * 1. uninitialized: not connected to Wifi, I2S hardware uninitialized
 * 2. inactive: connected to the Wifi, I2S hardware initialized but stopped
 * 3. active: processing I2S data, sending it to the websocket and receiving STT (speech to text) data
 *
 */

#include "audio_streaming.h"
 
static EventGroupHandle_t audiostreamingstatus = NULL;

#define AUDIOSTREAMING_INITIALIZED  (1<<0)
//#define AUDIOSTREAMING_CONNECTED  	(1<<1)
#define AUDIOSTREAMING_RUNNING		(1<<2)

#define LOG_TAG "audiostreaming"

wifi_config_t wifi_config;
esp_websocket_client_handle_t client;

/** @brief NVS handle to store key/value pairs via UART
 * 
 * In NVS, we store arbitrary values, which are sent via UART.
 * This can(will) be used for storing different values from the
 * GUI on the ESP32. */
nvs_handle nvs_storage_h;

/** @brief main task for audio streaming */
void audio_streaming_task(void *param)
{
	ESP_LOGI(LOG_TAG,"main I2S task started");
    
    //samples buffer, int32 raw data. I2S_DMA_SIZE is in Bytes -> divide by size
    int32_t samples[I2S_DMA_SIZE/sizeof(int32_t)];
    //byte buffer for processed data sent to WS. is char, but stored data is int16 WAV!
    //(required by CMM4030-D: always clock 64bits)
    //char txbuf[(I2S_DMA_SIZE/sizeof(int32_t))*sizeof(int16_t)/2];
    int16_t txbuf[2048];
    
    
	while(1)
	{
		//wait until transfer is enabled
		xEventGroupWaitBits(audiostreamingstatus,AUDIOSTREAMING_RUNNING,pdFALSE,pdFALSE,portMAX_DELAY);
		ESP_LOGI(LOG_TAG,"main I2S task resumed");
		
		i2s_start(I2S_NUM);

		//if started (flag) start I2S & send to WS.
		while(1)
		{
			size_t totalWords = 0;
			//we read 32 i2s buffers (32x64samples a 16bits -> 4096 B as in txbuf)
			for(uint16_t j = 0; j<16; j++)
			{
				size_t read;
				i2s_read(I2S_NUM,samples,I2S_DMA_SIZE,&read,portMAX_DELAY);
			
				//process I2S samples to int16_t (WAV format) & split into bytes (for WS)
				//we got count of bytes (read), but iterate over by samples in int32_t
				//and convert stereo to mono
				//char buf[9];
				for(int i = 0; i<(read/sizeof(int32_t)/2); i++)
				{
					//map to 16bit raw (original: 24 bits)
					txbuf[totalWords] = (int16_t)(samples[i*2]>>10);
					
					//this variant led to clipping, above is shifted for less resolution but without clipping.
					//txbuf[totalWords] = (int16_t)(samples[i*2]>>8);
					totalWords++;
				}
			}
			
			//send to WS.
			if (esp_websocket_client_is_connected(client)) {
				esp_websocket_client_send_bin(client, txbuf, totalWords*2, portMAX_DELAY);
			}
			
			//at least one tick delay to avoid watchdog reset
			//vTaskDelay(1);
			//check if we should break this loop
			if(!(xEventGroupGetBits(audiostreamingstatus) & AUDIOSTREAMING_RUNNING))
			{
				ESP_LOGI(LOG_TAG,"main I2S task suspended");
				i2s_stop(I2S_NUM);
				break;
			}
		}
	}
		
}


static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(LOG_TAG, "WEBSOCKET_EVENT_CONNECTED");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(LOG_TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(LOG_TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(LOG_TAG, "Received opcode=%d", data->op_code);
        if (data->op_code == 0x08 && data->data_len == 2) {
            ESP_LOGW(LOG_TAG, "Received closed message with code=%d", 256*data->data_ptr[0] + data->data_ptr[1]);
        } else {
			//if we received something from the websocket (recognized words), send this with a prefix to the UART.
			if(data->data_len > 0)
			{
				ESP_LOGW(LOG_TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
				uart_write_bytes(EX_UART_NUM, "AP:",strlen("AP:"));
				uart_write_bytes(EX_UART_NUM, data->data_ptr,data->data_len);
				uart_write_bytes(EX_UART_NUM,"\r\n",strlen("\r\n")); //newline
			}
        }
        ESP_LOGW(LOG_TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(LOG_TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}

static esp_err_t init_i2s(void)
{
	//zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set, here it is IO13 -> enable I2S mic
    io_conf.pin_bit_mask = (1ULL<<13);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    
    //enable mic
    gpio_set_level(13, 1);
    vTaskDelay(100/portTICK_PERIOD_MS);
    
    // Configure the I2S peripheral for the PDM microphone
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = I2S_DMA_SIZE,
    };

    // Set the Clock and Data pins
    i2s_pin_config_t pin_config;
    pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
    pin_config.bck_io_num = I2S_BCK_IO;
    pin_config.ws_io_num = I2S_WS_IO;
    pin_config.data_out_num = I2S_DO_IO;
    pin_config.data_in_num = I2S_DI_IO;
    
    esp_err_t ret = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if(ret != ESP_OK)
    {
		ESP_LOGE(LOG_TAG, "err-init: %s",esp_err_to_name(ret));
		return ret;
	}
    ret = i2s_set_pin(I2S_NUM, &pin_config);
	if(ret != ESP_OK)
    {
		ESP_LOGE(LOG_TAG, "err-init: %s",esp_err_to_name(ret));
		return ret;
	}
    i2s_set_clk(I2S_NUM, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_STEREO);
    //stop I2S
	i2s_stop(I2S_NUM);
	return ESP_OK;
}

static esp_ip4_addr_t s_ip_addr;

static void on_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(LOG_TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
    esp_websocket_client_start(client);
}

static void on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGI(LOG_TAG, "Wi-Fi disconnected, trying to reconnect...");
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
    ESP_ERROR_CHECK(err);
}

static esp_netif_t *wifi_start(void)
{
    char *desc;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    // Prefix the interface description with the module TAG
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    asprintf(&desc, "%s: %s", LOG_TAG, esp_netif_config.if_desc);
    esp_netif_config.if_desc = desc;
    esp_netif_config.route_prio = 128;
    esp_netif_t *netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    free(desc);
    esp_wifi_set_default_wifi_sta_handlers();

    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL);

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_LOGI(LOG_TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
    return netif;
}

/** @brief Initialize audio streaming
 * 
 * Initialize the audio streaming by:
 * * reading out the Wifi credentials & IP/port to connect to
 * * connect to the wifi
 * * initialize but stop the I2S
 */
esp_err_t audio_streaming_init(void)
{
	esp_err_t ret;
	
	//check if already initialized
	if(audiostreamingstatus != NULL)
	{
		if(xEventGroupGetBits(audiostreamingstatus) & AUDIOSTREAMING_INITIALIZED)
		{
			ESP_LOGW(LOG_TAG,"Already initialized!");
			return ESP_ERR_INVALID_STATE;
		} else {
			ESP_LOGE(LOG_TAG,"Cannot initialize, flags are != NULL!");
			return ESP_FAIL;
		}
	}
	
	audiostreamingstatus = xEventGroupCreate();
	
	/** initialize the Wifi stack (from websocket_example.c) */
	nvs_flash_init();
	esp_netif_init();
	esp_event_loop_create_default();
	
	/** initialize the NVS backend */
	ESP_LOGI(LOG_TAG,"opening NVS handle for key/value storage");
    ret = nvs_open("kvstorage", NVS_READWRITE, &nvs_storage_h);
    if(ret != ESP_OK)
    {
		ESP_LOGE(LOG_TAG,"error opening NVS for key/value storage");
		return ret;
	}
	
	/** initialize Wifi & Websocket data */
	esp_websocket_client_config_t websocket_cfg = {};
	size_t sizeData;
	char* wsuri = NULL;
	char* nvspayload = NULL;
	ret = nvs_get_str(nvs_storage_h, "SPURI",NULL,&sizeData);
	//if we have a data length, load string
	if(ret == ESP_OK)
	{
		//load str data
		wsuri = malloc(sizeData);
		ret = nvs_get_str(nvs_storage_h, "SPURI", wsuri, &sizeData);
	}
	//OK or error?
	if(ret != ESP_OK)
	{
		//send back error message
		ESP_LOGE(LOG_TAG,"error reading value SPURI: %s",esp_err_to_name(ret));
		return ret;
	} else {
		websocket_cfg.uri = wsuri;
		ESP_LOGI(LOG_TAG,"connecting to WS URI: %s",wsuri);
	}
	
	ret = nvs_get_str(nvs_storage_h, "SSID",NULL,&sizeData);
	//if we have a data length, load string
	if(ret == ESP_OK)
	{
		//load str data
		nvspayload = malloc(sizeData);
		ret = nvs_get_str(nvs_storage_h, "SSID", nvspayload, &sizeData);
	}
	//OK or error?
	if(ret != ESP_OK)
	{
		//send back error message
		ESP_LOGE(LOG_TAG,"error reading value SSID: %s",esp_err_to_name(ret));
		return ret;
	} else {
		strncpy((char*)wifi_config.sta.ssid,nvspayload,32);
		free(nvspayload);
		ESP_LOGI(LOG_TAG,"connecting to SSID: %s",wifi_config.sta.ssid);
	}
	
	ret = nvs_get_str(nvs_storage_h, "PASS",NULL,&sizeData);
	//if we have a data length, load string
	if(ret == ESP_OK)
	{
		//load str data
		nvspayload = malloc(sizeData);
		ret = nvs_get_str(nvs_storage_h, "PASS", nvspayload, &sizeData);
	}
	//OK or error?
	if(ret != ESP_OK)
	{
		//send back error message
		ESP_LOGE(LOG_TAG,"error reading value PASS: %s",esp_err_to_name(ret));
		return ret;
	} else {
		strncpy((char*)wifi_config.sta.password,nvspayload,32);
		free(nvspayload);
		ESP_LOGI(LOG_TAG,"connecting with PW: %s",wifi_config.sta.password);
	}

	
	ESP_LOGI(LOG_TAG, "Connecting to %s...", websocket_cfg.uri);
	wifi_start();
    client = esp_websocket_client_init(&websocket_cfg);
    ret = esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);
    if(ret != ESP_OK)
    {
		ESP_LOGE(LOG_TAG, "err-init: %s",esp_err_to_name(ret));
		return ret;
	}

    ret = init_i2s();
    if(ret != ESP_OK)
    {
		ESP_LOGE(LOG_TAG, "err-init: %s",esp_err_to_name(ret));
		return ret;
	}
	
	//start main task
	ret = xTaskCreate( audio_streaming_task, "I2S_WS", 8192, NULL, tskIDLE_PRIORITY, NULL);
    
    //esp_websocket_client_is_connected(client)
    xEventGroupSetBits(audiostreamingstatus,AUDIOSTREAMING_INITIALIZED);
	return ESP_OK;
}

/** @brief Start audio streaming
 * 
 * Once initialized, the audio streaming can be started with this command.
 */
esp_err_t audio_streaming_start(void)
{
	if(audiostreamingstatus == NULL)
	{
		ESP_LOGE(LOG_TAG,"Cannot start, flags uninitialized");
		return ESP_ERR_INVALID_STATE;
	}
	
	if(!(xEventGroupGetBits(audiostreamingstatus) & AUDIOSTREAMING_INITIALIZED))
	{
		ESP_LOGE(LOG_TAG,"Cannot start, uninitialized");
		return ESP_ERR_INVALID_STATE;		
	}
	
	if(xEventGroupGetBits(audiostreamingstatus) & AUDIOSTREAMING_RUNNING)
	{
		ESP_LOGE(LOG_TAG,"Cannot start, already running");
		return ESP_FAIL;		
	}
	
	xEventGroupSetBits(audiostreamingstatus,AUDIOSTREAMING_RUNNING);
	ESP_LOGI(LOG_TAG,"Started streaming");
	return ESP_OK;
}

/** @brief Stop audio streaming
 * 
 * Once initialized and started, the audio streaming can be stopped with this command.
 */
esp_err_t audio_streaming_stop(void)
{
	if(audiostreamingstatus == NULL)
	{
		ESP_LOGE(LOG_TAG,"Cannot stop, flags uninitialized");
		return ESP_ERR_INVALID_STATE;
	}
	
	if(!(xEventGroupGetBits(audiostreamingstatus) & AUDIOSTREAMING_INITIALIZED))
	{
		ESP_LOGE(LOG_TAG,"Cannot stop, uninitialized");
		return ESP_ERR_INVALID_STATE;		
	}
	
	if(!(xEventGroupGetBits(audiostreamingstatus) & AUDIOSTREAMING_RUNNING))
	{
		ESP_LOGE(LOG_TAG,"Cannot stop, not running");
		return ESP_FAIL;		
	}
	esp_websocket_client_send_text(client, "EOS", strlen("EOS"), portMAX_DELAY);
	xEventGroupClearBits(audiostreamingstatus,AUDIOSTREAMING_RUNNING);
	ESP_LOGI(LOG_TAG,"Stopped streaming");
	return ESP_OK;
}
