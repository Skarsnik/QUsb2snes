/*
 * Copyright (c) 2021 the QUsb2Snes authors.
 *
 * This file is part of QUsb2Snes.
 * (see https://github.com/Skarsnik/QUsb2snes).
 *
 * QUsb2Snes is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QUsb2Snes is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QUsb2Snes.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "rominfo.h"
#include <string.h>
#include <stdlib.h>

struct rom_infos*    get_rom_info(const char* datain)
{
    const unsigned char* data = datain;
    struct rom_infos* toret = (struct rom_infos*) malloc(sizeof(struct rom_infos));
    memcpy(toret->title, data, 21);
    toret->title[21] = 0;
    toret->type = LoROM;
    toret->fastrom = (data[21] & 0b00110000) == 0b00110000;
    if (data[21] & 1)
        toret->type = HiROM;
    if ((data[21] & 0b00000111) == 0b00000111)
        toret->type = ExHiROM;
    toret->size = 0x400 << data[23];
    toret->sram_size = 0x400 << data[24];
    toret->creator_id =  (data[26] << 8) | data[25];
    toret->version = data[27];
    toret->checksum_comp = (data[29] << 8) | data[28];
    toret->checksum = (data[31] << 8) | data[30];
    toret->valid_checksum = false;
        if ((toret->checksum ^ toret->checksum_comp) == 0xFFFF)
            toret->valid_checksum = true;
    return toret;
}

bool rom_info_make_sense(struct rom_infos *infos, enum rom_type type)
{
    if (!(infos->type == type || (type == HiROM && infos->type == ExHiROM)))
        return false;
    return infos->valid_checksum;
}
