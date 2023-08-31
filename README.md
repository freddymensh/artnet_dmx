# ArtNet DMX
This repo contains several sub-projects each for a different purpose. See subsections for it.

## ESP32_DMX_TxRx
### Overview
Two esp32 are communicating via DMX to light up a RGB-LED. The TX generates a color randomly in a set interval and fades between them. The current color-value is showm by a RGB-LED. Furthermore the TX sends the color-value via DMX to the RX. The RX recieves the color-value and displays it at the second LED.

## ESP32_ArtNet_TxRx
### Overview
There are two esp32 in the setup. The _TX_ functiones as a transcoder from ArtNet to DMX. The _RX_ recieves DMX and lights up an RGB-LED. For the ArtNet signal MagicQ was used. <br> __This sub-project is under development!__

### Wishlist
- add an LCD to display several status infos
- add AT-commands to make settings to the device (network, universe, maybe more)
- make AT-commands available via http-requests and via USB-UART
- add a second universe

### ToDo:
1. finish the Settings-class by using it as the ressource
1. write ```String Settings::json()```
1. write ```String Settings::ini()```
1. swap out the MCU for the olimex esp32 PoE

1. fork the [AT-parser](https://github.com/yourapiexpert/ATCommands)
1. add function to it to inject AT-commands from inside the code.
1. define write commands and according callbacks to Setup and others

1. add the SD memory
1. make the Settings-constructor to init from SD [(SD example)](https://raw.githubusercontent.com/OLIMEX/ESP32-POE/master/SOFTWARE/ARDUINO/ESP32_PoE_Ethernet_SD_Card_Arduino/ESP32_PoE_Ethernet_SD_Card_Arduino.ino) with the [ini-library](https://github.com/stevemarple/IniFile)
1. write ```setings_response Settings::flush()``` to update the ini on the SD

1. assign pin for U2TXD
1. make a second instance of artnetEther to listen to the other universe.
1. extend the Setup-class to meet the handling of the second universe


# Development
Development was done wit Platformio in VSCode. Due to the ESP32 mcu the platform espressif32 at version 5.0.0 was used. Ther version number is the one you select during installing the platform in Platformio. 
