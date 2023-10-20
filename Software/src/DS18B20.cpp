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

#include "DS18B20.h"
#include "DS18B20_commands.h"
#include "config.h"
#include "globals.h"
#include "pins.h"

#include "../lib/one_wire/OneWire.h"

// Module globals
OneWire ds18b20_one_wire(DS18B20_OW);
int8_t ds18b20_last_temp = 0;

// Exported functions
void DS18B20_begin()
{
  ds18b20_one_wire.reset();
  ds18b20_one_wire.write(DS18B20_SKIP_ROM); // Saves time in a single drop bus system by allowing the bus master to access the memory functions without providing the 64-bit ROM code

  // Start temperature conversion for next measurement and set wire high for parasitically powered devices
  ds18b20_one_wire.write(DS18B20_CONVERT_T, true);
}

void DS18B20_temp_measure()
{
  int16_t raw_temp;

  ds18b20_one_wire.reset();
  ds18b20_one_wire.write(DS18B20_SKIP_ROM); // Saves time in a single drop bus system by allowing the bus master to access the memory functions without providing the 64-bit ROM code

  // Start reading the last temperature measurement from scratchpad
  ds18b20_one_wire.write(DS18B20_READ_SCRATCHPAD); 
  ds18b20_one_wire.read_bytes((uint8_t*) &raw_temp, 2); // Read 16bit from scratchpad [Sign 5-bit][Temp 11-bit] (16bit sign-extended twos complement format)

  ds18b20_one_wire.reset();
  ds18b20_one_wire.write(DS18B20_SKIP_ROM); // Saves time in a single drop bus system by allowing the bus master to access the memory functions without providing the 64-bit ROM code

  ds18b20_one_wire.write(DS18B20_CONVERT_T, true); // Start temperature conversion for next measurement and set wire high for parasitically powered devices

  ds18b20_last_temp = raw_temp * 0.0625; // With default 12bit reading temperature resolution is 0.0625

  DEBUG_PRINT("[DS18B20] Temp: ");
  DEBUG_PRINTLN(ds18b20_last_temp);
}