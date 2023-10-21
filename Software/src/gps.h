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

#ifndef __GPS__H__
#define __GPS__H__

#include <Arduino.h>

extern int quality_indicator;
extern int satellites;
extern long int altitude;
extern char raw_time[10];

extern int speed;
extern int course;

// Exported functions
void gps_begin();
void gps_end();
void gps_proccess_for_ms(uint32_t duration_ms);

void gps_convert_coordinates_to_DMH(char* latitude_DMH, char* longitude_DMH);
void gps_convert_coordinates_to_DD(int16_t *latitude_DD, int16_t *longitude_DD);

#endif
