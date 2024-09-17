# OpenAce Conspicuity Device

> **_NOTE:_**: 
> I am in the process of uploading the necessary files to GitHub. More will be added soon, including the OpenSCAD case, KiCAD PCB files, unit tests for the source code, and additional resources.

> [!TIP]
> Please visit the [OpenAce WIKI](https://github.com/rvt/OpenAce/wiki) for build information.

The OpenAce Conspicuity device is designed for General Aviation pilots flying in areas where multiple protocols, such as OGN, Flarm, and ADS-L, are used. It can transmit and receive multiple protocols simultaneously (excluding sending ADS-B) using one or more transceiver modules. All received traffic is sent to your Electronic Flight Bag (EFB), such as SkyDemon, via the GLD90 protocol.

The device is built around the Raspberry Pi Pico 2040 and can be configured with a custom PCB that supports either two transceivers or a simpler configuration with one. In both setups, it can send and receive all protocols using time-sharing technology. The device can store configurations for multiple aircraft, which can be selected through an easy-to-use web interface.

Powered by a Li-Ion battery, the device includes a PCB with a USB-C charger. The estimated battery life is between 4 and 6 hours, though this is subject to further testing.

![KiCAD 3D Rendering](doc/img/kicadpcb.jpg)
![Soldered PCB](doc/img/solderedpcb.jpg)
![OpenScad View (Open)](doc/img/openscadopen.jpg)
![OpenScad View (Closed)](doc/img/openscadclosed.jpg)

The PCB measures approximately 8x9 cm.

## External Libraries and frameworks used

Most libraries are used 'as-is' Some of them have been slightly modified for performance reasons, compatibility or 
other reasons.

1. Raspberry PI PICO SDK [https://github.com/raspberrypi/pico-sdk](https://github.com/raspberrypi/pico-sdk)
2. FreeRTOS, OpenAce uses tasks and timers and avoids loops and runs in multi-core SMP mode. [https://www.freertos.org](https://www.freertos.org)
3. LWiP Pretty cool and sometimes confusing TCP/IP protocol suite [https://savannah.nongnu.org/projects/lwip/](https://savannah.nongnu.org/projects/lwip/)
4. ArduinoJSON for loading and storing configuration [https://arduinojson.org](https://arduinojson.org)
5. etlcpp pretty awesome library written by John Wellbelove  [https://www.etlcpp.com](https://www.etlcpp.com)
6. libcrc [https://github.com/lammertb/libcrc](https://github.com/lammertb/libcrc)
7. minnmea for parding NMEA sentences [https://github.com/kosma/minmea/](https://github.com/kosma/minmea/)
8. Catch2 for unit testing
