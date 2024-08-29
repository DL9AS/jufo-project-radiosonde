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

#ifndef __CONFIG__H_
#define __CONFIG__H_

#include "defines.h"

/*
 * Target config
 */

  // Choose target:
  // TARGET_RS_1TO3 is compatibly with radiosonde generation 1 to 3
  // TARGET_RS_4 is compatibly with radiosonde generation 4 with camera
  #define TARGET TARGET_RS_4 

/*
 * Payload config
 */

  #define PAYLOAD_FLIGHT_NUMBER 73

  #define WATCHDOG_ENABLE

/*
 * TARGET_RS_4 specific config
 */

  #define PRE_IMG_LOOP_REAPEATS 10

  #define NVS_STATUS NVS_RUNNING // NVS_RUNNING normal mode | NVS_RESET resets the non-volatile file system (development only)

/*
 * Radio protocol config
 */

  #define RADIO_PACKET_DELAY  35000  // Radio packet delay in ms

/*
 * Radio config
 */

  // Set SX1278 NF modulation option
  // MOD_DIO2: use SX1278 DIO2 to generate AFSK modulation
  // MOD_F_HOP: use SX1278 fast frequency hopping to generate AFSK modulation (DIO2 not longer needed)
  #define SX1278_MOD_OPTION MOD_DIO2

  #if SX1278_MOD_OPTION == MOD_DIO2
    #define SX1278_FREQUENCY_CORRECTION -47078 // Frequency offset in Hz
    #define SX1278_TX_POWER 17  // Tx power in dbm (2-20 dbm)
    #define SX1278_DEVIATION 3000 // FSK deviation in Hz
  #elif SX1278_MOD_OPTION == MOD_F_HOP
    #define SX1278_FREQUENCY_CORRECTION -55800 // Frequency offset in Hz
    #define SX1278_TX_POWER 17  // Tx power in dbm (2-20 dbm)
    #define SX1278_DEVIATION 3000 // FSK deviation in Hz
  #endif

  #define SX1278_CRYSTAL_FREQ 32000000 // Crystal frequency in Hz

/*
 * APRS config
 */
 
  #define APRS_SOURCE_CALLSIGN "DL9AS"
  #define APRS_SOURCE_SSID 11
  
  #define APRS_DESTINATION_CALLSIGN "APMON1"
  #define APRS_DESTINATION_SSID 0
  
  #define APRS_DIGIPEATER_ENABLE NO_DIGI // Set to USE_DIGI or NO_DIGI
  #define APRS_DIGIPEATER_CALLSIGN "WIDE1"
  #define APRS_DIGIPEATER_SSID 1

  #define APRS_SYMBOL_OVERLAY '/'
  #define APRS_SYMBOL 'O'
   
  #define APRS_FREQUENCY_DEFAULT     144800000  // Aprs frequency default in Hz
 
  #define APRS_FREQUENCY_REGION1     144800000  // Aprs frequency region1 in Hz
  #define APRS_FREQUENCY_REGION2     144390000  // Aprs frequency region2 in Hz
  #define APRS_FREQUENCY_BRAZIL      145570000  // Aprs frequency brazil in Hz
  #define APRS_FREQUENCY_CHINA       144640000  // Aprs frequency china in Hz
  #define APRS_FREQUENCY_JAPAN       144660000  // Aprs frequency japan in Hz
  #define APRS_FREQUENCY_THAILAND    145525000  // Aprs frequency thailand in Hz
  #define APRS_FREQUENCY_NEWZEALAND  144575000  // Aprs frequency newzealand in Hz
  #define APRS_FREQUENCY_AUSTRALIA   145175000  // Aprs frequency australia in Hz

  #define APRS_FLAGS_AT_BEGINNING 100 // Send x times ax25 flag as TX delay

  // AFSK mark length can differ due to inaccurate MCU clock 
  // For DIO2 mod: 408us 1200Hz and 204us 2400Hz good starting point
  // For SX1278 Fhop mod: 390us 1200Hz and 195us 2400Hz good stating point
  #if SX1278_MOD_OPTION == MOD_DIO2
    #define APRS_1200_MARK_DELAY 412 // Send 1200Hz mark for ~833us, so theoretical a delay between state changes of ~417us
    #define APRS_2400_SPACE_DELAY 206 // Send twice 2400Hz space for ~417us each, so theoretical a delay between state changes of ~208us
  #elif SX1278_MOD_OPTION == MOD_F_HOP
    #define APRS_1200_MARK_DELAY 390 // Send 1200Hz mark for ~833us, so theoretical a delay between state changes of ~417us
    #define APRS_2400_MARK_DELAY 195 // Send twice 2400Hz mark for ~417us each, so theoretical a delay between state changes of ~208s
  #endif

  #define APRS_COMMENT_BUF_SIZE 150

  #define APRS_ADDITIONAL_COMMENT "Ground Test"
  
/*
 * GPS config
 */

  #define GPS_BAUD_RATE 9600 

  #define GPS_PARSE_SENTENCE_GGA "GNGGA" // Use GNGGA for GPS receiver with more than one constellation and GPGGA for GPS receiver with GPS only
  #define GPS_PARSE_SENTENCE_RMC "GNRMC" // Use GNRMC for GPS receiver with more than one constellation and GPRMC for GPS receiver with GPS only
  
/*
 * Camera OV2640 config
 */

  #define OV2640_FRAMESIZE FRAMESIZE_VGA // Choose: 96X96 (96x96), QCIF (176x144), HQVGA (240x176), 240X240 (240x240), QVGA (320x240), HVGA (480x320), VGA (640x480)

  #define OV2640_JPEG_QUALITY 5 // 0-63 -> smaller number means higher image quality

  #define SSDV_JPEG_QUALITY 3 // 0-7 -> higher number means higher image quality

  #define IMAGE_PACKET_LENGTH 195

  #define IMAGE_SSDV_FEED_BUF_LENGTH 128

  #define IMAGE_APRS_SOURCE_SSID 7

/*
 * Environment sensor config
 */

  #define TEMPERATURE_SENSOR SX1278_INTERNAL // SX1278_INTERNAL or DS18B20
  #define SX1278_INTERNAL_TEMP_OFFSET 15 // Offset in K

/*
 * Serial config
 */

  #define DEBUG_SERIAL_ENABLE

  #define DEBUG_SERIAL_BAUD_RATE 115200

/*
 * CACHE config
 */

  #define CACHE_ENABLE

  #define CACHE_APRS_SOURCE_SSID 9

  #define CACHE_RUN_HANDLER_EVERY 15 // Run cache handler after every X position packets
  #define CACHE_LENGTH 250 // Cache length must be an even!

/*
 * Solar config
 */

  #define INPUT_VOL_CORRECTION_FACTOR 1.3455
  
#endif