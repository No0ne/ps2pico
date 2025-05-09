# ps2pico
USB keyboard to PS/2+AT or XT interface converter using a Raspberry Pi Pico

|![hw1](https://raw.githubusercontent.com/No0ne/ps2pico/main/doc/hw1.jpg) |![hw2](https://raw.githubusercontent.com/No0ne/ps2pico/main/doc/hw2.jpg) |![hw3](https://raw.githubusercontent.com/No0ne/ps2pico/main/doc/hw3.jpg) |![hw4](https://raw.githubusercontent.com/No0ne/ps2pico/main/doc/hw4.jpg) |
|-|-|-|-|

Keyboard + Mouse variant: https://github.com/No0ne/ps2x2pico

Additional excellent documentation by Ray: https://minuszerodegrees.net/keyboard/ps2pico.htm

# Usage
* Download `ps2pico.uf2` or `ps2pico-XT.uf2` from https://github.com/No0ne/ps2pico/releases
* Copy `ps2pico.uf2` or `ps2pico-XT.uf2` to your Pi Pico by pressing BOOTSEL before pluggging in.
* Afterwards connect a USB keyboard using an OTG-adapter and PS/2+AT or XT 5V to Pico VBUS.
* Also works with wireless keyboards with a dedicated USB receiver.
* 3.3V/5V conversion is done with two NPN transistors, two zener diodes and four resistors as shown below:
```
                 PS/2+AT / XT CLOCK
                       |           ____
                       |__________|10k |___________ GPIO 14
            ____       |          |____|     |
GPIO 15 ___|2k2 |____|/  BC547             __|__
           |____|    |\e                    / \  3V6
                       |                     |
                   ____|__GND________________|___


                 PS/2+AT / XT DATA
                       |          ____
                       |_________|10k |____________ GPIO 17
            ____       |         |____|      |
GPIO 16 ___|2k2 |____|/  BC547             __|__
           |____|    |\e                    / \  3V6
                       |                     |
                   ____|__GND________________|___
```
![ps2pico](https://github.com/No0ne/ps2pico/assets/716129/b0133c44-c170-40f4-a3ad-c545aee92532)

# NuXTv2
If you have a [NuXTv2](https://monotech.fwscart.com/NuXT_v20_-_MicroATX_Turbo_XT_-_10MHz_832K_XT-IDE_Multi-IO_SVGA/p6083514_19777986.aspx) you can build an internal version of the ps2pico-XT! Replace U10 with the pico, remove RN13 and add two 4k7 pull-up resistors as shown below:

|![hw5](https://raw.githubusercontent.com/No0ne/ps2pico/main/doc/hw5.jpg) |![hw6](https://raw.githubusercontent.com/No0ne/ps2pico/main/doc/hw6.jpg) |![hw7](https://raw.githubusercontent.com/No0ne/ps2pico/main/doc/hw7.jpg) |
|-|-|-|

# Build
```
export PICO_SDK_PATH=/path/to/pico-sdk
mkdir build
cd build
cmake ..
make
```

# Resources
* https://wiki.osdev.org/PS/2_Keyboard
* https://github.com/Harvie/ps2dev/blob/master/src/ps2dev.cpp
* http://www.lucadavidian.com/2017/11/15/interfacing-ps2-keyboard-to-a-microcontroller/
* https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/translate.pdf
* https://github.com/tmk/tmk_keyboard/wiki/IBM-PC-XT-Keyboard-Protocol
* https://github.com/AndersBNielsen/pcxtkbd/blob/master/XT_KEYBOARD.ino
