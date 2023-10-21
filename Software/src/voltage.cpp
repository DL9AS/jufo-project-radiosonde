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

#include "voltage.h"
#include "config.h"
#include "pins.h"

// Module globals
uint16_t mcu_voltage = 0;
uint16_t solar_voltage = 0;

// Exported functions
#if TARGET == TARGET_RS_1TO3
  // Get mcu and solar voltage
  void voltage_get_measurements()
  {
    // Measure mcu input voltage
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    
    delay(1);
    ADCSRA |= _BV(ADSC);
    while (bit_is_set(ADCSRA,ADSC));

    int32_t raw_voltage = (ADCH<<8) | ADCL;

    raw_voltage = (1125300L / raw_voltage) * INPUT_VOL_CORRECTION_FACTOR;
    mcu_voltage = int32_t (raw_voltage / 10);

    // Measure and calculate solar input voltage
    analogReference(DEFAULT);   // Adc reference = mcu input voltage
    delay(3);
    analogRead(SOLAR_VOL_PIN);  // Ignore first adc reading after changing adc reference
    solar_voltage = uint16_t ( mcu_voltage / 1023.00 * analogRead(SOLAR_VOL_PIN) );
  }

#elif TARGET == TARGET_RS_4
  // Get mcu and solar voltage - TODO: implement this function correctly for RS_4
  void voltage_get_measurements()
  {
    solar_voltage = analogRead(SOLAR_VOL_PIN);
  }
#endif