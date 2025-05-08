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
#include "bsp/board_api.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"

struct {
  u8 report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
  u8 dev_addr;
  u8 modifiers;
  u8 boot[MAX_BOOT];
  u8 nkro[MAX_NKRO];
} keyboards[CFG_TUH_HID];

void tuh_hid_mount_cb(u8 dev_addr, u8 instance, u8 const* desc_report, u16 desc_len) {
  if(tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_MOUSE) return;

  keyboards[instance].report_count = tuh_hid_parse_report_descriptor(keyboards[instance].report_info, MAX_REPORT, desc_report, desc_len);
  printf("HID device address = %d, instance = %d is mounted with %u reports, ", dev_addr, instance, keyboards[instance].report_count);

  if(!tuh_hid_receive_report(dev_addr, instance)) {
    printf("ERROR: Could not register for HID(%d,%d)!\n", dev_addr, instance);
  } else {
    printf("HID(%d,%d) registered for reports\n", dev_addr, instance);
    keyboards[instance].dev_addr = dev_addr;
    keyboards[instance].modifiers = 0;
    memset(keyboards[instance].boot, 0, MAX_BOOT);
    memset(keyboards[instance].nkro, 0, MAX_NKRO);
    board_led_write(0);
  }
}

void tuh_hid_umount_cb(u8 dev_addr, u8 instance) {
  printf("HID device address = %d, instance = %d is unmounted\n", dev_addr, instance);
  keyboards[instance].dev_addr = 0;
}

void tuh_hid_report_received_cb(u8 dev_addr, u8 instance, u8 const* report, u16 len) {
  u8 const rpt_count = keyboards[instance].report_count;
  tuh_hid_report_info_t *rpt_infos = keyboards[instance].report_info;
  tuh_hid_report_info_t *rpt_info = NULL;

  if(rpt_count == 1 && rpt_infos[0].report_id == 0) {
    rpt_info = &rpt_infos[0];
  } else {
    u8 const rpt_id = report[0];
    for(u8 i = 0; i < rpt_count; i++) {
      if(rpt_id == rpt_infos[i].report_id) {
        rpt_info = &rpt_infos[i];
        break;
      }
    }
    report++;
    len--;
  }

  if(!rpt_info) return;
  board_led_write(1);
  tuh_hid_receive_report(dev_addr, instance);

  if(rpt_info->usage_page == HID_USAGE_PAGE_CONSUMER && rpt_info->usage == HID_USAGE_CONSUMER_CONTROL) {
    printf("len %d  %02x %02x %02x %02x\n", len, report[0], report[1], report[2], report[3]);
    return;
  }

  if(rpt_info->usage_page != HID_USAGE_PAGE_DESKTOP || rpt_info->usage != HID_USAGE_DESKTOP_KEYBOARD) {
    printf("UNKNOWN key  usage_page: %02x  usage: %02x\n", rpt_info->usage_page, rpt_info->usage);
    return;
  }

  if(report[0] != keyboards[instance].modifiers) {
    for(u8 i = 0; i < 8; i++) {
      if((report[0] >> i & 1) != (keyboards[instance].modifiers >> i & 1)) {
        kb_send_key(i + HID_KEY_CONTROL_LEFT, report[0] >> i & 1);
      }
    }

    keyboards[instance].modifiers = report[0];
  }

  report++;
  len--;

  if(len > 12 && len < 31) {
    for(u8 i = 0; i < len && i < MAX_NKRO; i++) {
      for(u8 j = 0; j < 8; j++) {
        if((report[i] >> j & 1) != (keyboards[instance].nkro[i] >> j & 1)) {
          kb_send_key(i*8+j, report[i] >> j & 1);
        }
      }
    }

    memcpy(keyboards[instance].nkro, report, len > MAX_NKRO ? MAX_NKRO : len);
    return;
  }

  switch(len) {
    case 8:
    case 7:
      report++;
      // fall through
    case 6:
      for(u8 i = 0; i < MAX_BOOT; i++) {
        if(keyboards[instance].boot[i]) {
          bool brk = true;

          for(u8 j = 0; j < MAX_BOOT; j++) {
            if(keyboards[instance].boot[i] == report[j]) {
              brk = false;
              break;
            }
          }

          if(brk) kb_send_key(keyboards[instance].boot[i], false);
        }
      }

      for(u8 i = 0; i < MAX_BOOT; i++) {
        if(report[i]) {
          bool make = true;

          for(u8 j = 0; j < MAX_BOOT; j++) {
            if(report[i] == keyboards[instance].boot[j]) {
              make = false;
              break;
            }
          }

          if(make) kb_send_key(report[i], true);
        }
      }

      memcpy(keyboards[instance].boot, report, MAX_BOOT);
    return;
  }

  printf("UKNOWN keyboard  len: %d\n", len);
}

s8 set_led = -1;
u8 inst_loop = 0;
u8 last_dev = 0;

s64 set_led_callback() {
  if(set_led == -1) return 50000;

  if(keyboards[inst_loop].dev_addr && last_dev != keyboards[inst_loop].dev_addr) {
    tuh_hid_set_report(keyboards[inst_loop].dev_addr, inst_loop, 0, HID_REPORT_TYPE_OUTPUT, &set_led, 1);
    last_dev = keyboards[inst_loop].dev_addr;
  }

  inst_loop++;

  if(inst_loop == CFG_TUH_HID) {
    inst_loop = 0;
    set_led = -1;
    return 100000;
  }

  return 1000;
}

int main() {
  set_sys_clock_khz(120000, true);

  board_init();
  board_led_write(1);

  printf("\n%s-%s ", PICO_PROGRAM_NAME, PICO_PROGRAM_VERSION_STRING);
  printf("PS/2+AT version\n");

  tuh_hid_set_default_protocol(HID_PROTOCOL_REPORT);
  tuh_init(BOARD_TUH_RHPORT);

  kb_init();
  add_alarm_in_ms(500, set_led_callback, NULL, false);

  while(1) {
    tuh_task();
    kb_task();
  }
}

void reset() {
  printf("\n\n *** PANIC via tinyusb: watchdog reset!\n\n");
  watchdog_enable(100, false);
}
