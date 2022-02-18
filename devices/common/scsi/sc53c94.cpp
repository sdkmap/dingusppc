/*
DingusPPC - The Experimental PowerPC Macintosh emulator
Copyright (C) 2018-22 divingkatae and maximum
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

/** @file NCR53C94/Am53CF94 SCSI controller emulation. */

#include <core/timermanager.h>
#include <devices/common/hwcomponent.h>
#include <devices/common/hwinterrupt.h>
#include <devices/common/scsi/sc53c94.h>
#include <loguru.hpp>
#include <machines/machinebase.h>

#include <cinttypes>

Sc53C94::Sc53C94(uint8_t chip_id, uint8_t my_id)
{
    this->chip_id   = chip_id;
    this->my_bus_id = my_id;
    supports_types(HWCompType::SCSI_HOST | HWCompType::SCSI_DEV);
    reset_device();
}

int Sc53C94::device_postinit()
{
    this->bus_obj = dynamic_cast<ScsiBus*>(gMachineObj->get_comp_by_name("SCSI0"));
    this->bus_obj->register_device(7, static_cast<ScsiDevice*>(this));

    this->int_ctrl = dynamic_cast<InterruptCtrl*>(
        gMachineObj->get_comp_by_type(HWCompType::INT_CTRL));
    this->irq_id = this->int_ctrl->register_dev_int(IntSrc::SCSI1);

    return 0;
}

void Sc53C94::reset_device()
{
    // part-unique ID to be read using a magic sequence
    this->set_xfer_count = this->chip_id << 16;

    this->clk_factor   = 2;
    this->sel_timeout  = 0;
    this->is_initiator = true;

    // clear command FIFO
    this->cmd_fifo_pos = 0;

    // clear data FIFO
    this->data_fifo_pos = 0;
    this->data_fifo[0]  = 0;

    this->seq_step = 0;

    this->status = 0;
}

uint8_t Sc53C94::read(uint8_t reg_offset)
{
    uint8_t int_status;

    switch (reg_offset) {
    case Read::Reg53C94::Command:
        return this->cmd_fifo[0];
    case Read::Reg53C94::Status:
        return this->status;
    case Read::Reg53C94::Int_Status:
        int_status = this->int_status;
        this->seq_step = 0;
        this->int_status = 0;
        this->update_irq();
        return int_status;
    case Read::Reg53C94::Seq_Step:
        return this->seq_step;
    case Read::Reg53C94::FIFO_Flags:
        return (this->seq_step << 5) | (this->data_fifo_pos & 0x1F);
    case Read::Reg53C94::Config_3:
        return this->config3;
    case Read::Reg53C94::Xfer_Cnt_Hi:
        if (this->config2 & CFG2_ENF) {
            return (this->xfer_count >> 16) & 0xFFU;
        }
        break;
    default:
        LOG_F(INFO, "SC53C94: reading from register %d", reg_offset);
    }
    return 0;
}

void Sc53C94::write(uint8_t reg_offset, uint8_t value)
{
    switch (reg_offset) {
    case Write::Reg53C94::Command:
        update_command_reg(value);
        break;
    case Write::Reg53C94::FIFO:
        fifo_push(value);
        break;
    case Write::Reg53C94::Dest_Bus_ID:
        this->target_id = value & 7;
        break;
    case Write::Reg53C94::Sel_Timeout:
        this->sel_timeout = value;
        break;
    case Write::Reg53C94::Sync_Offset:
        this->sync_offset = value;
        break;
    case Write::Reg53C94::Clock_Factor:
        this->clk_factor = value;
        break;
    case Write::Reg53C94::Config_1:
        if ((value & 7) != this->my_bus_id) {
            ABORT_F("SC53C94: HBA bus ID mismatch!");
        }
        this->config1 = value;
        break;
    case Write::Reg53C94::Config_2:
        this->config2 = value;
        break;
    case Write::Reg53C94::Config_3:
        this->config3 = value;
        break;
    default:
        LOG_F(INFO, "SC53C94: writing 0x%X to %d register", value, reg_offset);
    }
}

void Sc53C94::update_command_reg(uint8_t cmd)
{
    if (this->on_reset && (cmd & 0x7F) != CMD_NOP) {
        LOG_F(WARNING, "SC53C94: command register blocked after RESET!");
        return;
    }

    // NOTE: Reset Device (chip), Reset Bus and DMA Stop commands execute
    // immediately while all others are placed into the command FIFO
    switch (cmd & 0x7F) {
    case CMD_RESET_DEVICE:
    case CMD_RESET_BUS:
    case CMD_DMA_STOP:
        this->cmd_fifo_pos = 0; // put them at the bottom of the command FIFO
    }

    if (this->cmd_fifo_pos < 2) {
        // put new command into the command FIFO
        this->cmd_fifo[this->cmd_fifo_pos++] = cmd;
        if (this->cmd_fifo_pos == 1) {
            exec_command();
        }
    } else {
        LOG_F(ERROR, "SC53C94: the top of the command FIFO overwritten!");
        this->status |= 0x40; // signal IOE/Gross Error
    }
}

void Sc53C94::exec_command()
{
    uint8_t cmd = this->cmd_fifo[0] & 0x7F;
    bool    is_dma_cmd = !!(this->cmd_fifo[0] & 0x80);

    if (is_dma_cmd) {
        if (this->config2 & CFG2_ENF) { // extended mode: 24-bit
            this->xfer_count = this->set_xfer_count & 0xFFFFFFUL;
        } else { // standard mode: 16-bit
            this->xfer_count = this->set_xfer_count & 0xFFFFUL;
        }
    }

    // simple commands will be executed immediately
    // complex commands will be broken into multiple steps
    // and handled by the sequencer
    switch (cmd) {
    case CMD_NOP:
        this->on_reset = false; // unblock the command register
        exec_next_command();
        break;
    case CMD_CLEAR_FIFO:
        this->data_fifo_pos = 0; // set the bottom of the FIFO to zero
        this->data_fifo[0] = 0;
        exec_next_command();
        break;
    case CMD_RESET_DEVICE:
        reset_device();
        this->on_reset = true; // block the command register
        return;
    case CMD_RESET_BUS:
        LOG_F(INFO, "SC53C94: resetting SCSI bus...");
        // assert RST line
        this->bus_obj->assert_ctrl_line(this->my_bus_id, SCSI_CTRL_RST);
        // release RST line after 25 us
        my_timer_id = TimerManager::get_instance()->add_oneshot_timer(
            USECS_TO_NSECS(25),
            [this]() {
                this->bus_obj->release_ctrl_line(this->my_bus_id, SCSI_CTRL_RST);
        });
        if (!(config1 & 0x40)) {
            LOG_F(INFO, "SC53C94: reset interrupt issued");
            this->int_status |= INTSTAT_SRST;
        }
        exec_next_command();
        break;
    case CMD_SELECT_NO_ATN:
        static SeqDesc sel_no_atn_desc[] {
            {SeqState::SEL_BEGIN,    0, INTSTAT_DIS            },
            {SeqState::CMD_BEGIN,    3, INTSTAT_SR | INTSTAT_SO},
            {SeqState::CMD_COMPLETE, 4, INTSTAT_SR | INTSTAT_SO},
        };
        this->seq_step = 0;
        this->cmd_steps = sel_no_atn_desc;
        this->cur_state = SeqState::BUS_FREE;
        sequencer();
        LOG_F(INFO, "SC53C94: SELECT W/O ATN command started");
        break;
    case CMD_ENA_SEL_RESEL:
        LOG_F(INFO, "SC53C94: ENABLE SELECTION/RESELECTION command executed");
        exec_next_command();
        break;
    default:
        LOG_F(ERROR, "SC53C94: invalid/unimplemented command 0x%X", cmd);
        this->cmd_fifo_pos--; // remove invalid command from FIFO
        this->int_status |= INTSTAT_ICMD;
    }
}

void Sc53C94::exec_next_command()
{
    if (this->cmd_fifo_pos) { // skip empty command FIFO
        this->cmd_fifo_pos--; // remove completed command
        if (this->cmd_fifo_pos) { // is there another command in the FIFO?
            this->cmd_fifo[0] = this->cmd_fifo[1]; // top -> bottom
            exec_command(); // execute it
        }
    }
}

void Sc53C94::fifo_push(const uint8_t data)
{
    if (this->data_fifo_pos < 16) {
        this->data_fifo[this->data_fifo_pos++] = data;
    } else {
        LOG_F(ERROR, "SC53C94: data FIFO overflow!");
        this->status |= 0x40; // signal IOE/Gross Error
    }
}

void Sc53C94::seq_defer_state(uint64_t delay_ns)
{
    seq_timer_id = TimerManager::get_instance()->add_oneshot_timer(
        delay_ns,
        [this]() {
            // re-enter the sequencer with the state specified in next_state
            this->cur_state = this->next_state;
            this->sequencer();
    });
}

void Sc53C94::sequencer()
{
    switch (this->cur_state) {
    case SeqState::IDLE:
        break;
    case SeqState::BUS_FREE:
        if (this->bus_obj->current_phase() == ScsiPhase::BUS_FREE) {
            this->next_state = SeqState::ARB_BEGIN;
            this->seq_defer_state(BUS_FREE_DELAY + BUS_SETTLE_DELAY);
        } else { // continue waiting
            this->next_state = SeqState::BUS_FREE;
            this->seq_defer_state(BUS_FREE_DELAY);
        }
        break;
    case SeqState::ARB_BEGIN:
        if (!this->bus_obj->begin_arbitration(this->my_bus_id)) {
            LOG_F(ERROR, "SC53C94: arbitration error, bus not free!");
            this->bus_obj->release_ctrl_lines(this->my_bus_id);
            this->next_state = SeqState::BUS_FREE;
            this->seq_defer_state(BUS_CLEAR_DELAY);
            break;
        }
        this->next_state = SeqState::ARB_END;
        this->seq_defer_state(ARB_DELAY);
        break;
    case SeqState::ARB_END:
        if (this->bus_obj->end_arbitration(this->my_bus_id)) { // arbitration won
            this->next_state = this->cmd_steps->next_step;
            this->seq_defer_state(BUS_CLEAR_DELAY + BUS_SETTLE_DELAY);
        } else { // arbitration lost
            LOG_F(INFO, "SC53C94: arbitration lost!");
            this->bus_obj->release_ctrl_lines(this->my_bus_id);
            this->next_state = SeqState::BUS_FREE;
            this->seq_defer_state(BUS_CLEAR_DELAY);
        }
        break;
    case SeqState::SEL_BEGIN:
        this->is_initiator = true;
        this->bus_obj->begin_selection(this->my_bus_id, this->target_id,
            this->cur_cmd != CMD_SELECT_NO_ATN);
        this->next_state = SeqState::SEL_END;
        this->seq_defer_state(SEL_TIME_OUT);
        break;
    case SeqState::SEL_END:
        if (this->bus_obj->end_selection(this->my_bus_id, this->target_id)) {
            LOG_F(INFO, "SC53C94: selection completed");
            this->cmd_steps++;
        } else { // selection timeout
            this->seq_step = this->cmd_steps->step_num;
            this->int_status |= this->cmd_steps->status;
            this->bus_obj->disconnect(this->my_bus_id);
            this->cur_state = SeqState::IDLE;
            this->update_irq();
            exec_next_command();
        }
        break;
    default:
        ABORT_F("SC53C94: unimplemented sequencer state %d", this->cur_state);
    }
}

void Sc53C94::update_irq()
{
    uint8_t new_irq = !!(this->int_status != 0);
    if (new_irq != this->irq) {
        this->irq = new_irq;
        this->status = (this->status & 0x7F) | (new_irq << 7);
        this->int_ctrl->ack_int(this->irq_id, new_irq);
    }
}

void Sc53C94::notify(ScsiMsg msg_type, int param)
{
    switch (msg_type) {
    case ScsiMsg::CONFIRM_SEL:
        if (this->target_id == param) {
            // cancel selection timeout timer
            TimerManager::get_instance()->cancel_timer(this->seq_timer_id);
            this->cur_state = SeqState::SEL_END;
            sequencer();
        } else {
            LOG_F(WARNING, "SC53C94: ignore invalid selection confirmation message");
        }
        break;
    default:
        LOG_F(WARNING, "SC53C94: ignore notification message, type: %d", msg_type);
    }
}