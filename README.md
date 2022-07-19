# ps2pico
USB keyboard to PS/2 interface converter using a Raspberry Pi Pico

![Photo](https://raw.githubusercontent.com/No0ne/ps2pico/main/photo.jpg)

WORK-IN-PROGRESS
(final version will be written in Rust https://github.com/No0ne/ps2pico-rs)

# Usage
* Copy `ps2pico.uf2` to your Pi Pico by pressing BOOTSEL before pluggging in.
* Afterwards connect a USB keyboard using an OTG-adapter and PS/2 5V to Pico VBUS.
* 3.3V/5V conversion is done with two NPN transistors and six resistors as shown below:
```
                   PS/2 CLOCK      ____
                       |__________|2k2 |__________ GPIO 14
            ____       |          |____|   __|__
GPIO 15 ___|2k2 |____|/   BC547            |3k3|
           |____|    |\e                   |___|
                       |                     |
                  _____|__GND________________|____


                   PS/2 DATA      ____
                       |_________|2k2 |___________ GPIO 17
            ____       |         |____|    __|__
GPIO 16 ___|2k2 |____|/   BC547            |3k3|
           |____|    |\e                   |___|
                       |                     |
                  _____|__GND________________|____
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
https://wiki.osdev.org/PS/2_Keyboard
https://github.com/Harvie/ps2dev/blob/master/src/ps2dev.cpp
http://www.lucadavidian.com/2017/11/15/interfacing-ps2-keyboard-to-a-microcontroller/
https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/translate.pdf