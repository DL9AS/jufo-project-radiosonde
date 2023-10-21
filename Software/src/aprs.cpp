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

#include "aprs.h"
#include "globals.h"
#include "SX1278.h"
#include "ax25.h"
#include "config.h"
#include "defines.h"

// Exported functions
void aprs_send_position_packet(uint64_t *freq, uint8_t pwr, uint32_t deviation,const char *src_call, uint8_t src_SSID, char *latitude, char *longitude, char *comment) 
{
  MCU_SET_FREQ_RADIO; // Clock up MCU for more accurate AFSK timing
  
  SX1278_enable_TX_direct(freq, pwr, deviation);
  
  // Send flag often times at start
  ax25_TX_flag(APRS_FLAGS_AT_BEGINNING);

  continuous_crc = 0xFFFF; // Set CRC at start to 0xFFFF, this is standard in AX25
  
  // Send destination callsign [6 bytes Call][1 byte SSID] and left shifted by 1
  char buf[8];
  sprintf(buf, "%-6s%c", APRS_DESTINATION_CALLSIGN, APRS_DESTINATION_SSID);
  for(uint8_t i = 0; i < 7; i++) ax25_TX_byte(buf[i] << 1, false);

  // Send source callsign [6 bytes Call][1 byte SSID] and left shifted by 1
  sprintf(buf, "%-6s%c", src_call, src_SSID);
  for(uint8_t i = 0; i < 6; i++) ax25_TX_byte(buf[i] << 1, false);
  #if APRS_DIGIPEATER_ENABLE == USE_DIGI
    ax25_TX_byte((buf[6]  << 1), false);
  #else
    ax25_TX_byte((buf[6]  << 1) | 1, true); // Set LSB to 1, to indicate end of header
  #endif

  #if APRS_DIGIPEATER_ENABLE == USE_DIGI
    // Send digipeater callsign [6 bytes Call][1 byte SSID], left shifted by 1 and SSID + 1
    sprintf(buf, "%-6s%c", APRS_DIGIPEATER_CALLSIGN, APRS_DIGIPEATER_SSID);
    for(uint8_t i = 0; i < 6; i++) ax25_TX_byte(buf[i] << 1, false);
    ax25_TX_byte((buf[6]  << 1) | 1, true); // Set LSB to 1, to indicate end of header
  #endif

  // Send control field and protocol field
  ax25_TX_byte(0x03, false);
  ax25_TX_byte(0xf0, false);

  // Send '!' for uncompressed position packet
  ax25_TX_byte('!', false);
  
  // Send latitude
  ax25_TX_c_string(latitude, 8);

  // Send symbol table id
  ax25_TX_byte(APRS_SYMBOL_OVERLAY, false);

  // Send latitude
  ax25_TX_c_string(longitude, 9);

  // Send symbol code
  ax25_TX_byte(APRS_SYMBOL, false);

  // Send comment
  ax25_TX_c_string(comment, strlen(comment));

  // Send CRC LSB and MSB
  byte crc_lsb = continuous_crc;
  byte crc_msb = continuous_crc >> 8;
  ax25_TX_byte(crc_lsb ^ 0xFF, false);
  ax25_TX_byte(crc_msb ^ 0xFF, false);

  // Send flag at end
  ax25_TX_flag(3);

  SX1278_sleep();

  MCU_SET_FREQ_NORMAL; // Clock down MCU to save power
}

#if TARGET == TARGET_RS_4
  void aprs_send_status_packet(uint64_t *freq, uint8_t pwr, uint32_t deviation, const char *src_call, uint8_t src_SSID, uint8_t dest_SSID, char *comment) 
  {
    MCU_SET_FREQ_RADIO; // Clock up MCU for more accurate AFSK timing
    
    SX1278_enable_TX_direct(freq, pwr, deviation);
    
    // Send flag often times at start
    ax25_TX_flag(APRS_FLAGS_AT_BEGINNING);

    continuous_crc = 0xFFFF; // Set CRC at start to 0xFFFF, this is standard in AX25
    
    // Send destination callsign [6 bytes Call][1 byte SSID] and left shifted by 1
    char buf[8];
    sprintf(buf, "%-6s%c", APRS_DESTINATION_CALLSIGN, dest_SSID);
    for(uint8_t i = 0; i < 7; i++) ax25_TX_byte(buf[i] << 1, false);
    
    // Send source callsign [6 bytes Call][1 byte SSID] and left shifted by 1
    sprintf(buf, "%-6s%c", src_call, src_SSID);
    for(uint8_t i = 0; i < 6; i++) ax25_TX_byte(buf[i] << 1, false);
    #if APRS_DIGIPEATER_ENABLE == USE_DIGI
      ax25_TX_byte((buf[6]  << 1), false);
    #else
      ax25_TX_byte((buf[6]  << 1) | 1, true); // Set LSB to 1, to indicate end of header
    #endif

    #if APRS_DIGIPEATER_ENABLE == USE_DIGI
      // Send digipeater callsign [6 bytes Call][1 byte SSID], left shifted by 1 and SSID + 1
      sprintf(buf, "%-6s%c", APRS_DIGIPEATER_CALLSIGN, APRS_DIGIPEATER_SSID);
      for(uint8_t i = 0; i < 6; i++) ax25_TX_byte(buf[i] << 1, false);
      ax25_TX_byte((buf[6]  << 1) | 1, true); // Set LSB to 1, to indicate end of header
    #endif
    
    // Send control field and protocol field
    ax25_TX_byte(0x03, false);
    ax25_TX_byte(0xf0, false);

    // Send '!' for uncompressed position packet
    ax25_TX_byte('>', false);

    // Send comment
    ax25_TX_c_string(comment, strlen(comment));

    // Send CRC LSB and MSB
    byte crc_lsb = continuous_crc;
    byte crc_msb = continuous_crc >> 8;
    ax25_TX_byte(crc_lsb ^ 0xFF, false);
    ax25_TX_byte(crc_msb ^ 0xFF, false);

    // Send flag at end
    ax25_TX_flag(3);

    SX1278_sleep();

    MCU_SET_FREQ_NORMAL; // Clock down MCU to save power
  }
#endif