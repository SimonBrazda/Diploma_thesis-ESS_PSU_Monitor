# Diploma Thesis: A Module for Power Supply Analysis of Electronic Security Systems

## Description
**TODO: Add description**

## Usage
**TODO: Add usage**

## Installation
To be able to use this project right away with all its blows and whistles you will need working [Visual Studio Code IDE](https://code.visualstudio.com/) with [Platformio](https://platformio.org/) extension and [Arduino IDE](https://www.arduino.cc/en/software). Easy to follow list of required software and libraries is at [Requirements](#requirements). It is possible to use this project in vanila Arduino IDE or other IDEs, however you will need to do some extra work to get it working.

1. Clone thi repo with `git clone https://github.com/SimonBrazda/Diploma_thesis-ESS_PSU_Monitor` or download the zip file.

### VS Code & PlatformIO
2. Install [VS Code](https://code.visualstudio.com/Download).
3. Install [Arduino IDE](https://www.arduino.cc/en/software).
4. Install [Platformio](https://platformio.org/platformio-ide).
5. Open VS Code in the projects directory.
6. Wait for PIO server to launch and download all the nessesary dependencies.
7. [Build](#build) and upload.

### Arduino IDE
2. Install [Arduino IDE](https://www.arduino.cc/en/software).
3. Download nessesary [libraries](#libraries) into your Arduino libraries directory such as `$HOME/Arduino/libraries/`.
4. Extract all build flags specified in `platformio.pio` as a preprocessor directive defines (`#define USER_SETUP_LOADED 1`, etc.) either right on top of `main.cpp` or into a separate header file that you then include on top of `main.cpp`.
5. [Build](#build) and upload.

#### Build
To be able to build the project you will need to make sure that **`ClickEncoder.h`** is included in **`clickEncoder.h`** of **ArduinoMenu library**. It can be ensured in many ways however I recommend commenting out preprocessor directive _#ifndef ARDUINO_SAM_DUE_. After the change the header should look like this:
```cpp
#ifndef __ClickEncoderStream_h__
  #define __ClickEncoderStream_h__

  #include <Arduino.h>

//   #ifndef ARDUINO_SAM_DUE
    // Arduino specific libraries
    // #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega328P__)
      // #include <stdint.h>
      //#include <avr/io.h>
      //#include <avr/interrupt.h>
      #include <ClickEncoder.h>
    // #endif

    #include "../menuDefs.h"

    namespace Menu {
    .
    .
    .
    // #endif
```
## Prerequsities
[Arduino IDE](https://www.arduino.cc/en/software)

### Libraries
* [ArduinoMenu v4.21.3+](https://github.com/neu-rah/ArduinoMenu) by Rui Azevedo
* [SAMDUE_TimerInterrupt v1.2.0+](https://github.com/khoih-prog/SAMDUE_TimerInterrupt) by Khoi Hoang
* [SD v1.2.4+](https://github.com/arduino-libraries/SD)
* [ArduinoJson v6.17.2+](https://github.com/bblanchon/ArduinoJson) by Benoît Blanchon
* [TFT_eSPI v2.3.58+](https://github.com/Bodmer/TFT_eSPI) by Bodmer
* [ClickEncoder v0.0.0+](https://github.com/0xPIT/encoder/blob/master/ClickEncoder.h) by ToeKnee
* [I2C_EEPROM v1.4.3+](https://github.com/RobTillaart/I2C_EEPROM) by Rob Tillaart
* [Rtc v2.3.5+](https://github.com/Makuna/Rtc) by Michael Miller
* [SPI](https://github.com/arduino/ArduinoCore-avr/tree/master/libraries/SPI)

### Hardware
* [Arduino DUE](https://www.arduino.cc/en/Guide/ArduinoDue)

## Thanks
Big thanks to all contributors to these awesome [libraries](#libraries)! Without them this project would not be possible.

## Support
If you find any issues with the project or have any questions feel free to contact me at [simonbrazda@seznam.cz](mailto:simonbrazda@seznam.cz).

## License
The library is licensed under [GNU LGPLv3](https://www.gnu.org/licenses/lgpl-3.0.html)