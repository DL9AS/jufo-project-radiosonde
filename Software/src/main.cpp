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

#include "config.h"
#include "globals.h"
#include "aprs.h"
#include "gps.h"
#include "geofence.h"
#include "SX1278.h"
#include "voltage.h"
#include "DS18B20.h"
#include "defines.h"
#if TARGET == TARGET_RS_4
  #include "camera.h"
  #ifdef CACHE_ENABLE
    #include "cache.h"
  #endif
  // ESP specific includes
  #include "soc/soc.h" // Disable brownout detector
  #include "soc/rtc_cntl_reg.h" // Disable brownout detector
  #if NVS_STATUS == NVS_RESET
    #include <nvs_flash.h>
  #endif
#endif

// Module globals
uint64_t global_freq = APRS_FREQUENCY_DEFAULT; // Global APRS frequency

uint16_t aprs_packet_counter = 0;
#if TARGET == TARGET_RS_4
  int16_t image_packet_counter = -1;
#endif

#if TARGET == TARGET_RS_4
  #include <Preferences.h> // Non-volatile storage
  Preferences* p_pref = new Preferences();
#endif

// Module functions
void main_generate_aprs_position_packet();
#if TARGET == TARGET_RS_4
  void pre_img_loop();
  void main_generate_aprs_image_packet();
  void main_capture_image();
  void main_handle_cache();
#endif

void setup()
{
  MCU_SET_FREQ_NORMAL; // Clock down MCU to save power

  #if TARGET == TARGET_RS_4
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector
  #endif

  // Enable or disable debug serial
  #ifdef DEBUG_SERIAL_ENABLE
    Serial.begin(DEBUG_SERIAL_BAUD_RATE);
  #endif

  // Initialize watchdog
  DEBUG_PRINTLN("[WDT] Init");
  WDT_INIT;

  // Initialize temperature sensor
  TEMP_BEGIN;

  // Start GPS
  DEBUG_PRINTLN("[GPS] Begin");
  gps_begin();
  
  // Init SX1278 SPI
  SX1278_begin();

  #if TARGET == TARGET_RS_4
    // Erase NVS partition if REST enabled
    #if NVS_STATUS == NVS_RESET
      nvs_flash_erase();
      nvs_flash_init();
    #endif

    DEBUG_PRINTLN("[CAM] Begin");
    camera_begin(p_pref);
    #ifdef CACHE_ENABLE
      cache_begin(p_pref);
    #endif
    p_pref->begin("DL9AS", false); // Open preferences namespace
    pre_img_loop(); // Run this loop before initializing camera
  #endif
}

void loop()
{
  gps_proccess_for_ms(RADIO_PACKET_DELAY); // Sleep with GPS running

  main_generate_aprs_position_packet();

  #if TARGET == TARGET_RS_4
    gps_proccess_for_ms(RADIO_PACKET_DELAY); // Sleep with GPS running

    main_generate_aprs_image_packet();

    #ifdef CACHE_ENABLE
      if(aprs_packet_counter % CACHE_RUN_HANDLER_EVERY == 0)
      {
        gps_proccess_for_ms(RADIO_PACKET_DELAY); // Sleep with GPS running

        main_handle_cache();
      }  
    #endif
  #endif
}

#if TARGET == TARGET_RS_4
  void pre_img_loop() // Loop for actions before camera initialized
  {
    for(uint8_t i = 0; i < PRE_IMG_LOOP_REAPEATS; i++)
    {
      gps_proccess_for_ms(RADIO_PACKET_DELAY * 2); // Sleep with GPS running

      main_generate_aprs_position_packet();
    }
  }
#endif

void main_generate_aprs_position_packet()
{
  GPS_END_BETWEEN; // Second serial needs to be stopped for APRS to function correctly on some MCUs
  
  // Acquire GPS position data
  char DMH_latitude_buf[9];
  char DMH_longitude_buf[10];
  gps_convert_coordinates_to_DMH(DMH_latitude_buf, DMH_longitude_buf);

  //  Get APRS frequency using geofence
  int16_t DD_latitude_buf;
  int16_t DD_longitude_buf;
  gps_convert_coordinates_to_DD(&DD_latitude_buf, &DD_longitude_buf);
  global_freq = geofence_get_aprs_frequency(DD_latitude_buf, DD_longitude_buf);

  // Acquire environmental data
  voltage_get_measurements();
  TEMP_MEASURE;

  /* APRS comment format:

  Sample: 000/000/A=000000/F0N0T0E0Y0S0a0b0c0_XYZ

  Fixed attributes for every packet:
    [VALUE]/[VALUE]   Course [deg] / Speed [knots]
    /A=[VALUE]/       Altitude [feet]
    F[VALUE]          Flight number
    N[VALUE]          Packet counter
    T[VALUE]          Temperature [deg C]
    E[VALUE]          MCU voltage [V*100]
    Y[VALUE]          Solar voltage [V*100]
    S[VALUE]          GNSS-Satellite count

  Optional attributes starting lower case [a-z]:
    a[VALUE]     Additional 0
    b[VALUE]     Additional 1
    d[VALUE]     Additional 2
    ...          ...
  
  Optional additional comment
    _[STRING]    Additional comment */

  // Assemble APRS comment
  char aprs_packet_comment_buf[128];
  sprintf(aprs_packet_comment_buf, "%03d/%03d/A=%06ld/F%dN%dT%dE%dY%dS%d_%s",
  course,
  speed,
  (int32_t) (altitude*3.28084), 
  PAYLOAD_FLIGHT_NUMBER, 
  aprs_packet_counter, 
  TEMP_VAR, 
  mcu_voltage, 
  solar_voltage, 
  satellites, 
  APRS_ADDITIONAL_COMMENT); 

  DEBUG_PRINTLN("[APRS] Send position packet");
  DEBUG_PRINT("[APRS] Freq: ");
  DEBUG_PRINTLN((uint32_t) global_freq);
  DEBUG_PRINT("[APRS] Comment: ");
  DEBUG_PRINTLN(aprs_packet_comment_buf);
  DEBUG_PRINTLN();

  // Send APRS packet
  aprs_send_position_packet(&global_freq, SX1278_TX_POWER, SX1278_DEVIATION, APRS_SOURCE_CALLSIGN, APRS_SOURCE_SSID, DMH_latitude_buf, DMH_longitude_buf, aprs_packet_comment_buf);

  // Increment APRS packet counter
  aprs_packet_counter++;

  GPS_BEGIN_BETWEEN;
}

#if TARGET == TARGET_RS_4
  void main_generate_aprs_image_packet()
  {
    if(image_packet_counter == -1) // Go here after startup
    {
      DEBUG_PRINTLN("[CAM] Capture new image");
      main_capture_image(); // ESP needs to be restarted for next image
      image_packet_counter = 0;
    }
    else if(camera_get_new_packet()) // Check if new image packet available to send
    {
      DEBUG_PRINT("[IMG] Send: ");
      DEBUG_PRINT((uint32_t)global_freq);
      DEBUG_PRINTLN((char*) packet_img_base64_buf);
      DEBUG_PRINTLN();

      // Send APRS packet
      aprs_send_status_packet(&global_freq, SX1278_TX_POWER, SX1278_DEVIATION, APRS_SOURCE_CALLSIGN, IMAGE_APRS_SOURCE_SSID, image_packet_counter, (char*) packet_img_base64_buf); // Send aprs image packet
      image_packet_counter++; // Increment image packet counter
    }
    else // Capture new image after the last one was send
    {
      DEBUG_PRINTLN("[IMG] Last IMG packet send");
      DEBUG_PRINTLN("[CAM] Capture new image");
      main_capture_image(); // ESP needs to be restarted for next image
      image_packet_counter = 0;
    }
  }

  void main_capture_image()
  {
    DEBUG_PRINTLN("[CAM] Init");
    camera_init(); // Initialize camera
    MCU_SET_FREQ_NORMAL; // Clock down MCU to save power

    gps_proccess_for_ms(2000); // Needed for OV2640 to properly adjust brightness

    camera_capture_image(); // Capture new image

    DEBUG_PRINTLN("[CAM] Disable");
    camera_deinit(); // Deinit camera
    camera_disable(); // Disable camera to save power
  }

  #ifdef CACHE_ENABLE
    void main_handle_cache()
    {
      // Get buffer and index from NVS 
      uint16_t index;
      uint16_t element_number;
      char *tmp_buf = cache_get(&element_number);

      // Convert GPS data to deg 
      int16_t DD_latitude_buf;
      int16_t DD_longitude_buf;
      gps_convert_coordinates_to_DD(&DD_latitude_buf, &DD_longitude_buf);

      if(DD_latitude_buf != 0 && DD_longitude_buf != 0) // Add new position to cache
      {
        char latitude_B91_char = round(0.5 * (DD_latitude_buf/100 + 90)) + 33; // Map -90:90to 0:90 and add 33 for ASCII Base91 encoding
        char longitude_B91_char = round(0.25 * (DD_longitude_buf/100 + 180)) + 33; // Map -180:180 to 0:90 and add 33 for ASCII Base91 encoding
        
        DEBUG_PRINT("[MAIN] Cache pos: ");
        DEBUG_PRINT(latitude_B91_char);
        DEBUG_PRINTLN(longitude_B91_char);
        
        if(element_number >= CACHE_LENGTH) index = CACHE_LENGTH; // Set index to last position, if cache full
        else index = element_number;

        if(index == 0) // Always write new postion, if buffer empty
        {
          element_number = cache_push(latitude_B91_char); // Add latitude
          element_number = cache_push(longitude_B91_char); // Add longitude
          cache_store(); // Save updated cache
        }
        else if(latitude_B91_char != tmp_buf[index-2] || longitude_B91_char != tmp_buf[index-1]) // Check if latest position different from current
        {
          if(index >= 4) // If there are more than two positions, also check the second to last position -> this removes oscillations
          {
            if(latitude_B91_char != tmp_buf[index-4] || longitude_B91_char != tmp_buf[index-3])
            {
              element_number = cache_push(latitude_B91_char); // Add latitude
              element_number = cache_push(longitude_B91_char); // Add longitude
              cache_store(); // Save updated cache
            }
          }
          else
          {
            element_number = cache_push(latitude_B91_char); // Add latitude
            element_number = cache_push(longitude_B91_char); // Add longitude
            cache_store(); // Save updated cache
          }
        }
      }

      if(element_number >= CACHE_LENGTH) index = CACHE_LENGTH; // Set index to last position, if cache full
      else index = element_number;

      tmp_buf[index+0] = '|'; // Add end flag
      tmp_buf[index+1] = (element_number / 2) / 90 + 33; // Add ASCII Base91 encoded element number MSB
      tmp_buf[index+2] = (element_number / 2) % 90 + 33; // Add ASCII Base91 encoded element number LSB
      tmp_buf[index+3] = '\0'; // Add null termination

      // Send APRS packet with cache
      aprs_send_status_packet(&global_freq, SX1278_TX_POWER, SX1278_DEVIATION, APRS_SOURCE_CALLSIGN, CACHE_APRS_SOURCE_SSID, APRS_DESTINATION_SSID, tmp_buf); // Send aprs image packet
    }
  #endif
#endif
