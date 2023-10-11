/*
 * This file is part of a radiosonde firmware.
 * 
 
 * Copyright (C) 2023  Amon Schumann / DL9AS
 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */ 

#include <Arduino.h>
#include <SPI.h>

#include "camera.h"
#include "esp_camera.h"
#include "globals.h"
#include "pins.h"

#include <Preferences.h> // Save image ID permanently to EEPROM 

#include "../lib/ssdv/ssdv.h"
#include "../lib/base64/base64.hpp"

// Module globals
camera_fb_t *ov2640_frame_buf; // OV2640 camera frame buffer
camera_config_t ov2640_config; // OV2640 camera settings

Preferences preferences; // Save image ID permanently to EEPROM 

ssdv_t ssdv;
uint8_t packet_img_buf[IMAGE_PACKET_LENGTH]; // RAW image packet buffer
uint8_t packet_img_base64_buf[IMAGE_PACKET_BASE64_LENGTH]; // BASE64 image packet buffer
uint16_t img_buf_index = 0; // Index for iterating thru the image buffer

// Exported functions
void camera_begin()
{
  // Camera power enable pin initialization
  pinMode(OV2640_PWEN, OUTPUT);
  // Disable camera at begin
  camera_disable();
}

void camera_init()
{
  MCU_SET_FREQ_CAMERA;

  preferences.begin("DL9AS", false); // Open preferences namespace

  camera_enable(); // Enable camera at initialization
  DEBUG_PRINT("[OV2640] enabled ");

  // OV2640 camera hardware pin declaration
  ov2640_config.pin_pwdn = OV2640_PWDN;
  ov2640_config.pin_reset = OV2640_RESET;
  ov2640_config.pin_xclk = OV2640_XCLK;
  ov2640_config.pin_sccb_sda = OV2640_SIOD;
  ov2640_config.pin_sccb_scl = OV2640_SIOC;

  ov2640_config.pin_d7 = OV2640_D7;
  ov2640_config.pin_d6 = OV2640_D6;
  ov2640_config.pin_d5 = OV2640_D5;
  ov2640_config.pin_d4 = OV2640_D4;
  ov2640_config.pin_d3 = OV2640_D3;
  ov2640_config.pin_d2 = OV2640_D2;
  ov2640_config.pin_d1 = OV2640_D1;
  ov2640_config.pin_d0 = OV2640_D0;

  ov2640_config.pin_vsync = OV2640_VSYNC;
  ov2640_config.pin_href = OV2640_HREF;
  ov2640_config.pin_pclk = OV2640_PCLK;


  ov2640_config.xclk_freq_hz = 20000000; // Xclk frequency setting
  
  ov2640_config.pixel_format = PIXFORMAT_JPEG; // Set JPEG compression
  ov2640_config.frame_size = OV2640_FRAMESIZE;

  ov2640_config.jpeg_quality = OV2640_JPEG_QUALITY;
  ov2640_config.fb_count = 1;
  ov2640_config.fb_location = CAMERA_FB_IN_DRAM;

  // Initialize OV2640 camera
  esp_err_t ov2640_initialization_error = esp_camera_init(&ov2640_config);
  if (ov2640_initialization_error != ESP_OK) 
  {
    DEBUG_PRINT("[OV2640] Init err: ");
    DEBUG_PRINTLN(ov2640_initialization_error);
    
    camera_panic();
  }

  MCU_SET_FREQ_NORMAL; // Clock MCU down to save power
}

void camera_capture_image()
{
  uint16_t img_id_counter = preferences.getUInt("img_id", 0); // Get image ID from EEPROM

  img_buf_index = 0;
  
  DEBUG_PRINT("[OV2640] Capture IMG: ");
  DEBUG_PRINTLN(img_id_counter);

  #if IMAGE_ID_COUNTER == IMAGE_ID_RESET
    if(img_id_counter)
    {
      img_id_counter = 0; // Reset image ID counter
      preferences.putUInt("img_id", 0); // Write 0 to EEPROM
    }
  #endif

  MCU_SET_FREQ_CAMERA;

  // Capture image
  ov2640_frame_buf = esp_camera_fb_get();

  MCU_SET_FREQ_NORMAL; // Clock MCU down to save power

  // Initialize SSDV
  ssdv_enc_init(&ssdv, SSDV_TYPE_NOFEC, (char*) "", img_id_counter, SSDV_JPEG_QUALITY, IMAGE_PACKET_LENGTH); // Set SSDV callsign to "" -> will be striped off anyway
  ssdv_enc_set_buffer(&ssdv, packet_img_buf);

  #if IMAGE_ID_COUNTER == IMAGE_ID_RUNNING
    img_id_counter++; // Increment image ID counter
    preferences.putUInt("img_id", img_id_counter); // Write image ID to EEPROM
  #endif
}

bool camera_get_new_packet()
{
  DEBUG_PRINTLN("[SSDV] New packet");

  uint8_t ssdv_status = 0;

  while((ssdv_status = ssdv_enc_get_packet(&ssdv)) == SSDV_FEED_ME)
  {
    ssdv_enc_feed(&ssdv, &ov2640_frame_buf->buf[img_buf_index], IMAGE_SSDV_FEED_BUF_LENGTH);

    DEBUG_PRINT("[SSDV] FB index:"); // Print frame buffer index
    DEBUG_PRINTLN(img_buf_index);

    // Get index to image buffer next time
    if((img_buf_index + IMAGE_SSDV_FEED_BUF_LENGTH ) < ov2640_frame_buf->len) img_buf_index = img_buf_index + IMAGE_SSDV_FEED_BUF_LENGTH; // IMG buffer end not yet reached
    else img_buf_index = img_buf_index + (ov2640_frame_buf->len -  img_buf_index); // IMG buffer end reached
  }
  
  if(ssdv_status == SSDV_EOI) 
  {
    DEBUG_PRINTLN("[SSDV] End of Image");
    esp_camera_return_all();

    return false;
  }

  if(ssdv_status == SSDV_OK)
  {
    DEBUG_PRINTLN("[SSDV] Packet sucess");

    // Convert image packet to BASE64
    encode_base64(packet_img_buf + IMAGE_PACKET_SSDV_OFFSET, IMAGE_PACKET_LENGTH - IMAGE_PACKET_SSDV_OFFSET, packet_img_base64_buf); // Strip of sync byte, packet type and callsign from SSDV
    
    return true;
  }
  else
  {
    DEBUG_PRINTLN("[SSDV] Error");

    camera_panic();
  }
  
  return false;
}

void camera_panic()
{
  DEBUG_PRINTLN("[ESP] Panic RST");
  ESP.restart();
}

void camera_enable()
{
  digitalWrite(OV2640_PWEN, HIGH); // Enable power to OV2640
}

void camera_deinit()
{
  esp_camera_deinit();
}

void camera_disable()
{ 
  digitalWrite(OV2640_PWEN, LOW); // Disable power to OV2640
}