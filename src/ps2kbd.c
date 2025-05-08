/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 No0ne (https://github.com/No0ne)
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

#include "tusb.h"
#include "ps2pico.h"
#include "atphy.pio.h"
#include "bsp/board_api.h"
#include "pico/util/queue.h"

bool kb_enabled = true;
bool locked = false;

u8 leds;
u8 modifiers;
u8 sent = 0;
u8 packet[9];
u8 last_rx = 0;
u8 last_tx = 0;
u8 repeat = 0;
u32 repeat_us = 91743;
u16 delay_ms = 500;

queue_t packets;
alarm_id_t repeater;
extern s8 set_led;

u8 const led2ps2[] = { 0, 4, 1, 5, 2, 6, 3, 7 };
u8 const mod2ps2[] = { 0x14, 0x12, 0x11, 0x1f, 0x14, 0x59, 0x11, 0x27 };
u8 const hid2ps2[] = {
  0x00, 0x00, 0xfc, 0x00, 0x1c, 0x32, 0x21, 0x23, 0x24, 0x2b, 0x34, 0x33, 0x43, 0x3b, 0x42, 0x4b,
  0x3a, 0x31, 0x44, 0x4d, 0x15, 0x2d, 0x1b, 0x2c, 0x3c, 0x2a, 0x1d, 0x22, 0x35, 0x1a, 0x16, 0x1e,
  0x26, 0x25, 0x2e, 0x36, 0x3d, 0x3e, 0x46, 0x45, 0x5a, 0x76, 0x66, 0x0d, 0x29, 0x4e, 0x55, 0x54,
  0x5b, 0x5d, 0x5d, 0x4c, 0x52, 0x0e, 0x41, 0x49, 0x4a, 0x58, 0x05, 0x06, 0x04, 0x0c, 0x03, 0x0b,
  0x83, 0x0a, 0x01, 0x09, 0x78, 0x07, 0x7c, 0x7e, 0x7e, 0x70, 0x6c, 0x7d, 0x71, 0x69, 0x7a, 0x74,
  0x6b, 0x72, 0x75, 0x77, 0x4a, 0x7c, 0x7b, 0x79, 0x5a, 0x69, 0x72, 0x7a, 0x6b, 0x73, 0x74, 0x6c,
  0x75, 0x7d, 0x70, 0x71, 0x61, 0x2f, 0x37, 0x0f, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40,
  0x48, 0x50, 0x57, 0x5f
};
u32 const repeats[] = {
  33333, 37453, 41667, 45872, 48309, 54054, 58480, 62500,
  66667, 75188, 83333, 91743, 100000, 108696, 116279, 125000,
  133333, 149254, 166667, 181818, 200000, 217391, 232558, 250000,
  270270, 303030, 333333, 370370, 400000, 434783, 476190, 500000
};
u16 const delays[] = { 250, 500, 750, 1000 };

u32 ps2_frame(u8 byte) {
  bool parity = 1;
  for(u8 i = 0; i < 8; i++) {
    parity = parity ^ (byte >> i & 1);
  }
  return ((1 << 10) | (parity << 9) | (byte << 1)) ^ 0x7ff;
}

void ps2_send(u8 len) {
  packet[0] = len;

  /*printf("TX: ");
  for(u8 i = 1; i <= packet[0]; i++) {
    printf("%02x ", packet[i]);
  }
  printf("\n");*/

  board_led_write(1);
  queue_try_add(&packets, &packet);
}

void kb_set_leds(u8 byte) {
  if(byte > 7) byte = 0;
  set_led = led2ps2[byte];
}

s64 blink_callback() {
  kb_set_leds(0);
  packet[1] = 0xaa;
  ps2_send(1);
  return 0;
}

bool key_is_e0(u8 key) {
  return key == HID_KEY_PRINT_SCREEN ||
        (key >= HID_KEY_INSERT && key <= HID_KEY_ARROW_UP) ||
         key == HID_KEY_KEYPAD_DIVIDE ||
         key == HID_KEY_KEYPAD_ENTER ||
         key == HID_KEY_APPLICATION ||
         key == HID_KEY_POWER ||
        (key >= HID_KEY_GUI_LEFT && key != HID_KEY_SHIFT_RIGHT);
}

s64 repeat_callback() {
  if(repeat) {
    u8 len = 0;
    if(key_is_e0(repeat)) packet[++len] = 0xe0;

    if(repeat >= HID_KEY_CONTROL_LEFT && repeat <= HID_KEY_GUI_RIGHT) {
      packet[++len] = mod2ps2[repeat - HID_KEY_CONTROL_LEFT];
    } else {
      packet[++len] = hid2ps2[repeat];
    }

    ps2_send(len);
    return repeat_us;
  }

  repeater = 0;
  return 0;
}

void kb_receive(u8 byte, u8 prev_byte) {
  switch(prev_byte) {
    case 0xed: // Set LEDs
      kb_set_leds(byte);
    break;

    case 0xf3: // Set typematic rate and delay
      repeat_us = repeats[byte & 0x1f];
      delay_ms = delays[(byte & 0x60) >> 5];
    break;

    default:
      switch(byte) {
        case 0xff: // Reset
          kb_enabled = true;
          repeat_us = 91743;
          delay_ms = 500;
          repeat = 0;
          kb_set_leds(KEYBOARD_LED_NUMLOCK | KEYBOARD_LED_CAPSLOCK | KEYBOARD_LED_SCROLLLOCK);
          add_alarm_in_ms(500, blink_callback, NULL, false);
        break;

        case 0xee: // Echo
          packet[1] = 0xee;
          ps2_send(1);
        return;

        case 0xf2: // Identify keyboard
          packet[1] = 0xfa;
          packet[2] = 0xab;
          packet[3] = 0x83;
          ps2_send(3);
        return;

        case 0xf4: // Enable scanning
          kb_enabled = true;
        break;

        case 0xf5: // Disable scanning, restore default parameters
        case 0xf6: // Set default parameters
          kb_enabled = byte == 0xf6;
          repeat_us = 91743;
          delay_ms = 500;
          repeat = 0;
          kb_set_leds(0);
        break;
      }
    break;
  }

  packet[1] = 0xfa;
  ps2_send(1);
}

void kb_send_key(u8 key, bool state) {
  if(key >= HID_KEY_CONTROL_LEFT && key <= HID_KEY_GUI_RIGHT) {
    if(state) {
      modifiers = modifiers | (1 << (key - HID_KEY_CONTROL_LEFT));
    } else {
      modifiers = modifiers & ~(1 << (key - HID_KEY_CONTROL_LEFT));
    }
  } else if(key < HID_KEY_A || key > HID_KEY_F24) {
    return;
  }

  u8 len = 0;
  //printf("HID code = %02x, state = %01x\n", key, state);

  if(!kb_enabled) {
    printf("kb_enabled = false\n");
    return;
  }

  if(key == HID_KEY_PAUSE) {
    repeat = 0;

    if(state) {
      if(modifiers & KEYBOARD_MODIFIER_LEFTCTRL ||
         modifiers & KEYBOARD_MODIFIER_RIGHTCTRL) {
        packet[++len] = 0xe0;
        packet[++len] = 0x7e;
        packet[++len] = 0xe0;
        packet[++len] = 0xf0;
        packet[++len] = 0x7e;
      } else {
        packet[++len] = 0xe1;
        packet[++len] = 0x14;
        packet[++len] = 0x77;
        packet[++len] = 0xe1;
        packet[++len] = 0xf0;
        packet[++len] = 0x14;
        packet[++len] = 0xf0;
        packet[++len] = 0x77;
      }

      ps2_send(len);
    }

    return;
  }

  if(key_is_e0(key)) packet[++len] = 0xe0;

  if(state) {
    repeat = key;
    if(repeater) cancel_alarm(repeater);
    repeater = add_alarm_in_ms(delay_ms, repeat_callback, NULL, false);
  } else {
    if(key == repeat) repeat = 0;
    packet[++len] = 0xf0;
  }

  if(key >= HID_KEY_CONTROL_LEFT && key <= HID_KEY_GUI_RIGHT) {
    packet[++len] = mod2ps2[key - HID_KEY_CONTROL_LEFT];
  } else {
    packet[++len] = hid2ps2[key];
  }

  ps2_send(len);
}

void kb_task() {
  if(pio_interrupt_get(pio0, 1)) {
    if(sent > 0) sent--;
    pio_interrupt_clear(pio0, 1);
  }

  if(!locked && !queue_is_empty(&packets) && !pio_interrupt_get(pio0, 0)) {
    if(queue_try_peek(&packets, &packet)) {
      if(sent == packet[0]) {
        sent = 0;
        queue_try_remove(&packets, &packet);
        board_led_write(0);
      } else {
        sent++;
        last_tx = packet[sent];
        locked = true;
        //printf(" put %02x \n", last_tx);
        pio_sm_put(pio0, 0, ps2_frame(last_tx));
      }
    }
  }

  if(locked && pio_interrupt_get(pio0, 0)) locked = false;

  if(!pio_sm_is_rx_fifo_empty(pio0, 1)) {
    u32 fifo = pio_sm_get(pio0, 1) >> 23;

    bool parity = 1;
    for(u8 i = 0; i < 8; i++) {
      parity = parity ^ (fifo >> i & 1);
    }

    if(parity != fifo >> 8) {
      pio_sm_put(pio0, 0, ps2_frame(0xfe));
      return;
    }

    if((fifo & 0xff) == 0xfe) {
      pio_sm_put(pio0, 0, ps2_frame(last_tx));
      return;
    }

    while(queue_try_remove(&packets, &packet));
    sent = 0;

    //printf("RX: 0x%02lx\n", fifo & 0xff);

    kb_receive(fifo, last_rx);
    last_rx = fifo;
  }
}

void kb_init() {
  ps2write_program_init(pio0, 0, pio_add_program(pio0, &ps2write_program));
  ps2read_program_init(pio0, 1, pio_add_program(pio0, &ps2read_program));
  queue_init(&packets, 9, 32);
}
