/*
DingusPPC - The Experimental PowerPC Macintosh emulator
Copyright (C) 2018-23 divingkatae and maximum
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

/** @file Apple Desktop Bus Keyboard definitions. */

#ifndef ADB_KEYBOARD_H
#define ADB_KEYBOARD_H

#include <core/hostevents.h>
#include <devices/common/adb/adbdevice.h>
#include <devices/common/hwcomponent.h>

#include <SDL.h>

#include <cinttypes>
#include <memory>
#include <string>

enum : uint8_t {
    ADB_keycode_a,
    ADB_keycode_s,
    ADB_keycode_d,
    ADB_keycode_f,
    ADB_keycode_h,
    ADB_keycode_g,
    ADB_keycode_z,
    ADB_keycode_x,
    ADB_keycode_c,
    ADB_keycode_v,
    ADB_keycode_int1,
    ADB_keycode_b,
    ADB_keycode_q,
    ADB_keycode_w,
    ADB_keycode_e,
    ADB_keycode_r,
    ADB_keycode_y,
    ADB_keycode_t,
    ADB_keycode_1,
    ADB_keycode_2,
    ADB_keycode_3,
    ADB_keycode_4,
    ADB_keycode_6,
    ADB_keycode_5,
    ADB_keycode_eq,
    ADB_keycode_9,
    ADB_keycode_7,
    ADB_keycode_min,
    ADB_keycode_8,
    ADB_keycode_0,
    ADB_keycode_rb,
    ADB_keycode_o,
    ADB_keycode_u,
    ADB_keycode_lb,
    ADB_keycode_i,
    ADB_keycode_p,
    ADB_keycode_ret,
    ADB_keycode_l,
    ADB_keycode_j,
    ADB_keycode_quote,
    ADB_keycode_k,
    ADB_keycode_semi,
    ADB_keycode_bksl,
    ADB_keycode_comma,
    ADB_keycode_slsh,
    ADB_keycode_n,
    ADB_keycode_m,
    ADB_keycode_per,
    ADB_keycode_tab,
    ADB_keycode_space,
    ADB_keycode_bkqt,
    ADB_keycode_del,
    ADB_keycode_bunk_34,
    ADB_keycode_esc,
    ADB_keycode_ctrl,
    ADB_keycode_appl,
    ADB_keycode_shift,
    ADB_keycode_caps,
    ADB_keycode_opt,
    ADB_keycode_left,
    ADB_keycode_right,
    ADB_keycode_down,
    ADB_keycode_up,
    ADB_keycode_bunk_3f,
    ADB_keycode_bunk_40,
    ADB_keycode_kp_per,
    ADB_keycode_bunk_42,
    ADB_keycode_kp_mul,
    ADB_keycode_bunk_44,
    ADB_keycode_kp_plus,
    ADB_keycode_bunk_46,
    ADB_keycode_kp_clr,
    ADB_keycode_bunk_48,
    ADB_keycode_bunk_49,
    ADB_keycode_bunk_4a,
    ADB_keycode_kp_div,
    ADB_keycode_kp_enter,
    ADB_keycode_bunk_4d,
    ADB_keycode_kp_minus,
    ADB_keycode_bunk_4f,
    ADB_keycode_bunk_50,
    ADB_keycode_bunk_51,
    ADB_keycode_kp_0,
    ADB_keycode_kp_1,
    ADB_keycode_kp_2,
    ADB_keycode_kp_3,
    ADB_keycode_kp_4,
    ADB_keycode_kp_5,
    ADB_keycode_kp_6,
    ADB_keycode_kp_7,
    ADB_keycode_bunk_5a,
    ADB_keycode_kp_8,
    ADB_keycode_kp_9,
};

class AdbKeyboard : public AdbDevice {
public:
    AdbKeyboard(std::string name);
    ~AdbKeyboard() = default;

    static std::unique_ptr<HWComponent> create() {
        return std::unique_ptr<AdbKeyboard>(new AdbKeyboard("ADB-KEYBOARD"));
    }

    uint8_t adb_translate(SDL_Keycode input);

    void reset() override;
    void event_handler(const KeyboardEvent& event);

    bool get_register_0() override;
    bool get_register_2() override;
    void set_register_2() override;
    void set_register_3() override;


private:
    uint32_t flags         = 0;
    uint32_t key_code      = 0;
    uint16_t led_lights    = 0;
    uint8_t key_buffer[16] = {0xFF};
    uint8_t key_state[128] = {0xFF};
    uint8_t adb_code       = 0;
    bool key_pressed       = false;
};

#endif    // ADB_KEYBOARD_H
