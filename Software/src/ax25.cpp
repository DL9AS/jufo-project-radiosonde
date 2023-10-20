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

#include "ax25.h"
#include "SX1278.h"
#include "config.h"

// Module globals
uint16_t continuous_crc = 0xFFFF;

bool  rectangle_wave_out_state = true; // State at initialization does not matter
uint8_t consecutive_true_bit_counter = 0; // Used by bit stuffing

// Module functions
static void ax25_set_rectangle_wave_out(bool rectangle_wave_out_state)
{
  if(rectangle_wave_out_state)
  {
    SX1278_mod_direct_out(APRS_1200_MARK_DELAY); // Send 1200Hz mark for ~833us, so theoretical a delay between state changes of ~417us
  }
  else
  {
    SX1278_mod_direct_out(APRS_2400_SPACE_DELAY); // Send first 2400Hz space for ~417us, so theoretical a delay between state changes of ~208s
    SX1278_mod_direct_out(APRS_2400_SPACE_DELAY); // Send second 2400Hz space for ~417us, so theoretical a delay between state changes of ~208us
  }
}

static void ax25_calc_crc(uint16_t *crc, bool bit)
{
  const uint16_t POLY = 0x8408; // The polynomial value used for the CRC calculation
  
  bool crc_lsb_old = *crc & 0x0001; // Get LSB from CRC

  *crc = *crc >> 1; // Shift the CRC value right by one bit

  if(crc_lsb_old != bit) *crc = *crc ^ POLY; // If LSB of CRC before shifting unequal to input but -> XOR with POLY
}

// Exported functions
void ax25_TX_flag(uint8_t len)
{
  for(uint8_t i = 0; i < len; i++) ax25_TX_byte(0x7E, true); // Send len times 0x7E flag
}

void ax25_TX_c_string(const char * c_string, uint16_t len)
{
  for(uint16_t i = 0; i < len; i++) ax25_TX_byte(c_string[i], false);
}

void ax25_TX_byte(byte tx_byte, bool is_flag)
{
  bool working_bit;
    
  for(uint8_t i = 0; i < 8; i++) // Iterate thru all bits
  {
    working_bit = tx_byte & 0x01; // Strip of the rightmost bit to be sent

    ax25_calc_crc(&continuous_crc, working_bit);

    if(working_bit) // Current bit is a true
    {
      ax25_set_rectangle_wave_out(rectangle_wave_out_state); // Send rectangle wave, without flipping state  
      consecutive_true_bit_counter++; // Increment since current bit one

      if(consecutive_true_bit_counter == 5 && !is_flag) // Send an extra true, if 5 true bits in a row and not flag
      {
        rectangle_wave_out_state = !rectangle_wave_out_state; // Current Bit is false, so flip output state
        ax25_set_rectangle_wave_out(rectangle_wave_out_state); // Send rectangle wave, with new state  
        
        consecutive_true_bit_counter = 0; // Reset consecutive true bit counter
      }
    }
    else
    {
      rectangle_wave_out_state = !rectangle_wave_out_state; // Current Bit is false, so flip output state
      ax25_set_rectangle_wave_out(rectangle_wave_out_state); // Send rectangle wave, with new state  

      consecutive_true_bit_counter = 0; // Reset consecutive true bit counter
    }

    tx_byte = tx_byte >> 1; // go to the next bit in the byte
  }
}