#include "hardware/gpio.h"
#include "bsp/board.h"
#include "tusb.h"

#define CLKIN  14
#define CLKOUT 15
#define DTIN   17
#define DTOUT  16

#define CLKFULL 40
#define CLKHALF 20
#define DTDELAY 1000

alarm_id_t repeater;
uint32_t repeatus = 35000;
uint16_t delayms = 250;
uint8_t repeat = 0;
bool repeating = false;
bool receiving = false;
bool sending = false;
bool enabled = true;
bool blink = false;

uint8_t kbd_addr;
uint8_t kbd_inst;

uint8_t prevhid[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t prevps2 = 0;
uint8_t lastps2 = 0;
uint8_t const mod2ps2[] = { 0x14, 0x12, 0x11, 0x1f, 0x14, 0x59, 0x11, 0x27 };
uint8_t const hid2ps2[] = {
  0x00, 0x00, 0xfc, 0x00, 0x1c, 0x32, 0x21, 0x23, 0x24, 0x2b, 0x34, 0x33, 0x43, 0x3b, 0x42, 0x4b,
  0x3a, 0x31, 0x44, 0x4d, 0x15, 0x2d, 0x1b, 0x2c, 0x3c, 0x2a, 0x1d, 0x22, 0x35, 0x1a, 0x16, 0x1e,
  0x26, 0x25, 0x2e, 0x36, 0x3d, 0x3e, 0x46, 0x45, 0x5a, 0x76, 0x66, 0x0d, 0x29, 0x4e, 0x55, 0x54,
  0x5b, 0x5d, 0x5d, 0x4c, 0x52, 0x0e, 0x41, 0x49, 0x4a, 0x58, 0x05, 0x06, 0x04, 0x0c, 0x03, 0x0b,
  0x83, 0x0a, 0x01, 0x09, 0x78, 0x07, 0x7c, 0x7e, 0x7e, 0x70, 0x6c, 0x7d, 0x71, 0x69, 0x7a, 0x74,
  0x6b, 0x72, 0x75, 0x77, 0x4a, 0x7c, 0x7b, 0x79, 0x5a, 0x69, 0x72, 0x7a, 0x6b, 0x73, 0x74, 0x6c,
  0x75, 0x7d, 0x70, 0x71, 0x61, 0x2f, 0x37, 0x0f, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40,
  0x48, 0x50, 0x57, 0x5f
};

int64_t blink_callback(alarm_id_t id, void *user_data) {
  if(blink) {
    blink = false;
    add_alarm_in_ms(1000, blink_callback, NULL, false);
    uint8_t static value = 7; tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&value, 1);
  } else {
    uint8_t static value = 0; tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&value, 1);
  }
}

bool ps2_is_e0(uint8_t data) {
  return data == 0x46 ||
         data >= 0x48 && data <= 0x52 ||
         data == 0x54 || data == 0x58 ||
         data == 0x65 || data == 0x66;
}

bool ps2_send(uint8_t data) {
  sleep_us(DTDELAY);
  
  if(!gpio_get(CLKIN)) return false;
  if(!gpio_get(DTIN)) return false;
  
  lastps2 = data;
  uint8_t parity = 1;
  sending = true;
  
  gpio_put(DTOUT, !0); sleep_us(CLKHALF);
  gpio_put(CLKOUT, !0); sleep_us(CLKFULL);
  gpio_put(CLKOUT, !1); sleep_us(CLKHALF);
  
  for(uint8_t i = 0; i < 8; i++) {
    gpio_put(DTOUT, !(data & 0x01)); sleep_us(CLKHALF);
    gpio_put(CLKOUT, !0); sleep_us(CLKFULL);
    gpio_put(CLKOUT, !1); sleep_us(CLKHALF);
  
    parity = parity ^ (data & 0x01);
    data = data >> 1;
  }
  
  gpio_put(DTOUT, !parity); sleep_us(CLKHALF);
  gpio_put(CLKOUT, !0); sleep_us(CLKFULL);
  gpio_put(CLKOUT, !1); sleep_us(CLKHALF);
  
  gpio_put(DTOUT, !1); sleep_us(CLKHALF);
  gpio_put(CLKOUT, !0); sleep_us(CLKFULL);
  gpio_put(CLKOUT, !1); sleep_us(CLKHALF);
  
  sending = false;
  return true;
}

void ps2_receive() {
  sending = true;
  uint16_t data = 0x00;
  uint16_t bit = 0x01;
  
  uint8_t cp = 1;
  uint8_t rp = 0;
  
  sleep_us(CLKHALF);
  gpio_put(CLKOUT, !0); sleep_us(CLKFULL);
  gpio_put(CLKOUT, !1); sleep_us(CLKHALF);

  while(bit < 0x0100) {
    if(gpio_get(DTIN)) {
      data = data | bit;
      cp = cp ^ 1;
    } else {
      cp = cp ^ 0;
    }

    bit = bit << 1;
    
    sleep_us(CLKHALF);
    gpio_put(CLKOUT, !0); sleep_us(CLKFULL);
    gpio_put(CLKOUT, !1); sleep_us(CLKHALF);
  }

  rp = gpio_get(DTIN);

  sleep_us(CLKHALF);
  gpio_put(CLKOUT, !0); sleep_us(CLKFULL);
  gpio_put(CLKOUT, !1); sleep_us(CLKHALF);

  sleep_us(CLKHALF);
  gpio_put(DTOUT, !0);
  gpio_put(CLKOUT, !0); sleep_us(CLKFULL);
  gpio_put(CLKOUT, !1); sleep_us(CLKHALF);
  gpio_put(DTOUT, !1);
  
  uint8_t received = data & 0x00ff;
  sending = false;
  
  if(rp == cp) {
    
    if(received == 0xff) {
      while(!ps2_send(0xaa));
      enabled = true;
      blink = true;
      add_alarm_in_ms(100, blink_callback, NULL, false);
      return;
      
    } else if(received == 0xee) {
      ps2_send(0xee);
      return;
      
    } else if(received == 0xfe) {
      ps2_send(lastps2);
      return;
      
    } else if(received == 0xf4) {
      enabled = true;
      return;
      
    } else if(received == 0xf5 || received == 0xf6) {
      enabled = received == 0xf6;
      repeatus = 35000;
      delayms = 250;
      uint8_t static value = 0; tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&value, 1);
      return;
      
    } else if(received == 0xf2) {
      ps2_send(0xfa);
      ps2_send(0xab);
      ps2_send(0x83);
      return;
    }
    
    if(prevps2 == 0xf3) {
      repeatus = received & 0x1f;
      delayms = received & 0x60;
      
      repeatus = 35000 + repeatus * 15000;
      
      if(delayms == 0x00) delayms = 250;
      if(delayms == 0x20) delayms = 500;
      if(delayms == 0x40) delayms = 750;
      if(delayms == 0x60) delayms = 1000;
      
    } else if(prevps2 == 0xed) {
      if(received == 1) { uint8_t static value = 4; tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&value, 1); } else
      if(received == 2) { uint8_t static value = 1; tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&value, 1); } else
      if(received == 3) { uint8_t static value = 5; tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&value, 1); } else
      if(received == 4) { uint8_t static value = 2; tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&value, 1); } else
      if(received == 5) { uint8_t static value = 6; tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&value, 1); } else
      if(received == 6) { uint8_t static value = 3; tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&value, 1); } else
      if(received == 7) { uint8_t static value = 7; tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&value, 1); } else
                        { uint8_t static value = 0; tuh_hid_set_report(kbd_addr, kbd_inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&value, 1); }
    }
    
    prevps2 = received;
    ps2_send(0xfa);
    
  } else {
    ps2_send(0xfe);
  }
}

int64_t repeat_callback(alarm_id_t id, void *user_data) {
  if(repeat) {
    repeating = true;
    return repeatus;
  }
  
  repeater = 0;
  return 0;
}

void gpio_callback(uint gpio, uint32_t events) {
  if(!sending && !gpio_get(DTIN)) {
    receiving = true;
  }
}

void main() {
  board_init();
  
  gpio_init(CLKOUT);
  gpio_init(DTOUT);
  gpio_init(CLKIN);
  gpio_init(DTIN);
  gpio_set_dir(CLKOUT, GPIO_OUT);
  gpio_set_dir(DTOUT, GPIO_OUT);
  gpio_set_dir(CLKIN, GPIO_IN);
  gpio_set_dir(DTIN, GPIO_IN);
  gpio_put(CLKOUT, !1);
  gpio_put(DTOUT, !1);
  
  gpio_set_irq_enabled_with_callback(CLKIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
  
  tusb_init();
  while(true) {
    tuh_task();
    
    if(repeating) {
      repeating = false;
      
      if(repeat) {
        if(ps2_is_e0(repeat)) ps2_send(0xe0);
        ps2_send(hid2ps2[repeat]);
      }
    }
    
    if(receiving) {
      receiving = false;
      ps2_receive();
    }
  }
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
  kbd_addr = dev_addr;
  kbd_inst = instance;
  tuh_hid_receive_report(dev_addr, instance);
  blink = true;
  add_alarm_in_ms(100, blink_callback, NULL, false);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {

  if(enabled && tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD) {
    board_led_write(1);
    
    kbd_addr = dev_addr;
    kbd_inst = instance;
    
    if(report[0] != prevhid[0]) {
      uint8_t rbits = report[0];
      uint8_t pbits = prevhid[0];
      
      for(uint8_t j = 0; j < 8; j++) {
        
        if((rbits & 0x01) != (pbits & 0x01)) {
          if(j > 2 && j != 5) ps2_send(0xe0);
          
          if(rbits & 0x01) {
            ps2_send(mod2ps2[j]);
          } else {
            ps2_send(0xf0);
            ps2_send(mod2ps2[j]);
          }
        }
        
        rbits = rbits >> 1;
        pbits = pbits >> 1;
        
      }
      
      prevhid[0] = report[0];
    }
    
    for(uint8_t i = 2; i < 8; i++) {
      if(prevhid[i]) {
        bool brk = true;
        
        for(uint8_t j = 2; j < 8; j++) {
          if(prevhid[i] == report[j]) {
            brk = false;
            break;
          }
        }
        
        if(brk) {
          if(prevhid[i] == 0x48) continue;
          repeat = 0;
          
          if(ps2_is_e0(prevhid[i])) ps2_send(0xe0);
          ps2_send(0xf0);
          ps2_send(hid2ps2[prevhid[i]]);
        }
      }
      
      if(report[i]) {
        bool make = true;
        
        for(uint8_t j = 2; j < 8; j++) {
          if(report[i] == prevhid[j]) {
            make = false;
            break;
          }
        }
        
        if(make) {
          if(report[i] == 0x48) {
            ps2_send(0xe1); ps2_send(0x14); ps2_send(0x77); ps2_send(0xe1);
            ps2_send(0xf0); ps2_send(0x14); ps2_send(0xf0); ps2_send(0x77);
            continue;
          }
          
          repeat = report[i];
          if(repeater) cancel_alarm(repeater);
          repeater = add_alarm_in_ms(delayms, repeat_callback, NULL, false);
          
          if(ps2_is_e0(report[i])) ps2_send(0xe0);
          ps2_send(hid2ps2[report[i]]);
        }
      }
      
      prevhid[i] = report[i];
    }
    
    board_led_write(0);
  }

  tuh_hid_receive_report(dev_addr, instance);
  
}
