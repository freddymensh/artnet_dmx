#include <Arduino.h>
#include <time.h>
#include "const.h"

void service_dmx_status_print(void * pvParameters);
void artnet_u1_callback(const uint8_t* data, const uint16_t size);
void print_wifi_status();
void print_dmx(byte* dmx_buffer);
void printLocalTime(bool newline=true);



// LED settings
byte LED_PINS[] = {22, 19, 5};  // pins at MCU where LEDs are connected
#define DMX_CHANNEL_OFFSET 1    // DMX base address for LEDs
#define N_CHANNELS 3            // number of channels occupied by LEDs


// time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;


// network
#ifdef ETHERNET
#else  // WiFi
    #include <WiFi.h>
    #define WIFI_RECONNECT_TRIALS 50

    const char* SSID = "FREDDY-L390 6365";//"Jubel";//"FREDDY-L390 6365";
    const char* PWD = "0936u)1E";//"69792805351678254921";//"0936u)1E";    
#endif


// artnet
#ifdef TRANSMIT
    #ifdef ETHERNET
        #include <ArtnetEther.h>
    #else // ETHERNET
        #include <ArtnetWiFi.h>
            ArtnetWiFiReceiver artnet;
            uint8_t universe0 = 0;
    #endif
#else // TRANSMIT
#endif // TRANSMIT


// DMX
#include "esp_dmx.h"
byte dmx_buffer[DMX_MAX_PACKET_SIZE] = {0};
const dmx_port_t dmx_num = DMX_NUM_2;
QueueHandle_t dmx_queue;
bool dmx_available = false;

#ifdef TRANSMIT
  #define DMX_SEND_INTERVAL 1000
  int last_dmx_send = 0;
#else
  dmx_event_t event;
  byte current_value[N_CHANNELS] = {0};
#endif


// LCD
#ifdef LCD
  //#include <gfxfont.h>
    #include "screen.h"
    
    enum wifi_status {disconnected, connected}; 
    int last_tft_update = 0;
    Screen tft = Screen();

    void service_tft_update_screen(void * pvParameters);
    void tft_update_screen();


    wifi_status tft_status_wifi = disconnected;
#endif


// debug
#ifdef TRANSMIT
  #define DMX_PRINT_INTERVAL 10000
  int last_dmx_print = 0;
#endif
