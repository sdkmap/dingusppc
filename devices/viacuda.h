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

/** VIA-CUDA combo device emulation.

    Author: Max Poliakovski 2019

    Versatile interface adapter (VIA) is an old I/O controller that can be found
    in nearly every Macintosh computer. In the 68k era, VIA was used to control
    various peripherial devices. In a Power Macintosh, its function is limited
    to the I/O interface for the Cuda MCU. I therefore decided to put VIA
    emulation code here.

    Cuda MCU is a multipurpose IC built around a custom version of the Motorola
    MC68HC05 microcontroller. It provides several functions including:
    - Apple Desktop Bus (ADB) master
    - I2C bus master
    - Realtime clock (RTC)
    - parameter RAM (first generation of the Power Macintosh)
    - power management

    MC68HC05 doesn't provide any dedicated hardware for serial communication
    protocols. All signals required for ADB and I2C will be generated by Cuda
    firmware using bit banging (https://en.wikipedia.org/wiki/Bit_banging).
*/

#ifndef VIACUDA_H
#define VIACUDA_H

#include "hwcomponent.h"
#include "nvram.h"
#include "i2c.h"

/** VIA register offsets. */
enum {
    VIA_B    = 0x00, /* input/output register B */
    VIA_A    = 0x01, /* input/output register A */
    VIA_DIRB = 0x02, /* direction B */
    VIA_DIRA = 0x03, /* direction A */
    VIA_T1CL = 0x04, /* low-order  timer 1 counter */
    VIA_T1CH = 0x05, /* high-order timer 1 counter */
    VIA_T1LL = 0x06, /* low-order  timer 1 latches */
    VIA_T1LH = 0x07, /* high-order timer 1 latches */
    VIA_T2CL = 0x08, /* low-order  timer 2 latches */
    VIA_T2CH = 0x09, /* high-order timer 2 counter */
    VIA_SR   = 0x0A, /* shift register */
    VIA_ACR  = 0x0B, /* auxiliary control register */
    VIA_PCR  = 0x0C, /* periheral control register */
    VIA_IFR  = 0x0D, /* interrupt flag register */
    VIA_IER  = 0x0E, /* interrupt enable register */
    VIA_ANH  = 0x0F, /* input/output register A, no handshake */
};

/** Cuda communication signals. */
enum {
    CUDA_TIP     = 0x20, /* transaction in progress: 0 - true, 1 - false */
    CUDA_BYTEACK = 0x10, /* byte acknowledge: 0 - true, 1 - false */
    CUDA_TREQ    = 0x08  /* Cuda requests transaction from host */
};

/** Cuda packet types. */
enum {
    CUDA_PKT_ADB    = 0,
    CUDA_PKT_PSEUDO = 1,
    CUDA_PKT_ERROR  = 2,
    CUDA_PKT_TICK   = 3,
    CUDA_PKT_POWER  = 4
};

/** Cuda pseudo commands. */
enum {
    CUDA_READ_PRAM      = 0x07, /* read parameter RAM*/
    CUDA_WRITE_PRAM     = 0x0C, /* write parameter RAM*/
    CUDA_READ_WRITE_I2C = 0x22, /* read/write I2C device */
    CUDA_COMB_FMT_I2C   = 0x25, /* combined format I2C transaction */
    CUDA_OUT_PB0        = 0x26, /* output one bit to Cuda's PB0 line */
};

/** Cuda error codes. */
enum {
    CUDA_ERR_BAD_PKT  = 1, /* invalid packet type */
    CUDA_ERR_BAD_CMD  = 2, /* invalid pseudo command */
    CUDA_ERR_BAD_SIZE = 3, /* invalid packet size */
    CUDA_ERR_I2C      = 5  /* invalid I2C data or no acknowledge */
};


class ViaCuda : public HWComponent, public I2CBus
{
public:
    ViaCuda();
    ~ViaCuda();

    bool supports_type(HWCompType type) {
        return (type == HWCompType::ADB_HOST || type == HWCompType::I2C_HOST);
    };

    uint8_t read(int reg);
    void write(int reg, uint8_t value);

private:
    uint8_t via_regs[16]; /* VIA virtual registers */

    /* Cuda state. */
    uint8_t old_tip;
    uint8_t old_byteack;
    uint8_t treq;
    uint8_t in_buf[16];
    int32_t in_count;
    uint8_t out_buf[16];
    int32_t out_count;
    int32_t out_pos;

    bool    is_open_ended;
    uint8_t curr_i2c_addr;

    void (ViaCuda::*out_handler)(void);
    void (ViaCuda::*next_out_handler)(void);

    NVram* pram_obj;

    void print_enabled_ints(); /* print enabled VIA interrupts and their sources */

    void init();
    bool ready();
    void assert_sr_int();
    void write(uint8_t new_state);
    void response_header(uint32_t pkt_type, uint32_t pkt_flag);
    //void cuda_response_packet();
    void error_response(uint32_t error);
    void process_packet();
    void process_adb_command(uint8_t cmd_byte, int data_count);
    void pseudo_command(int cmd, int data_count);

    void null_out_handler(void);
    void out_buf_handler(void);
    void i2c_handler(void);

    /* I2C related methods */
    void i2c_simple_transaction(uint8_t dev_addr, const uint8_t* in_buf, int in_bytes);
    void i2c_comb_transaction(uint8_t dev_addr, uint8_t sub_addr, uint8_t dev_addr1,
        const uint8_t* in_buf, int in_bytes);
};

#endif /* VIACUDA_H */
