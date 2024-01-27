/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 No0ne (https://github.com/No0ne)
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
#include "atphy.pio.h"
#include "pico/util/queue.h"

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

queue_t qbytes;
queue_t qpacks;

u8 busy = 0;
u8 sent = 0;
u8 last_tx = 0;
u8 last_rx = 0;
u8 repeat = 0;

bool kb_enabled = true;
bool blinking = false;
u32 repeat_us;
u16 delay_ms;
alarm_id_t repeater;

u32 at_frame(u8 byte) {
  bool parity = 1;
  for(u8 i = 0; i < 8; i++) {
    parity = parity ^ (byte >> i & 1);
  }
  return ((1 << 10) | (parity << 9) | (byte << 1)) ^ 0x7ff;
}

void at_send(u8 byte) {
  queue_try_add(&qbytes, &byte);
}

void at_maybe_send_e0(u8 key) {
  if(key == HID_KEY_PRINT_SCREEN ||
     key >= HID_KEY_INSERT && key <= HID_KEY_ARROW_UP ||
     key == HID_KEY_KEYPAD_DIVIDE ||
     key == HID_KEY_KEYPAD_ENTER ||
     key == HID_KEY_APPLICATION ||
     key == HID_KEY_POWER ||
     key >= HID_KEY_GUI_LEFT && key != HID_KEY_SHIFT_RIGHT) {
    at_send(0xe0);
  }
}

void kb_set_leds(u8 byte) {
  if(byte > 7) byte = 0;
  tuh_kb_set_leds(led2ps2[byte]);
}

int64_t blink_callback(alarm_id_t id, void *user_data) {
  if(blinking) {
    kb_set_leds(KEYBOARD_LED_NUMLOCK | KEYBOARD_LED_CAPSLOCK | KEYBOARD_LED_SCROLLLOCK);
    blinking = false;
    return 500000;
  }
  
  //if(kb_addr) {
  kb_set_leds(0);
  at_send(0xaa);
  //} else {
  //  kb_send(0xfc);
  //}
  
  return 0;
}

int64_t kb_reset() {
  kb_enabled = true;
  repeat_us = 91743;
  delay_ms = 500;
  repeat = 0;
  blinking = true;
  add_alarm_in_ms(1, blink_callback, NULL, false);
}

int64_t repeat_callback() {
  if(repeat) {
    at_maybe_send_e0(repeat);
    
    if(repeat >= HID_KEY_CONTROL_LEFT && repeat <= HID_KEY_GUI_RIGHT) {
      at_send(mod2ps2[repeat - HID_KEY_CONTROL_LEFT]);
    } else {
      at_send(hid2ps2[repeat]);
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
  if(!kb_enabled) {
    printf("kb_enabled = false\n");
    return;
  }
  
  if(key == HID_KEY_PAUSE) {
    repeat = 0;
    
    if(state) {
      if(modifiers & KEYBOARD_MODIFIER_LEFTCTRL ||
         modifiers & KEYBOARD_MODIFIER_RIGHTCTRL) {
        at_send(0xe0); at_send(0x7e); at_send(0xe0); at_send(0xf0); at_send(0x7e);
      } else {
        at_send(0xe1); at_send(0x14); at_send(0x77); at_send(0xe1);
        at_send(0xf0); at_send(0x14); at_send(0xf0); at_send(0x77);
      }
    }
    
    return;
  }
  
  at_maybe_send_e0(key);
  
  if(state) {
    repeat = key;
    if(repeater) cancel_alarm(repeater);
    repeater = add_alarm_in_ms(delay_ms, repeat_callback, NULL, false);
  } else {
    if(key == repeat) repeat = 0;
    at_send(0xf0);
  }
  
  if(key >= HID_KEY_CONTROL_LEFT && key <= HID_KEY_GUI_RIGHT) {
    at_send(mod2ps2[key - HID_KEY_CONTROL_LEFT]);
  } else {
    at_send(hid2ps2[key]);
  }
}

void kb_receive(u8 byte, u8 prev_byte) {
  switch (prev_byte) {
    case 0xed: // Set LEDs
      kb_set_leds(byte);
    break;
    
    case 0xf3: // Set typematic rate and delay
      repeat_us = repeats[byte & 0x1f];
      delay_ms = delays[(byte & 0x60) >> 5];
    break;
    
    default:
      switch (byte) {
        case 0xff: // Reset
          kb_reset();
        break;
        
        case 0xee: // Echo
          at_send(0xee);
        return;
        
        case 0xf2: // Identify keyboard
          at_send(0xfa);
          at_send(0xab);
          at_send(0x83);
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
  
  at_send(0xfa);
}

void kb_task() {
  u8 i = 0;
  u8 byte;
  u8 pack[9];
  
  if(!queue_is_empty(&qbytes)) {
    printf("TX:");
    while(i < 9 && queue_try_remove(&qbytes, &byte)) {
      i++;
      pack[i] = byte;
      printf(" %02x", byte);
    }
    printf("\n");
    
    pack[0] = i;
    queue_try_add(&qpacks, &pack);
  }
  
  if(pio_interrupt_get(pio0, 0)) {
    busy = 1;
  } else {
    busy &= 2;
  }
  
  if(pio_interrupt_get(pio0, 1)) {
    sent--;
    pio_interrupt_clear(pio0, 1);
  }
  
  if(!queue_is_empty(&qpacks) && pio_sm_is_tx_fifo_empty(pio0, 0) && !busy) {
    if(queue_try_peek(&qpacks, &pack)) {
      if(sent == pack[0]) {
        sent = 0;
        queue_try_remove(&qpacks, &pack);
      } else {
        sent++;
        last_tx = pack[sent];
        busy |= 2;
        pio_sm_put(pio0, 0, at_frame(last_tx));
      }
    }
  }
  
  if(!pio_sm_is_rx_fifo_empty(pio0, 0)) {
    u32 fifo = pio_sm_get(pio0, 0) >> 23;
    
    bool parity = 1;
    for(i = 0; i < 8; i++) {
      parity = parity ^ (fifo >> i & 1);
    }
    
    if(parity != fifo >> 8) {
      pio_sm_put(pio0, 0, at_frame(0xfe));
      return;
    }
    
    if(fifo & 0xff == 0xfe) {
      pio_sm_put(pio0, 0, at_frame(last_tx));
      return;
    }
    
    while(queue_try_remove(&qbytes, &byte));
    while(queue_try_remove(&qpacks, &pack));
    sent = 0;
    
    printf("RX: %02x\n", fifo & 0xff);
    
    kb_receive(fifo, last_rx);
    last_rx = fifo;
  }
}

void kb_init() {
  atphy_program_init(pio0, 0, pio_add_program(pio0, &atphy_program));
  queue_init(&qbytes, sizeof(u8), 9);
  queue_init(&qpacks, sizeof(u8) * 9, 16);
}
