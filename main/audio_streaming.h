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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_websocket_client.h"
#include "esp_event.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include <math.h>


#define SAMPLE_RATE     (16000)
#define I2S_DMA_SIZE	(512)
#define I2S_NUM         (0)
#define I2S_BCK_IO      (14)
#define I2S_WS_IO       (15)
#define I2S_DI_IO       (32)
#define I2S_DO_IO       (-1)

/** @brief Initialize audio streaming
 * 
 * Initialize the audio streaming by:
 * * reading out the Wifi credentials & IP/port to connect to
 * * connect to the wifi
 * * initialize but stop the I2S
 */
esp_err_t audio_streaming_init(void);

/** @brief Start audio streaming
 * 
 * Once initialized, the audio streaming can be started with this command.
 */
esp_err_t audio_streaming_start(void);

/** @brief Stop audio streaming
 * 
 * Once initialized and started, the audio streaming can be stopped with this command.
 */
esp_err_t audio_streaming_stop(void);
