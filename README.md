# ps2pico
USB keyboard to PS/2 interface converter using a Raspberry Pi Pico

![Photo](https://raw.githubusercontent.com/No0ne/ps2pico/main/photo.jpg)

WORK-IN-PROGRESS
(final version will be written in Rust https://github.com/No0ne/ps2pico-rs)

**Currently NOT working PC->keyboard communication for e.g. keyboard leds (num, caps and scroll lock)**

# Usage
* Copy `ps2pico.uf2` to your Pi Pico by pressing BOOTSEL before pluggging in.
* Afterwards connect a USB keyboard using an OTG-adapter and PS/2 5V to Pico VBUS.
* 3.3V to 5V conversion is done with two NPN transistors as shown below:
```
                   PS/2 CLOCK
                       |
            ____       |
GPIO 15 ___|2k2 |____|/   BC547
           |____|    |\e
                       |
                  _____|__GND


                   PS/2 DATA
                       |
            ____       |
GPIO 16 ___|2k2 |____|/   BC547
           |____|    |\e
                       |
                  _____|__GND
```

# Build
```
export PICO_SDK_PATH=/path/to/pico-sdk
mkdir build
cd build
cmake ..
make
```

# Resources
https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/translate.pdf