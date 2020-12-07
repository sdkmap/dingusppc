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

#ifndef ESCC_H
#define ESCC_H

#include <cinttypes>

class ESCC {
public:
    ESCC();
    ~ESCC();

//protected:
    uint8_t escc_read(uint32_t offset, int size);
    void escc_write(uint32_t offset, uint8_t value, int size);

    uint8_t escc_legacy_read(uint32_t offset, int size);
    void escc_legacy_write(uint32_t offset, uint8_t value, int size);

private:
    uint8_t escc_reg[16];

    uint8_t store_reg = 0;

    bool prep_value = false;
};

#endif /* ESCC_H */
