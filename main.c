#include "hardware/gpio.h"
#include "bsp/board.h"
#include "tusb.h"

#define CLKGPIO 15
#define DATGPIO 16

#define CLKFULL 40
#define CLKHALF 20
#define DTDELAY 1000

uint8_t const hid2ps2[] = {
  0x00, 0x00, 0xfc, 0x00, 0x1c, 0x32, 0x21, 0x23, 0x24, 0x2b, 0x34, 0x33, 0x43, 0x3b, 0x42, 0x4b,
  0x3a, 0x31, 0x44, 0x4d, 0x15, 0x2d, 0x1b, 0x2c, 0x3c, 0x2a, 0x1d, 0x22, 0x35, 0x1a, 0x16, 0x1e,
  0x26, 0x25, 0x2e, 0x36, 0x3d, 0x3e, 0x46, 0x45, 0x5a, 0x76, 0x66, 0x0d, 0x29, 0x4e, 0x55, 0x54,
  0x5b, 0x5d, 0x5d, 0x4c, 0x52, 0x0e, 0x41, 0x49, 0x4a, 0x58, 0x05, 0x06, 0x04, 0x0c, 0x03, 0x0b,
  0x83, 0x0a, 0x01, 0x09, 0x78, 0x07
};

void ps2_send(uint8_t data) {
  uint8_t parity = 1;
  
  gpio_put(DATGPIO, !0); sleep_us(CLKHALF);
  gpio_put(CLKGPIO, !0); sleep_us(CLKFULL);
  gpio_put(CLKGPIO, !1); sleep_us(CLKHALF);
  
  for(uint8_t i = 0; i < 8; i++) {
    gpio_put(DATGPIO, !(data & 0x01)); sleep_us(CLKHALF);
    gpio_put(CLKGPIO, !0); sleep_us(CLKFULL);
    gpio_put(CLKGPIO, !1); sleep_us(CLKHALF);
  
    parity = parity ^ (data & 0x01);
    data = data >> 1;
  }
  
  gpio_put(DATGPIO, !parity); sleep_us(CLKHALF);
  gpio_put(CLKGPIO, !0); sleep_us(CLKFULL);
  gpio_put(CLKGPIO, !1); sleep_us(CLKHALF);
  
  gpio_put(DATGPIO, !1); sleep_us(CLKHALF);
  gpio_put(CLKGPIO, !0); sleep_us(CLKFULL);
  gpio_put(CLKGPIO, !1); sleep_us(CLKHALF);
  
  sleep_us(DTDELAY);
}

void main() {
  board_init();
  
  gpio_init(CLKGPIO);
  gpio_init(DATGPIO);
  gpio_set_dir(CLKGPIO, GPIO_OUT);
  gpio_set_dir(DATGPIO, GPIO_OUT);
  gpio_put(CLKGPIO, !1);
  gpio_put(DATGPIO, !1);
  
  tusb_init();
  while (1) { tuh_task(); }
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
  tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {

  if(tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD) {
    board_led_write(1);
    
    //for(uint8_t i = 2; i < 8; i++) {
    uint8_t i = 2;
      if(report[i]) {
        ps2_send(hid2ps2[report[i]]);
      }
    //}
    
    board_led_write(0);
  }

  tuh_hid_receive_report(dev_addr, instance);
  
}
