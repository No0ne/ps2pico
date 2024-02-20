/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 No0ne (https://github.com/No0ne)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "ps2pico.h"
#include "tusb.h"
#ifdef XTPHY
  #include "xtphy.pio.h"
#endif
#ifdef XTALT
  #include "xtalt.pio.h"
#endif

u8 const mod2xt[] = { 0x1d, 0x2a, 0x38, 0x5b, 0x1d, 0x36, 0x38, 0x5c };
u8 const hid2xt[] = {
  0x00, 0xff, 0xfc, 0x00, 0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26,
  0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x1c, 0x01, 0x0e, 0x0f, 0x39, 0x0c, 0x0d, 0x1a,
  0x1b, 0x2b, 0x2b, 0x27, 0x28, 0x29, 0x33, 0x34, 0x35, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
  0x41, 0x42, 0x43, 0x44, 0x57, 0x58, 0x37, 0x46, 0x46, 0x52, 0x47, 0x49, 0x53, 0x4f, 0x51, 0x4d,
  0x4b, 0x50, 0x48, 0x45, 0x35, 0x37, 0x4a, 0x4e, 0x1c, 0x4f, 0x50, 0x51, 0x4b, 0x4c, 0x4d, 0x47,
  0x48, 0x49, 0x52, 0x53, 0x56, 0x5d, 0x5e, 0x59, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
  0x6c, 0x6d, 0x6e, 0x76
};

u32 const repeat_us = 91743;
u16 const delay_ms = 500;

u8 leds = 0;
u8 repeat = 0;
u8 last_pc;
u8 stuck;

bool blinking = false;
alarm_id_t repeater;

void xt_send(u8 byte) {
  printf("TX: %02x\n", byte);
  pio_sm_put(pio0, 0, (byte ^ 0xff) << 1);
}

void xt_maybe_send_e0(u8 key) {
  if(key == HID_KEY_PRINT_SCREEN ||
     key >= HID_KEY_INSERT && key <= HID_KEY_ARROW_UP ||
     key == HID_KEY_KEYPAD_DIVIDE ||
     key == HID_KEY_KEYPAD_ENTER ||
     key == HID_KEY_APPLICATION ||
     key == HID_KEY_POWER ||
     key >= HID_KEY_GUI_LEFT && key != HID_KEY_SHIFT_RIGHT) {
    xt_send(0xe0);
  }
}

void xt_set_led(u8 led) {
  leds ^= led;
  tuh_kb_set_leds(leds);
}

s64 repeat_callback(alarm_id_t id, void *user_data) {
  if(repeat) {
    xt_maybe_send_e0(repeat);
    
    if(repeat >= HID_KEY_CONTROL_LEFT && repeat <= HID_KEY_GUI_RIGHT) {
      xt_send(mod2xt[repeat - HID_KEY_CONTROL_LEFT]);
    } else {
      xt_send(hid2xt[repeat]);
    }
    
    return repeat_us;
  }
  
  repeater = 0;
  return 0;
}

void kb_send_key(u8 key, bool state, u8 modifiers) {
  if(key > HID_KEY_F24 &&
     key < HID_KEY_CONTROL_LEFT ||
     key > HID_KEY_GUI_RIGHT) return;
  
  printf("HID code = %02x, state = %01x\n", key, state);
  
  if(state) {
    if(key == HID_KEY_NUM_LOCK) xt_set_led(KEYBOARD_LED_NUMLOCK);
    if(key == HID_KEY_CAPS_LOCK) xt_set_led(KEYBOARD_LED_CAPSLOCK);
    if(key == HID_KEY_SCROLL_LOCK) xt_set_led(KEYBOARD_LED_SCROLLLOCK);
  }
  
  if(key == HID_KEY_PAUSE) {
    repeat = 0;
    
    if(state) {
      if(modifiers & KEYBOARD_MODIFIER_LEFTCTRL ||
         modifiers & KEYBOARD_MODIFIER_RIGHTCTRL) {
        xt_send(0xe0); xt_send(0x46);
        xt_send(0xe0); xt_send(0xc6);
      } else {
        xt_send(0xe1); xt_send(0x1d); xt_send(0x45);
        xt_send(0xe1); xt_send(0x9d); xt_send(0xc5);
      }
    }
    
    return;
  }
  
  if(state) {
    repeat = key;
    if(repeater) cancel_alarm(repeater);
    repeater = add_alarm_in_ms(delay_ms, repeat_callback, NULL, false);
  } else {
    if(key == repeat) repeat = 0;
  }
  
  xt_maybe_send_e0(key);
  
  if(key >= HID_KEY_CONTROL_LEFT && key <= HID_KEY_GUI_RIGHT) {
    key = mod2xt[key - HID_KEY_CONTROL_LEFT];
  } else {
    key = hid2xt[key];
  }
  
  if(state) {
    xt_send(key);
  } else {
    xt_send(key | 0x80);
  }
}

s64 blink_callback(alarm_id_t id, void *user_data) {
  if(blinking) {
    tuh_kb_set_leds(KEYBOARD_LED_NUMLOCK | KEYBOARD_LED_CAPSLOCK | KEYBOARD_LED_SCROLLLOCK);
    blinking = false;
    return 500000;
  }
  
  tuh_kb_set_leds(leds);
  xt_send(0xaa);
  return 0;
}

void kb_reset() {
  leds = 0;
  repeat = 0;
  blinking = true;
  pio_sm_drain_tx_fifo(pio0, 0);
  add_alarm_in_ms(50, blink_callback, NULL, false);
}

s64 reset_detect() {
  u8 pc = pio_sm_get_pc(pio0, 0);
  stuck = last_pc == pc ? stuck + 1 : 0;
  last_pc = pc;
  
  if(stuck == 5) {
    printf("reset detected!\n");
    stuck = 0;
    kb_reset();
    return 1000000;
  }
  
  return 5000;
}

void kb_init() {
  #ifdef XTPHY
    xtphy_program_init(pio0, 0, pio_add_program(pio0, &xtphy_program));
  #endif
  #ifdef XTALT
    xtalt_program_init(pio0, 0, pio_add_program(pio0, &xtalt_program));
  #endif
  add_alarm_in_ms(1000, reset_detect, NULL, false);
}
