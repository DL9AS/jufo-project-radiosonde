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

#include "cache.h"
#include "config.h"
#include "globals.h"
#include <Preferences.h> // Non-volatile storage

// Module globals
Preferences* p_cache_preferences;
uint16_t element_number;
char cache_buf[CACHE_LENGTH+4];

// Exported functions
void cache_begin(Preferences* p_pref)
{
  p_cache_preferences = p_pref;
}

char* cache_get(uint16_t* index)
{
  p_cache_preferences->getBytes("c_buf", cache_buf, CACHE_LENGTH); // Read cache from NVS
  element_number = p_cache_preferences->getUShort("c_id", 0); // Read cache index from NVS

  DEBUG_PRINT("[CACHE] READ: ");
  DEBUG_PRINTLN(cache_buf);

  *index = element_number;
  return cache_buf;
}

void cache_store(void)
{
  p_cache_preferences->putBytes("c_buf", cache_buf, CACHE_LENGTH); // Write cache to NVS
  p_cache_preferences->putUShort("c_id", element_number); // Write cache index to NVS
}

u_int16_t cache_push(char c)
{
  if(element_number >= CACHE_LENGTH)
  {
    memmove(&cache_buf[0], &cache_buf[1], CACHE_LENGTH*sizeof(char)); // Shift all elements left by one
    cache_buf[CACHE_LENGTH-1] = c; // Append new element to the end
  }
  else
  {
    cache_buf[element_number] = c; // Append new element at index
  }
  element_number++;

  return element_number;
}