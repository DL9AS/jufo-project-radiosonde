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
#if TARGET == TARGET_RS_4
  #include "camera.h"
  // ESP specific includes
  #include "soc/soc.h" // Disable brownout detector
  #include "soc/rtc_cntl_reg.h" // Disable brownout detector
#endif

// Module globals
uint64_t global_freq; // Global APRS frequency

uint16_t aprs_packet_counter = 0;
#if TARGET == TARGET_RS_4
  uint16_t image_packet_counter = 0;
#endif

// Module functions
void main_generate_aprs_position_packet();
#if TARGET == TARGET_RS_4
  void pre_img_loop();
  void main_generate_aprs_image_packet();
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
  
  #if TARGET == TARGET_RS_4
    DEBUG_PRINTLN("[CAM] Begin");
    camera_begin();

    pre_img_loop(); // Run this loop before initializing camera

    DEBUG_PRINTLN("[CAM] Init");
    camera_init(); // Initialize camera
    delay(2000); // Needed for OV2640 to properly adjust brightness
    camera_capture_image(); // Capture new image

    DEBUG_PRINTLN("[CAM] Disable");
    camera_disable(); // Disable camera to save power
  #endif
}

void loop()
{
  gps_proccess_for_ms(RADIO_PACKET_DELAY); // Sleep with GPS running

  main_generate_aprs_position_packet();

  #if TARGET == TARGET_RS_4
    gps_proccess_for_ms(RADIO_PACKET_DELAY); // Sleep with GPS running

    main_generate_aprs_image_packet();
  #endif
}

#if TARGET == TARGET_RS_4
  void pre_img_loop() // Loop for actions before camera initialized
  {
    for(int i = 0; i < PRE_IMG_LOOP_REAPEATS; i++)
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

  Sample: /A=000000/F0N0T0E0Y0V0C0S0a0b0c0_XYZ

  Fixed attributes for every packet:
    /A=[VALUE]/  Altitude [feet]
    F[VALUE]     Flight number
    N[VALUE]     Packet counter
    T[VALUE]     Temperature [deg C]
    E[VALUE]     MCU voltage [V*100]
    Y[VALUE]     Solar voltage [V*100]
    V[VALUE]     Speed [km/h]
    C[VALUE]     Course [deg]
    S[VALUE]     Satellite count

  Optional attributes starting lower case [a-z]:
    a[VALUE]     Additional 0
    b[VALUE]     Additional 1
    d[VALUE]     Additional 2
    ...          ...
  
  Optional additional comment
    _[STRING]    Additional comment */

  // Assemble APRS comment
  char aprs_packet_comment_buf[128];
  sprintf(aprs_packet_comment_buf, "/A=%06ld/F%dN%dT%dE%dY%dV%dC%dS%d_%s",
  (long int) (altitude*3.28084), 
  PAYLOAD_FLIGHT_NUMBER, 
  aprs_packet_counter, 
  TEMP_VAR, 
  mcu_voltage, 
  solar_voltage, 
  (int) (speed*1.852),
  course, 
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
    if(camera_get_new_packet()) // Check if new image packet available to send
    {
      DEBUG_PRINT("[IMG] Send: ");
      DEBUG_PRINTLN((char*) packet_img_base64_buf);
      DEBUG_PRINTLN();

      aprs_send_status_packet(&global_freq, SX1278_TX_POWER, SX1278_DEVIATION, APRS_SOURCE_CALLSIGN, 10, image_packet_counter, (char*) packet_img_base64_buf); // Send aprs image packet
      image_packet_counter++; // Increment image packet counter
    }
    else
    {
      DEBUG_PRINTLN("[CAM] Last IMG packet send");
      camera_panic(); // ESP needs to be restarted for next image 
    }
  }
#endif
