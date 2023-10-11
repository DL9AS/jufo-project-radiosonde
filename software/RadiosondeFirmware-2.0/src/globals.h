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

#ifndef __GLOBALS__H__
#define __GLOBALS__H__

#include <Arduino.h>

#include "config.h"

#ifdef DEBUG_SERIAL_ENABLE
  #define DEBUG_PRINT(x)  Serial.print (x)
  #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

#if TARGET == TARGET_RS_1TO3
  // Atmega 328p can not change operating frequency dynamically
  #define MCU_SET_FREQ_NORMAL
  #define MCU_SET_FREQ_RADIO
  #define MCU_SET_FREQ_CAMERA
#elif TARGET == TARGET_RS_4
  #define MCU_SET_FREQ_NORMAL setCpuFrequencyMhz(10) // Least power consumption
  #define MCU_SET_FREQ_RADIO setCpuFrequencyMhz(80)  // Higher clock for more accurate AFSK timing
  #define MCU_SET_FREQ_CAMERA setCpuFrequencyMhz(80)  // Higher clock for complex image routine
#endif

#if TARGET == TARGET_RS_1TO3
  #define GPS_BEGIN_BETWEEN gps_begin()
  #define GPS_END_BETWEEN gps_end()
#elif TARGET == TARGET_RS_4
  #define GPS_BEGIN_BETWEEN
  #define GPS_END_BETWEEN
#endif

#if defined(WATCHDOG_ENABLE) && TARGET == TARGET_RS_1TO3
  #include <avr/wdt.h>
  #define WDT_INIT wdt_enable(WDTO_8S)
  #define WDT_RESET wdt_reset()
#elif defined(WATCHDOG_ENABLE) && TARGET == TARGET_RS_4
  #include <esp_task_wdt.h>
  #define WDT_INIT esp_task_wdt_init(30, true); esp_task_wdt_add(NULL) // Set WDT timeout to 30s
  #define WDT_RESET esp_task_wdt_reset()
#else
  #define WDT_INIT
  #define WDT_RESET
#endif

#if TEMPERATURE_SENSOR == DS18B20
  #include "DS18B20.h"
  #define TEMP_BEGIN DS18B20_begin()
  #define TEMP_MEASURE DS18B20_temp_measure()
  #define TEMP_VAR ds18b20_last_temp
#elif TEMPERATURE_SENSOR == SX1278_INTERNAL
  #define TEMP_BEGIN
  #define TEMP_MEASURE
  #define TEMP_VAR sx1278_internal_last_temp
#endif

#endif