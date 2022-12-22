# ps2pico - XT version
USB keyboard to XT interface converter using a Raspberry Pi Pico

|![hw1](https://raw.githubusercontent.com/No0ne/ps2pico/main/hw1.jpg) |![hw2](https://raw.githubusercontent.com/No0ne/ps2pico/main/hw2.jpg) |![hw3](https://raw.githubusercontent.com/No0ne/ps2pico/main/hw3.jpg) |![hw4](https://raw.githubusercontent.com/No0ne/ps2pico/main/hw4.jpg) |
|-|-|-|-|

PS/2 / AT version: https://github.com/No0ne/ps2pico

Keyboard + Mouse variant: https://github.com/No0ne/ps2x2pico

# Usage
* Copy `ps2pico.uf2` to your Pi Pico by pressing BOOTSEL before pluggging in.
* Afterwards connect a USB keyboard using an OTG-adapter and XT 5V to Pico VBUS.
* Also works with wireless keyboards with a dedicated USB receiver.
* 3.3V/5V conversion is done with two NPN transistors, two zener diodes and four resistors as shown below:
```
                     XT CLOCK
                       |           ____
                       |__________|10k |___________ GPIO 14
            ____       |          |____|     |
GPIO 15 ___|2k2 |____|/  BC547             __|__
           |____|    |\e                    / \  3V6
                       |                     |
                   ____|__GND________________|___


                     XT DATA
                       |          ____
                       |_________|10k |____________ GPIO 17
            ____       |         |____|      |
GPIO 16 ___|2k2 |____|/  BC547             __|__
           |____|    |\e                    / \  3V6
                       |                     |
                   ____|__GND________________|___
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
* https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/translate.pdf
* https://github.com/kesrut/pcxtkbd/blob/master/XT_KEYBOARD.ino