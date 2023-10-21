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

#ifndef __CAMERA__H__
#define __CAMERA__H__

#include <Arduino.h>

#include "config.h"

#define IMAGE_PACKET_SSDV_OFFSET 6 // Strip of sync byte, packet type and callsign from SSDV
#define IMAGE_PACKET_BASE64_LENGTH ((IMAGE_PACKET_LENGTH - IMAGE_PACKET_SSDV_OFFSET + 2)/3*4)

extern uint8_t packet_img_base64_buf[IMAGE_PACKET_BASE64_LENGTH];

// Exported functions
void camera_init();
void camera_begin();

void camera_capture_image();

bool camera_get_new_packet();

void camera_enable();
void camera_disable();
void camera_panic();

#endif