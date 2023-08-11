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

/** @file Apple Desktop Bus Keyboard emulation. */

#include <devices/common/adb/adbkeyboard.h>
#include <devices/deviceregistry.h>

AdbKeyboard::AdbKeyboard(std::string name) : AdbDevice(name) {
    EventManager::get_instance()->add_keyboard_handler(this, &AdbKeyboard::event_handler);

    this->reset();
}

void AdbKeyboard::event_handler(const KeyboardEvent& event) {
    if (event.flags & KEYBOARD_EVENT_DOWN) {
        adb_code          = adb_translate(event.key_code);
        this->key_pressed = true;
        for (int input_spot = 0; input_spot < 16; input_spot++) {
            if (this->key_state[input_spot] == 0xFF) {
                this->key_state[input_spot] = 0x0 | adb_code;
                }
            }
        } 
    else if (event.flags & KEYBOARD_EVENT_UP) {
        this->key_pressed = false;
        for (int input_spot = 0; input_spot < 16; input_spot++) {
            this->key_state[input_spot] = 0x80 | adb_code;
        }
    }
}

void AdbKeyboard::reset() {
    this->my_addr         = ADB_ADDR_KBD;
    this->dev_handler_id  = 2;    // Standard ADB keyboard
    this->exc_event_flag  = 1;
    this->srq_flag        = 1;    // enable service requests
    this->flags           = 0;
    this->led_lights      = 0;
    for (int key_array_len = 0; key_array_len < 16; key_array_len++) {
        this->key_state[key_array_len] = 0xFF;
    }
    this->key_pressed = false;
}

bool AdbKeyboard::get_register_0() {
    if (this->key_pressed) {
        uint8_t* out_buf = this->host_obj->get_output_buf();

        for (int input_spot = 0; input_spot < 16; input_spot += 2) {
            out_buf[0]     = key_state[input_spot];
            out_buf[1]     = key_state[input_spot + 1];

            this->host_obj->set_output_count(2);
        }
        return true;
    }
    return false;
}

bool AdbKeyboard::get_register_2() {
    if (this->key_pressed) {
        uint8_t* out_buf = this->host_obj->get_output_buf();

        out_buf[0] = led_lights >> 8;
        out_buf[1] = led_lights & 0xFF;

        this->host_obj->set_output_count(2);
        this->key_pressed = false;
        return true;
    }
    return false;
}

void AdbKeyboard::set_register_2() {
    for (int input_spot = 0; input_spot < 16; input_spot++) {
        switch (key_state[input_spot]) {
        case ADB_keycode_del:
            led_lights |= 1 << 14;
        case ADB_keycode_caps:
            led_lights |= 1 << 13;
        case ADB_keycode_ctrl:
            led_lights |= 1 << 11;
        case ADB_keycode_shift:
            led_lights |= 1 << 10;
        case ADB_keycode_opt:
            led_lights |= 1 << 9;
        case ADB_keycode_appl:
            led_lights |= 1 << 8;
        default:
            led_lights = 0;
            }
    } 
}

void AdbKeyboard::set_register_3() {
    if (this->host_obj->get_input_count() < 2)    // ensure we got enough data
        return;

    const uint8_t* in_data = this->host_obj->get_input_buf();

    switch (in_data[1]) {
    case 0:
        this->my_addr  = in_data[0] & 0xF;
        this->srq_flag = !!(in_data[0] & 0x20);
        break;
    case 1:
    case 2:
        this->dev_handler_id = in_data[1];
        break;
    case 3:    // extended keyboard protocol isn't supported yet
        break;
    case 0xFE:    // move to a new address if there was no collision
        if (!this->got_collision) {
            this->my_addr = in_data[0] & 0xF;
        }
        break;
    default:
        LOG_F(WARNING, "%s: unknown handler ID = 0x%X", this->name.c_str(), in_data[1]);
    }
}

uint8_t AdbKeyboard::adb_translate(SDL_Keycode input) {
    // Hoping there's a better way to do this,
    // but this will have to do for now.
    switch (input) {
    case SDLK_0:
        return ADB_keycode_0;
    case SDLK_1:
        return ADB_keycode_1;
    case SDLK_2:
        return ADB_keycode_2;
    case SDLK_3:
        return ADB_keycode_3;
    case SDLK_4:
        return ADB_keycode_4;
    case SDLK_5:
        return ADB_keycode_5;
    case SDLK_6:
        return ADB_keycode_6;
    case SDLK_7:
        return ADB_keycode_7;
    case SDLK_8:
        return ADB_keycode_8;
    case SDLK_9:
        return ADB_keycode_9;
    case SDLK_q:
        return ADB_keycode_q;
    case SDLK_w:
        return ADB_keycode_w;
    case SDLK_e:
        return ADB_keycode_e;
    case SDLK_r:
        return ADB_keycode_r;
    case SDLK_t:
        return ADB_keycode_t;
    case SDLK_y:
        return ADB_keycode_y;
    case SDLK_i:
        return ADB_keycode_i;
    case SDLK_o:
        return ADB_keycode_o;
    case SDLK_u:
        return ADB_keycode_u;
    case SDLK_p:
        return ADB_keycode_p;
    case SDLK_a:
        return ADB_keycode_a;
    case SDLK_s:
        return ADB_keycode_s;
    case SDLK_d:
        return ADB_keycode_d;
    case SDLK_f:
        return ADB_keycode_f;
    case SDLK_g:
        return ADB_keycode_g;
    case SDLK_h:
        return ADB_keycode_h;
    case SDLK_j:
        return ADB_keycode_j;
    case SDLK_k:
        return ADB_keycode_k;
    case SDLK_l:
        return ADB_keycode_l;
    case SDLK_z:
        return ADB_keycode_z;
    case SDLK_x:
        return ADB_keycode_x;
    case SDLK_c:
        return ADB_keycode_c;
    case SDLK_v:
        return ADB_keycode_v;
    case SDLK_b:
        return ADB_keycode_b;
    case SDLK_n:
        return ADB_keycode_n;
    case SDLK_m:
        return ADB_keycode_m;
    case SDLK_SPACE:
        return ADB_keycode_space;
    case SDLK_EQUALS:
        return ADB_keycode_eq;
    case SDLK_ESCAPE:
        return ADB_keycode_esc;
    case SDLK_RETURN:
        return ADB_keycode_ret;
    case SDLK_QUOTE:
        return ADB_keycode_quote;
    case SDLK_COMMA:
        return ADB_keycode_comma;
    case SDLK_PERIOD:
        return ADB_keycode_per;
    case SDLK_SLASH:
        return ADB_keycode_slsh;
    case SDLK_BACKSLASH:
        return ADB_keycode_bksl;
    case SDLK_LEFTBRACKET:
        return ADB_keycode_lb;
    case SDLK_RIGHTBRACKET:
        return ADB_keycode_rb;
    case SDLK_LEFT:
        return ADB_keycode_left;
    case SDLK_RIGHT:
        return ADB_keycode_right;
    case SDLK_UP:
        return ADB_keycode_up;
    case SDLK_DOWN:
        return ADB_keycode_down;
    //case SDLK_BACKQUOTE:  -for ref; Apple Extended KB not yet implemented
    //    return ADB_keycode_bkqt;
    case SDLK_TAB:
        return ADB_keycode_tab;
    case SDLK_BACKSPACE:
        return ADB_keycode_del;
    case SDLK_LSHIFT:
        return ADB_keycode_shift;
    case SDLK_LCTRL:
        return ADB_keycode_ctrl;
    case SDLK_LALT:
        return ADB_keycode_opt;
    case SDLK_BACKQUOTE:
    case SDLK_APPLICATION: //temp map
    case SDLK_MENU:
        return ADB_keycode_opt;
    case SDLK_CAPSLOCK:
        return ADB_keycode_caps;
    case SDLK_NUMLOCKCLEAR:
        return ADB_keycode_kp_clr;
    case SDLK_KP_DIVIDE:
        return ADB_keycode_kp_div;
    case SDLK_KP_MULTIPLY:
        return ADB_keycode_kp_mul;
    case SDLK_KP_MINUS:
        return ADB_keycode_kp_minus;
    case SDLK_KP_PLUS:
        return ADB_keycode_kp_plus;
    case SDLK_KP_ENTER:
        return ADB_keycode_kp_enter;
    case SDLK_KP_PERIOD:
        return ADB_keycode_kp_per;
    case SDLK_KP_0:
        return ADB_keycode_kp_0;
    case SDLK_KP_1:
        return ADB_keycode_kp_1;
    case SDLK_KP_2:
        return ADB_keycode_kp_2;
    case SDLK_KP_3:
        return ADB_keycode_kp_3;
    case SDLK_KP_4:
        return ADB_keycode_kp_4;
    case SDLK_KP_5:
        return ADB_keycode_kp_5;
    case SDLK_KP_6:
        return ADB_keycode_kp_6;
    case SDLK_KP_7:
        return ADB_keycode_kp_7;
    case SDLK_KP_8:
        return ADB_keycode_kp_8;
    case SDLK_KP_9:
        return ADB_keycode_kp_9;
    }
}

static const DeviceDescription AdbKeyboard_Descriptor = {AdbKeyboard::create, {}, {}};

REGISTER_DEVICE(AdbKeyboard, AdbKeyboard_Descriptor);
