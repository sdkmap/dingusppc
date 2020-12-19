/*
DingusPPC - The Experimental PowerPC Macintosh emulator
Copyright (C) 2018-20 divingkatae and maximum
                      (theweirdo)     spatium

(Contact divingkatae#1017 or powermax#2286 on Discord for more info)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "escc.h"
#include <cinttypes>
#include <thirdparty/loguru/loguru.hpp>

using namespace std;


ESCC::ESCC() {}

ESCC::~ESCC() {}

uint8_t ESCC::escc_read(bool is_legacy, uint32_t offset, int size) {
    return 0;
}

void ESCC::escc_write(bool is_legacy, uint32_t offset, uint8_t value, int size) {
    if (is_legacy) {
        if (offset == 0x02) {
            if (!prep_value) {
                store_reg  = value & 0xF;
                prep_value = true;
            } else {
                escc_reg[store_reg] = value;
                prep_value          = false;
            }
        }

    } 
    else {
        if (offset == 0x20) {
            if (!prep_value) {
                store_reg  = value & 0xF;
                prep_value = true;
            } else {
                escc_reg[store_reg] = value;
                prep_value          = false;
            }
        }
    }
}