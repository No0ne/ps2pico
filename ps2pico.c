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

#include "hardware/gpio.h"
#include "bsp/board.h"
#include "tusb.h"

#define CLKIN  14
#define CLKOUT 12
#define DTIN   17
#define DTOUT  11

uint8_t const led2ps2[] = { 0, 4, 1, 5, 2, 6, 3, 7 };
uint8_t const mod2ps2[] = { 0x1d, 0x2a, 0x38, 0x5b, 0x1d, 0x36, 0x38, 0x5c };
uint8_t const hid2ps2[] = {
  0x00, 0xff, 0xfc, 0x00, 0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26,
  0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x1c, 0x01, 0x0e, 0x0f, 0x39, 0x0c, 0x0d, 0x1a,
  0x1b, 0x2b, 0x2b, 0x27, 0x28, 0x29, 0x33, 0x34, 0x35, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
  0x41, 0x42, 0x43, 0x44, 0x57, 0x58, 0x37, 0x46, 0x46, 0x52, 0x47, 0x49, 0x53, 0x4f, 0x51, 0x4d,
  0x4b, 0x50, 0x48, 0x45, 0x35, 0x37, 0x4a, 0x4e, 0x1c, 0x4f, 0x50, 0x51, 0x4b, 0x4c, 0x4d, 0x47,
  0x48, 0x49, 0x52, 0x53, 0x56, 0x5d, 0x5e, 0x59, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
  0x6c, 0x6d, 0x6e, 0x76
};
uint8_t const maparray = sizeof(hid2ps2) / sizeof(uint8_t);

bool irq_enabled = true;
bool kbd_enabled = true;
uint8_t kbd_addr = 0;
uint8_t kbd_inst = 0;

bool blinking = false;
bool receiving = false;
bool repeating = false;
uint32_t repeat_us = 35000;
uint16_t delay_ms = 250;
alarm_id_t repeater;

uint8_t prev_rpt[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t prev_ps2 = 0;
uint8_t resend = 0;
uint8_t repeat = 0;
uint8_t leds = 0;

int64_t repeat_callback(alarm_id_t id, void *user_data) {
  if(repeat) {
    repeating = true;
    return repeat_us;
  }
  
  repeater = 0;
  return 0;
}

void ps2_cycle_clock() {
  sleep_us(8);
  gpio_put(CLKOUT, 1);
  sleep_us(63);
  gpio_put(CLKOUT, 0);
  sleep_us(22);
}

void ps2_set_bit(bool bit) {
  gpio_put(DTOUT, bit);
  ps2_cycle_clock();
}

void ps2_send(uint8_t data) {
  /*uint8_t timeout = 10;
  sleep_ms(1);
  
  while(timeout) {
    if(gpio_get(CLKIN) && gpio_get(DTIN)) {*/
      
      resend = data;
      uint8_t parity = 1;
      irq_enabled = false;
      
      gpio_put(DTOUT, 0); sleep_us(5);
      gpio_put(CLKOUT, 0); sleep_us(5);
      gpio_put(DTOUT, 1); sleep_us(110);
      ps2_cycle_clock();
      
      for(uint8_t i = 0; i < 8; i++) {
        ps2_set_bit(data & 0x01);
        data = data >> 1;
      }
      
      //ps2_set_bit(1);
      
      gpio_put(CLKOUT, 1);
      gpio_put(DTOUT, 1);
      sleep_ms(1);
      
      irq_enabled = true;
      return;
      
  /*  }
    
    timeout--;
    sleep_ms(8);
  }*/
}

void maybe_send_e0(uint8_t data) {
  if(data == 0x46 ||
     data >= 0x48 && data <= 0x52 ||
     data == 0x54 || data == 0x58 ||
     data == 0x65 || data == 0x66) {
    ps2_send(0xe0);
  }
}

void kbd_set_leds(uint8_t data) {
  if(data > 7) data = 0;
  leds = led2ps2[data];
  tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, &leds, sizeof(leds));
}

int64_t blink_callback(alarm_id_t id, void *user_data) {
  if(kbd_addr) {
    if(blinking) {
      kbd_set_leds(7);
      blinking = false;
      return 500000;
    } else {
      kbd_set_leds(0);
    }
  }
  return 0;
}


void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
  if(tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD) {
    kbd_addr = dev_addr;
    kbd_inst = instance;
    
    blinking = true;
    add_alarm_in_ms(1, blink_callback, NULL, false);
    
    tuh_hid_receive_report(dev_addr, instance);
  }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  if(dev_addr == kbd_addr && instance == kbd_inst) {
    kbd_addr = 0;
    kbd_inst = 0;
  }
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
  if(dev_addr == kbd_addr && instance == kbd_inst) {
    
    if(!kbd_enabled || report[1] != 0) {
      tuh_hid_receive_report(dev_addr, instance);
      return;
    }
    
    board_led_write(1);
    
    if(report[0] != prev_rpt[0]) {
      uint8_t rbits = report[0];
      uint8_t pbits = prev_rpt[0];
      
      for(uint8_t j = 0; j < 8; j++) {
        
        if((rbits & 0x01) != (pbits & 0x01)) {
          if(j > 2 && j != 5) ps2_send(0xe0);
          
          if(rbits & 0x01) {
            ps2_send(mod2ps2[j]);
          } else {
            ps2_send(mod2ps2[j] ^ 0x80);
          }
        }
        
        rbits = rbits >> 1;
        pbits = pbits >> 1;
        
      }
      
      prev_rpt[0] = report[0];
    }
    
    for(uint8_t i = 2; i < 8; i++) {
      if(prev_rpt[i]) {
        bool brk = true;
        
        for(uint8_t j = 2; j < 8; j++) {
          if(prev_rpt[i] == report[j]) {
            brk = false;
            break;
          }
        }
        
        if(brk && report[i] < maparray) {
          if(prev_rpt[i] == 0x48) continue;
          if(prev_rpt[i] == repeat) repeat = 0;
          
          maybe_send_e0(prev_rpt[i]);
          ps2_send(hid2ps2[prev_rpt[i]] ^ 0x80);
        }
      }
      
      if(report[i]) {
        bool make = true;
        
        for(uint8_t j = 2; j < 8; j++) {
          if(report[i] == prev_rpt[j]) {
            make = false;
            break;
          }
        }
        
        if(make && report[i] < maparray) {
          /*if(report[i] == 0x48) {
            
            if(report[0] & 0x1 || report[0] & 0x10) {
              ps2_send(0xe0); ps2_send(0x7e); ps2_send(0xe0); ps2_send(0xf0); ps2_send(0x7e);
            } else {
              ps2_send(0xe1); ps2_send(0x14); ps2_send(0x77); ps2_send(0xe1);
              ps2_send(0xf0); ps2_send(0x14); ps2_send(0xf0); ps2_send(0x77);
            }
            
            continue;
          }*/
          
          repeat = report[i];
          if(repeater) cancel_alarm(repeater);
          repeater = add_alarm_in_ms(delay_ms, repeat_callback, NULL, false);
          
          maybe_send_e0(report[i]);
          ps2_send(hid2ps2[report[i]]);
        }
      }
      
      prev_rpt[i] = report[i];
    }
    
    tuh_hid_receive_report(dev_addr, instance);
    board_led_write(0);
    
  }
}

void irq_callback(uint gpio, uint32_t events) {
  if(irq_enabled && !gpio_get(DTIN)) {
    receiving = true;
  }
}

void main() {
  board_init();
  
  gpio_init(CLKOUT);
  gpio_init(DTOUT);
  //gpio_init(CLKIN);
  //gpio_init(DTIN);
  gpio_set_dir(CLKOUT, GPIO_OUT);
  gpio_set_dir(DTOUT, GPIO_OUT);
  //gpio_set_dir(CLKIN, GPIO_IN);
  //gpio_set_dir(DTIN, GPIO_IN);
  gpio_put(CLKOUT, 1);
  gpio_put(DTOUT, 0);
  
  gpio_init(13);
  gpio_set_dir(13, GPIO_OUT);
  gpio_put(13, 1);
  
  //gpio_set_irq_enabled_with_callback(CLKIN, GPIO_IRQ_EDGE_RISE, true, &irq_callback);
  tusb_init();
  
  while(true) {
    tuh_task();
    
    if(repeating) {
      repeating = false;
      
      if(repeat) {
        maybe_send_e0(repeat);
        ps2_send(hid2ps2[repeat]);
      }
    }
  }
}
