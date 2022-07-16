#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/gpio.h"
#include "bsp/board.h"
#include "tusb.h"

#define PS2CLK 15
#define PS2DAT 16
#define CLKFULL 40
#define CLKHALF 20

uint8_t const map[] = {
  0, 0xff, 0xfc, 0,
  
  0x1c, 0x32, 0x21, 0x23, 0x24, /* a-e */
  0x2b, 0x34, 0x33, 0x43, 0x3b, /* f-j */
  0x42, 0x4b, 0x3a, 0x31, 0x44, /* k-o */
  0x4d, 0x15, 0x2d, 0x1b, 0x2c, /* p-t */
  0x3c, 0x2a, 0x1d, 0x22, 0x35, /* u-y */
  0x1a, // z
  
  0x16, 0x1e, 0x26, 0x25, 0x2e, 0x36, 0x3d, 0x3e, 0x46, 0x45,
  
  0x5a, 0x76, 0x66, 0x0d, 0x29, 0x4e
  
};

int main(void) {
  board_init();
  
  gpio_init(PS2CLK);
  gpio_init(PS2DAT);
  gpio_set_dir(PS2CLK, GPIO_OUT);
  gpio_set_dir(PS2DAT, GPIO_OUT);
  gpio_put(PS2CLK, false);
  gpio_put(PS2DAT, false);
  
  printf("ps2pico 0.1\r\n");
  
  tusb_init();
  while (1) { tuh_task(); }
  return 0;
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
  printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
  tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {

  if (tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD) {
    board_led_write(1);
    printf("%d %d %d %d %d %d %d %d\r\n", report[0], report[1], report[2], report[3], report[4], report[5], report[6], report[7]);
    
    for(uint8_t i = 2; i < 8; i++) {
      if(report[i]) {
        uint8_t data = map[report[i]];
        uint8_t parity = 1;
        
        gpio_put(PS2DAT, true);
        sleep_us(20);
        gpio_put(PS2CLK, true);
        sleep_us(40);
        gpio_put(PS2CLK, false);
        sleep_us(20);
        
        for(uint8_t j = 0; j < 8; j++) {
          
          if(data & 0x01) {
            gpio_put(PS2DAT, false);
          } else {
            gpio_put(PS2DAT, true);
          }
          
          sleep_us(20);
          gpio_put(PS2CLK, true);
          sleep_us(40);
          gpio_put(PS2CLK, false);
          sleep_us(20);
      
          parity = parity ^ (data & 0x01);
          data = data >> 1;
          
        }
        
        if(parity) {
          gpio_put(PS2DAT, false);
        } else {
          gpio_put(PS2DAT, true);
        }
        
        sleep_us(20);
        gpio_put(PS2CLK, true);
        sleep_us(40);
        gpio_put(PS2CLK, false);
        sleep_us(20);
        
        gpio_put(PS2DAT, false);
        sleep_us(20);
        gpio_put(PS2CLK, true);
        sleep_us(40);
        gpio_put(PS2CLK, false);
        sleep_us(20);
      }
    }
    
    board_led_write(0);
  }

  tuh_hid_receive_report(dev_addr, instance);
}
