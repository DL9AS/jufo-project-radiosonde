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

#ifndef __PINS__H__
#define __PINS__H__

#include "globals.h"
#include "config.h"

#if TARGET == TARGET_RS_1TO3
  // SX1278 hardware pin definitions 
  #define SX1278_NSS 5                                 
  #define SX1278_NRESET 7                                                                                        
  #define SX1278_DIO2 6 

  // DS18B20 hardware pin definitions
  #define DS18B20_OW 2

  // Input voltage measurement
  #define SOLAR_VOL_PIN A3

#elif TARGET == TARGET_RS_4
  // SX1278 hardware pin definitions 
  #define SX1278_NSS 5                                 
  #define SX1278_NRESET 12                                                                                        
  #define SX1278_DIO2 22 

  // DS18B20 hardware pin definitions
  #define DS18B20_OW -1

  // Input voltage measurement
  #define SOLAR_VOL_PIN 37
#endif

// OV2640_ camera hardware pin definitions
#define OV2640_PWEN 21
#define OV2640_PWDN -1  
#define OV2640_RESET -1

#define OV2640_XCLK 13
#define OV2640_SIOD 26
#define OV2640_SIOC 27

#define OV2640_D7 15
#define OV2640_D6 14
#define OV2640_D5 25
#define OV2640_D4 32
#define OV2640_D3 34
#define OV2640_D2 36
#define OV2640_D1 39
#define OV2640_D0 35
#define OV2640_VSYNC 2
#define OV2640_HREF 4
#define OV2640_PCLK 33

#endif