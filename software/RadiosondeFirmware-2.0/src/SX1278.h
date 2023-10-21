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

#ifndef __SX1278__H__
#define __SX1278__H__

#include<Arduino.h>

extern int16_t sx1278_internal_last_temp;

// Exported functions
void SX1278_begin(void);

void SX1278_enable(void);
void SX1278_disable(void);
void SX1278_reset(void);

void SX1278_enable_TX_direct(uint64_t *freq, uint8_t pwr, uint16_t deviation);

void SX1278_mod_direct_out(uint32_t delay);
void SX1278_set_direct__out(bool value);

void SX1278_set_TX_deviation(uint64_t *freq, uint16_t deviation);
void SX1278_set_TX_power(uint8_t pwr, bool pa_boost_mode_20dbm);
void SX1278_set_TX_frequency(uint64_t *freq);

#endif