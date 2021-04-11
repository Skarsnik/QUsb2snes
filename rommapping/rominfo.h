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

#ifndef H_ROM_INFO_H
#define H_ROM_INFO_H
#include <stdint.h>
#include <stdbool.h>

#include "rommapping.h"

/*
https://en.wikibooks.org/wiki/Super_NES_Programming/SNES_memory_map
$xFC0	Game title.	21 bytes, usually uppercase, but there are no limitations to it. Basically every ASCII character between (not including) $1F and $7F is valid.
$xFD5	ROM makeup byte.	xxAAxxxB; AA==11 means FastROM ($30). If B is set, it's HiROM, otherwise it's LoROM.
$xFD6	ROM type.	ROM/RAM/SRAM/DSP1/FX
$xFD7	ROM size.	The size bits for the ROM, as the ROM sees itself. If you do $400<<(ROM size), you get the overall size in byte
$xFD8	SRAM size.	The size bits for the SRAM, RAM that is really fast and can be used by the cartridge for various tasks. Usually though, this memory is just used to save your progress. To get the size again use $400<<(RAM size).
$xFD9	Creator license ID code.
$xFDB	Version #.
$xFDC	Checksum complement.
$xFDE	Checksum.
*/

#ifdef __cplusplus
extern "C" {
#endif


struct rom_infos {
    enum rom_type   type;
    bool            fastrom;
    bool            valid_checksum;
    char            title[22];
    unsigned int    size;
    unsigned int    sram_size;
    uint16_t        creator_id;
    char            version;
    unsigned short  checksum_comp;
    unsigned short  checksum;
};

struct rom_infos* get_rom_info(const char* data);
bool              rom_info_make_sense(struct rom_infos* infos, enum rom_type type);


#ifdef __cplusplus
}
#endif


#endif
