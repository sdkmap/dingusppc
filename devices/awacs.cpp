#include <iostream>
#include "awacs.h"

/** AWAC sound device emulation.

    Author: Max Poliakovski 2019
*/

using namespace std;


uint32_t AWACDevice::snd_ctrl_read(uint32_t offset, int size)
{
    switch(offset) {
    case AWAC_CODEC_CTRL_REG:
        return this->is_busy;
    case AWAC_CODEC_STATUS_REG:
        return (AWAC_AVAILABLE << 8) | (AWAC_MAKER_CRYSTAL << 16) |
               (AWAC_REV_SCREAMER << 20);
        break;
    default:
        cout << "AWAC: unsupported register at offset 0x" << hex << offset << endl;
    }

    return 0;
}

void AWACDevice::snd_ctrl_write(uint32_t offset, uint32_t value, int size)
{
    int subframe, reg_num;
    uint16_t data;

    switch(offset) {
    case AWAC_CODEC_CTRL_REG:
        subframe = (value >> 14) & 3;
        reg_num  = (value >> 20) & 7;
        data = ((value >> 8) & 0xF00) | ((value >> 24) & 0xFF);
        printf("AWAC subframe = %d, reg = %d, data = %08X\n", subframe, reg_num, data);
        if (!subframe)
            this->control_regs[reg_num] = data;
        break;
    default:
        cout << "AWAC: unsupported register at offset 0x" << hex << offset << endl;
    }
}
