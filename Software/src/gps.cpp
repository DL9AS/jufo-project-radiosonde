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

#include "gps.h"
#include "config.h"
#include "globals.h"

#if TARGET == TARGET_RS_1TO3
  #include "../lib/alt_soft_serial/AltSoftSerial.h" // Used to get second serial port
  AltSoftSerial gps_serial_interface;
#elif TARGET == TARGET_RS_4
  #define gps_serial_interface Serial1
#endif

// Module globals
int dd_lat = 0;
int mm_lat = 0;
int last_mm_lat = 0;
char direction_lat = 'N';

int dd_long = 0;
int mm_long = 0;
int last_mm_long = 0;
char direction_long = 'E';

int quality_indicator = 0;
int satellites = 0;
long int altitude = 0;
char raw_time[10];

int speed = 0;
int course = 0;

// Exported functions
void gps_begin()
{
  MCU_SET_FREQ_RADIO;
  gps_serial_interface.begin(GPS_BAUD_RATE); // Set GPS BAUD rate
  MCU_SET_FREQ_NORMAL;
}

void gps_end()
{
  gps_serial_interface.end();
}

// Sleep function while keeping GPS running
void gps_proccess_for_ms(uint32_t duration_ms)
{
  uint32_t reference_millis = millis();
  char gps_input_buffer[128];

  DEBUG_PRINT("[GPS] Processing GPS for ms: ");
  DEBUG_PRINTLN(duration_ms);
  
  while(millis() < reference_millis + duration_ms) // Proccess GPS for certain duration
  {
    // Reset watchdog
    WDT_RESET;
    
    #if TARGET == TARGET_RS_4
      // Slightly adjust baud rate to compensate for lower MCU clocks
      gps_serial_interface.updateBaudRate(GPS_BAUD_RATE-200);
    #endif
    
    while(gps_serial_interface.available())
    {
      if (gps_serial_interface.find('$')) // NMEA sentence starts with '$'
      {
        gps_serial_interface.readBytesUntil('\n', gps_input_buffer, 128); // Read in single NMEA sentence
        // Parse GNGGA or GPGGA sentence from GPS receiver
        sscanf(gps_input_buffer, GPS_PARSE_SENTENCE_GGA",%10[^,],%2d%2d.%2d%*[^,],%c,%3d%2d.%2d%*[^,],%c,%d,%d,%*[^,],%ld", raw_time, &dd_lat, &mm_lat, &last_mm_lat, &direction_lat, &dd_long, &mm_long, &last_mm_long, &direction_long, &quality_indicator, &satellites, &altitude);
        // Parse GNRMC or GPRMC from GPS receiver
        sscanf(gps_input_buffer, GPS_PARSE_SENTENCE_RMC",%*[^,],%*c,%*[^,],%*c,%*[^,],%*c,%d.%*d,%d", &speed, &course);

        //DEBUG_PRINTLN(gps_input_buffer);
      }
    }
  }
}

/*
 * GNSS coordinates format conversion in degrees, minutes and hundredths of a minute (needed for aprs)
 * N = north; S = south; E = east; W = west
 * more information at: http://www.aprs.org/doc/APRS101.PDF
 */
void gps_convert_coordinates_to_DMH(char *latitude_DMH, char *longitude_DMH)
{
  // Convert ddmm.mmmm lat from GPS to ddmm.hh for APRS
  sprintf(latitude_DMH, "%02d%02d.%02d%c", dd_lat, mm_lat, last_mm_lat, direction_lat);

  // Convert ddmm.mmmm lat from GPS to ddmm.hh for APRS
  sprintf(longitude_DMH, "%03d%02d.%02d%c", dd_long, mm_long, last_mm_long, direction_long);

  DEBUG_PRINT("[GPS] DMH lat: ");
  DEBUG_PRINTLN(latitude_DMH);
  DEBUG_PRINT("[GPS] DMH long: ");
  DEBUG_PRINTLN(longitude_DMH);
}

// Latitude and longitude is stored in decimal degrees * 100
void gps_convert_coordinates_to_DD(int16_t *latitude_DD, int16_t *longitude_DD)
{
  // Calculate latitude in decimal format * 100
  *latitude_DD = dd_lat*100 + mm_lat*5/3 + last_mm_lat/60000;
  if(direction_lat == 'S') *latitude_DD = - *latitude_DD;

  // Calculate longitude in decimal format * 100
  *longitude_DD = dd_long*100 + mm_long*5/3 + last_mm_long/60000;
  if(direction_long == 'W') *longitude_DD = - *longitude_DD;

  DEBUG_PRINT("[GPS] DD lat: ");
  DEBUG_PRINTLN(*latitude_DD);
  DEBUG_PRINT("[GPS] DD long: ");
  DEBUG_PRINTLN(*longitude_DD);
}