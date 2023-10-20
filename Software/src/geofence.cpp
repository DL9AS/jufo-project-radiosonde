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

#include "geofence.h"
#include "config.h" 
#include "globals.h"

#if TARGET == TARGET_RS_1TO3
    #include <avr/pgmspace.h>
    #define PROGMEM_CUSTOM PROGMEM
#elif TARGET == TARGET_RS_4
    #define PROGMEM_CUSTOM
#endif

// Module globals
// Latitude and longitude is stored in decimal degrees *100
const PROGMEM_CUSTOM int16_t region1_vertices_lat[] = {
    2829,
    4458,
    7411,
    7876,
    6160
};

const PROGMEM_CUSTOM int16_t region1_vertices_long[] = {  
    6029,
    -17,
    1423,
    10212,
    -17806
};

const PROGMEM_CUSTOM int16_t region2_vertices_lat[] = {
    732,
    -1086,
    -2066,
    -1565,
    3485,
    6004,
    7083,
    8171,
    8408,
    8375,
    6125,
    3456
};

const PROGMEM_CUSTOM int16_t region2_vertices_long[] = {  
    -7910,
    -6539,
    -7102,
    -11285,
    -14942,
    -16664,
    -16875,
    -15364,
    -8754,
    -5977,
    -6047,
    -4535
};

const PROGMEM_CUSTOM int16_t brazil_vertices_lat[] = {
    -3566,
    -3349,
    -2152,
    -236,
    1144,
    1023,
    -148
};

const PROGMEM_CUSTOM int16_t brazil_vertices_long[] = {  
    -4470,
    -6368,
    -7915,
    -7686,
    -6649,
    -4276,
    -2870
};

const PROGMEM_CUSTOM int16_t china_vertices_lat[] = {
    1730,
    1747,
    2307,
    3488,
    4508,
    5128,
    5095,
    5073,
    5073,
    4420,
    2468
};

const PROGMEM_CUSTOM int16_t china_vertices_long[] = {  
    11093,
    9792,
    8157,
    7313,
    7665,
    8966,
    10144,
    11690,
    12446,
    13185,
    12306
};

const PROGMEM_CUSTOM int16_t japan_vertices_lat[] = {
    4174,
    3885,
    3663,
    3557,
    3236,
    3041,
    3229,
    3844,
    4555,
    5330,
    5449
};

const PROGMEM_CUSTOM int16_t japan_vertices_long[] = {  
    13782,
    13677,
    13395,
    13026,
    12859,
    13457,
    14740,
    15452,
    15162,
    14749,
    14213
};

const PROGMEM_CUSTOM int16_t thailand_vertices_lat[] = {
    772,
    667,
    781,
    1715,
    2138,
    2429,
    2847,
    2738,
    2413
};

const PROGMEM_CUSTOM int16_t thailand_vertices_long[] = {  
    9743,
    10507,
    11360,
    11175,
    10859,
    10200,
    9760,
    9233,
    8890
};

const PROGMEM_CUSTOM int16_t newzealand_vertices_lat[] = {
    -5163,
    -4893,
    -4460,
    -3979,
    -3330,
    -3016,
    -4729
};

const PROGMEM_CUSTOM int16_t newzealand_vertices_long[] = {  
    16698,
    17366,
    17859,
    17999,
    17771,
    16435,
    15591
};

const PROGMEM_CUSTOM int16_t australia_vertices_lat[] = {
    -764,
    -1513,
    -3352,
    -4086,
    -4256,
    -4073,
    -3590,
    -2414,
    -1249,
    -283
};

const PROGMEM_CUSTOM int16_t australia_vertices_long[] = {  
    12441,
    10990,
    11008,
    11439,
    13864,
    14919,
    15385,
    16035,
    15754,
    14321
};

// Module functions

// check if point is in geographic region - latitude and longitude in deg
// function based on example from W. Randolph Franklin - https://wrf.ecse.rpi.edu/Research/Short_Notes/pnpoly.html
static bool check_if_point_is_in_geographic_region (uint8_t number_of_vertices, const int16_t *vertices_latitude_degrees_list, const int16_t *vertices_longitude_list, int16_t test_point_latitude, int16_t test_point_longitude)
{
  bool c = 0;
  int8_t i, j = 0;
  for (i = 0, j = number_of_vertices-1; i < number_of_vertices; j = i++) {
    if ( (((int16_t) pgm_read_word_near(vertices_longitude_list + i)>test_point_longitude) != ((int16_t) pgm_read_word_near(vertices_longitude_list + j)>test_point_longitude)) &&
	 (test_point_latitude < ((int16_t) pgm_read_word_near(vertices_latitude_degrees_list + j)-(int16_t) pgm_read_word_near(vertices_latitude_degrees_list + i)) * (test_point_longitude-(int16_t) pgm_read_word_near(vertices_longitude_list + i)) / ((int16_t) pgm_read_word_near(vertices_longitude_list + j)-(int16_t) pgm_read_word_near(vertices_longitude_list + i)) + (int16_t) pgm_read_word_near(vertices_latitude_degrees_list + i)) )
       c = !c;
  }
  return c;
}

// Exported functions

// Get aprs frequency depending on the region - latitude and longitude in decimal degrees *100
uint32_t geofence_get_aprs_frequency(int16_t gps_latitude, int16_t gps_longitude)
{
    // Invalid gnss position
    if(gps_latitude == 0 && gps_longitude == 0) return APRS_FREQUENCY_DEFAULT;
    
    // Australia
    if (check_if_point_is_in_geographic_region (sizeof(australia_vertices_lat)/sizeof(australia_vertices_lat[0]), australia_vertices_lat, australia_vertices_long, gps_latitude, gps_longitude)) return APRS_FREQUENCY_AUSTRALIA;
    
    // Newzealand
    if (check_if_point_is_in_geographic_region (sizeof(newzealand_vertices_lat)/sizeof(newzealand_vertices_lat[0]), newzealand_vertices_lat, newzealand_vertices_long, gps_latitude, gps_longitude)) return APRS_FREQUENCY_NEWZEALAND;
    
    // Thailand
    if (check_if_point_is_in_geographic_region (sizeof(thailand_vertices_lat)/sizeof(thailand_vertices_lat[0]), thailand_vertices_lat, thailand_vertices_long, gps_latitude, gps_longitude)) return APRS_FREQUENCY_THAILAND;
    
    // Japan
    if (check_if_point_is_in_geographic_region (sizeof(japan_vertices_lat)/sizeof(japan_vertices_lat[0]), japan_vertices_lat, japan_vertices_long, gps_latitude, gps_longitude)) return APRS_FREQUENCY_JAPAN;
    
    // China
    if (check_if_point_is_in_geographic_region (sizeof(china_vertices_lat)/sizeof(china_vertices_lat[0]), china_vertices_lat, china_vertices_long, gps_latitude, gps_longitude)) return APRS_FREQUENCY_CHINA;
    
    // Brazil
    if (check_if_point_is_in_geographic_region (sizeof(brazil_vertices_lat)/sizeof(brazil_vertices_lat[0]), brazil_vertices_lat, brazil_vertices_long, gps_latitude, gps_longitude)) return APRS_FREQUENCY_BRAZIL;
    
    // Region2
    if (check_if_point_is_in_geographic_region (sizeof(region2_vertices_lat)/sizeof(region2_vertices_lat[0]), region2_vertices_lat, region2_vertices_long, gps_latitude, gps_longitude)) return APRS_FREQUENCY_REGION2;
    
    // Region1
    if (check_if_point_is_in_geographic_region (sizeof(region1_vertices_lat)/sizeof(region1_vertices_lat[0]), region1_vertices_lat, region1_vertices_long, gps_latitude, gps_longitude)) return APRS_FREQUENCY_REGION1;

    // If no position found -> transmit on default frequency
    return APRS_FREQUENCY_DEFAULT;
}
 