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
#include "bsp/board.h"
#include "tusb.h"

u8 kb_addr = 0;
u8 kb_inst = 0;
u8 kb_leds = 0;

u8 prev_rpt[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

void tuh_kb_set_leds(u8 leds) {
  if(kb_addr) {
    kb_leds = leds;
    printf("HID device address = %d, instance = %d, LEDs = %d\n", kb_addr, kb_inst, kb_leds);
    tuh_hid_set_report(kb_addr, kb_inst, 0, HID_REPORT_TYPE_OUTPUT, &kb_leds, sizeof(kb_leds));
  }
}

void tuh_hid_mount_cb(u8 dev_addr, u8 instance, u8 const* desc_report, u16 desc_len) {
  printf("HID device address = %d, instance = %d is mounted", dev_addr, instance);
  
  if(tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD) {
    printf(" - keyboard");
    
    if(!kb_addr) {
      printf(", primary");
      kb_addr = dev_addr;
      kb_inst = instance;
      kb_reset();
    }
    
    tuh_hid_receive_report(dev_addr, instance);
  }
  
  printf("\n");
}

void tuh_hid_umount_cb(u8 dev_addr, u8 instance) {
  printf("HID device address = %d, instance = %d is unmounted", dev_addr, instance);
  
  if(dev_addr == kb_addr && instance == kb_inst) {
    printf(" - keyboard, primary");
    kb_addr = 0;
    kb_inst = 0;
  }
  
  printf("\n");
}

void tuh_hid_report_received_cb(u8 dev_addr, u8 instance, u8 const* report, u16 len) {
  if(tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD && report[1] == 0) {
    
    if(report[0] != prev_rpt[0]) {
      u8 rbits = report[0];
      u8 pbits = prev_rpt[0];
      
      for(u8 j = 0; j < 8; j++) {
        if((rbits & 1) != (pbits & 1)) {
          kb_send_key(HID_KEY_CONTROL_LEFT + j, rbits & 1, report[0]);
        }
        
        rbits = rbits >> 1;
        pbits = pbits >> 1;
      }
    }
    
    for(u8 i = 2; i < 8; i++) {
      if(prev_rpt[i]) {
        bool brk = true;
        
        for(u8 j = 2; j < 8; j++) {
          if(prev_rpt[i] == report[j]) {
            brk = false;
            break;
          }
        }
        
        if(brk) {
          kb_send_key(prev_rpt[i], false, report[0]);
        }
      }
      
      if(report[i]) {
        bool make = true;
        
        for(u8 j = 2; j < 8; j++) {
          if(report[i] == prev_rpt[j]) {
            make = false;
            break;
          }
        }
        
        if(make) {
          kb_send_key(report[i], true, report[0]);
        }
      }
    }
    
    memcpy(prev_rpt, report, sizeof(prev_rpt));
    
  }
  
  tuh_hid_receive_report(dev_addr, instance);
}

void main() {
  board_init();
  tuh_init(BOARD_TUH_RHPORT);
  kb_init();
  
  printf("\n%s-%s ", PICO_PROGRAM_NAME, PICO_PROGRAM_VERSION_STRING);
  #ifdef ATPHY
    printf("PS/2+AT version\n");
  #endif
  #ifdef XTPHY
    printf("XT version\n");
  #endif
  
  while(1) {
    tuh_task();
    #ifdef ATPHY
      kb_task();
    #endif
  }
}
