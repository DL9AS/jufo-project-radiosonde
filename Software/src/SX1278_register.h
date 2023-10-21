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

#ifndef __SX1278_REGISTER__H__
#define __SX1278_REGISTER__H__

#define REG_DETECT_OPTIMIZE 0x31
#define REG_OP_MODE 0x01

#define REG_FR_MSB 0x06
#define REG_FR_MID 0x07
#define REG_FR_LSB 0x08

#define REG_PA_DAC 0x4D
#define REG_PA_CONFIG 0x09

#define REG_FDEV_MSB 0x04
#define REG_FDEV_LSB 0x05

#define REG_IMAGE_CAL 0x3B
#define REG_TEMP 0x3C

#define REG_PLL_HOP 0x44

#endif