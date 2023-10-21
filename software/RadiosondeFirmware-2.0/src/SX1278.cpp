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

#include "SX1278.h"
#include "SX1278_register.h"
#include "pins.h"
#include "config.h"
#include "defines.h"
#include "globals.h"

// Module globals
int16_t sx1278_internal_last_temp = 0;
#if SX1278_MOD_OPTION == MOD_F_HOP
  uint8_t freq_lsb_lo;
  uint8_t freq_lsb_hi;
#endif

// Module functions
static void SX1278_write_reg(uint8_t address, uint8_t reg_value)
{
  digitalWrite(SX1278_NSS, LOW);  // Select SX1278 SPI device
  SPI.transfer(address | 0x80);             
  SPI.transfer(reg_value);
  digitalWrite(SX1278_NSS, HIGH);  // Clear SX1278 SPI device selection
}

static uint8_t SX1278_read_reg(uint8_t address)
{
  digitalWrite(SX1278_NSS, LOW);  // Select SX1278 SPI device
  SPI.transfer(address & 0x7F);
  uint8_t reg_value = SPI.transfer(0);
  digitalWrite(SX1278_NSS, HIGH);  // Clear SX1278 SPI device selection

  return reg_value;
}

// This measurement must be performed with SX1278 in Fsk mode!
static void SX1278_measure_internal_temperature(int16_t *temp)
{
  // Set TEMP_MONITOR_OFF (LSB of REG_IMAGE_CAL) to 0 -> enable temperature sensor
  SX1278_write_reg(REG_IMAGE_CAL, 0x82);
  delayMicroseconds(140);
  // Set TEMP_MONITOR_OFF (LSB of REG_IMAGE_CAL) to 1 -> disable temperature sensor
  SX1278_write_reg(REG_IMAGE_CAL, 0x83);

  uint8_t temp_raw;
  temp_raw = SX1278_read_reg(REG_TEMP);

  *temp = ((int8_t) (255 - temp_raw)) + SX1278_INTERNAL_TEMP_OFFSET;

  DEBUG_PRINT("[SX1278] Internal temp: ");
  DEBUG_PRINTLN(*temp);
}

// Exported functions
void SX1278_begin(void)
{
  SPI.begin(); // Begin SPI communication

  pinMode(SX1278_NSS, OUTPUT);
  digitalWrite(SX1278_NSS, HIGH);  // Clear SX1278 SPI device selection

  pinMode(SX1278_NRESET, OUTPUT);
  SX1278_enable();

  pinMode(SX1278_DIO2, OUTPUT); // Set direct modulation pin as output
}

void SX1278_enable(void)
{
  digitalWrite(SX1278_NRESET, HIGH); // Enable SX1278
}

void SX1278_disable(void)
{
  digitalWrite(SX1278_NRESET, LOW); // Disable SX1278
}

void SX1278_reset(void)
{
  digitalWrite(SX1278_NRESET, LOW); // Disable SX1278
  delay(5);
  digitalWrite(SX1278_NRESET, HIGH); // Enable SX1278
  delay(5);
}

void SX1278_enable_TX_direct(uint64_t *freq, uint8_t pwr, uint16_t deviation)
{
  SX1278_set_TX_frequency(freq);
  
  // Set packet mode config
  // 7: unused
  // 6: Data Mode 0->Continuos mode
  // 5: Io Home On 0->off
  // 4: Io Home Pwr Frame 0->off
  // 3: Beacon On 0->off
  // 2-0: Payload length MSB
  SX1278_write_reg(REG_DETECT_OPTIMIZE, 0x00);
  
  // Set SX1278 operating mode
  // 7: 0->FSK/OOK Mode
  // 6-5: 00->FSK
  // 4: 0 (reserved)
  // 3: 1->Low Frequency Mode
  // 2-0: 011->Transmitter Mode (TX)
  SX1278_write_reg(REG_OP_MODE, 0x0B);

  #if SX1278_MOD_OPTION == MOD_F_HOP
    SX1278_write_reg(REG_PLL_HOP, 0xAD); // Set Fast_Hop_On (MSB bit) true to enable fast freq hopping
  #endif

  SX1278_measure_internal_temperature(&sx1278_internal_last_temp);

  if(pwr <= 15) SX1278_set_TX_power(pwr, false); // For PWR between 2-15: Do not use the +20dBm option on PA_BOOT
  else SX1278_set_TX_power(pwr, true); // For PWR between 5-20: Enable the +20dBm option on PA_BOOT
  
  SX1278_set_TX_deviation(freq, SX1278_DEVIATION);
}

void SX1278_mod_direct_out(uint32_t delay)
{
  #if SX1278_MOD_OPTION == MOD_DIO2
    // Use DIO2 on SX1278 as direct modulation output pin
    digitalWrite(SX1278_DIO2, HIGH);
    delayMicroseconds(delay);
    digitalWrite(SX1278_DIO2, LOW);
    delayMicroseconds(delay);
  
  #elif SX1278_MOD_OPTION == MOD_F_HOP
    // Use frequency hopping for modulation
    // For fast frequency change, only change Frf_LSB of the 3 Frf bytes
    SX1278_write_reg(REG_FR_LSB, freq_lsb_lo);
    delayMicroseconds(delay);
    SX1278_write_reg(REG_FR_LSB, freq_lsb_hi);
    delayMicroseconds(delay);
  #endif
}

void SX1278_set_direct__out(bool value)
{
  // Use DIO2 on SX1278 as direct modulation output pin
  digitalWrite(SX1278_DIO2, value);
}

void SX1278_set_TX_deviation(uint64_t *freq, uint16_t deviation)
{
  #if SX1278_MOD_OPTION == MOD_DIO2
    // Set TX deviation in Hz
    SX1278_write_reg(REG_FDEV_LSB, deviation / (SX1278_CRYSTAL_FREQ >> 19)); // Freg = Fdev / (Fxo / 2^19)  
  
  #elif SX1278_MOD_OPTION == MOD_F_HOP
    // Frf_MID and Frf_MSB must be set first using SX1278_set_TX_frequency()!

    // Set original TX deviation reg to 0 Hz -> not used here
    SX1278_write_reg(REG_FDEV_LSB, 0 / (SX1278_CRYSTAL_FREQ >> 19)); // Freg = Fdev / (Fxo / 2^19)  

    // Frequency value is calculate by:
    // Freg = (Frf * 2^19) / Fxo
    // Resolution is 61.035 Hz if Fxo = 32 MHz
    // Frf_LSB for high an low FSK frequency needed for fast frequency hopping
    freq_lsb_lo = (uint32_t) ((*freq + SX1278_FREQUENCY_CORRECTION - deviation / 2)  << 19) / SX1278_CRYSTAL_FREQ; // Frf_LSB with carrier frequency + deviation
    freq_lsb_hi = (uint32_t) ((*freq + SX1278_FREQUENCY_CORRECTION + deviation / 2)  << 19) / SX1278_CRYSTAL_FREQ; // Frf_LSB with carrier frequency + deviation
  #endif
}

void SX1278_set_TX_power(uint8_t pwr, bool pa_boost_mode_20dbm)
{
  uint8_t TX_out;

  if (pa_boost_mode_20dbm)
  {
    SX1278_write_reg(REG_PA_DAC, 0x87); // Enable the +20dBm option on PA_BOOT
    TX_out = pwr - 5; // TX out is between 0-15 (2bit) -> add 5 to Tx out to get dbm
  }
  else
  {
    SX1278_write_reg(REG_PA_DAC, 0x84); // Do not use the +20dBm option on PA_BOOT
    TX_out = pwr - 2; // TX out is between 0-15 (2bit) -> add 2 to Tx out to get dbm
  }

  // PA selection and output power control
  // 7-5: PA select 0 -> RFO pin (14dbm) | 1 -> PA_BOOST pin (20dbm)
  // 6-4: Max Pwr (Pmaxreg = (Pmax - 10.8) / 0.6) -> set to max
  // 3-0: Out Pwr
  SX1278_write_reg(REG_PA_CONFIG, (0xF0 | TX_out)); 
}

void SX1278_set_TX_frequency(uint64_t *freq)
{
  // Frequency value is calculate by:
  // Freg = (Frf * 2^19) / Fxo
  // Resolution is 61.035 Hz if Fxo = 32 MHz
  *freq = (((*freq + SX1278_FREQUENCY_CORRECTION) << 19) / SX1278_CRYSTAL_FREQ);

  SX1278_write_reg(REG_FR_MSB, *freq >> 16);  // Write MSB of RF carrier freq
  SX1278_write_reg(REG_FR_MID, *freq >> 8);  // Write MID of RF carrier freq
  SX1278_write_reg(REG_FR_LSB, *freq);  // Write LSB of RF carrier freq
}