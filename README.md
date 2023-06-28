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

# Development
Development was done wit Platformio in VSCode. Due to the ESP32 mcu the platform espressif32 at version 5.0.0 was used. Ther version number is the one you select during installing the platform in Platformio. 
