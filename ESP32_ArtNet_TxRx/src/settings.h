#include <Arduino.h>
#include "const.h" 

#ifdef WIFI
    enum network_type_t {wifi_only, no_network};
#endif // WIFI
#ifdef ETHERNET
    enum network_type_t {ethernet_only, no_network};
#endif // ETHERNET
#if defined(WIFI) && defined(ETHERNET) 
    enum network_type_t {ethernet_only, wifi_only, ether_wifi, no_network};
#endif

enum settings_response {SETTING_OK, SETTING_NOK};

#ifdef LCD
    #include <TFT_eSPI.h>
    #ifndef _LCDSettings
    #define _LCDSettings

    class LCDSettings{
        public:
            LCDSettings();
            ~LCDSettings();
        
        protected:
            uint32_t TFT_BG_COLOR;
            uint32_t TFT_COLOR_1;
            uint32_t TFT_COLOR_2;
            int32_t TFT_LAYOUT_MARGIN_LEFT;
            int32_t TFT_LAYOUT_MARGIN_RIGHT;
            int32_t TFT_LAYOUT_MARGIN_TOP;
            int32_t TFT_LAYOUT_BORDER_THICKNESS;
            int32_t TFT_LAYOUT_BUBBLE_H1;
            int32_t TFT_LAYOUT_BUBBLE_H2;
            int32_t TFT_LAYOUT_BUBBLE_W1;
            int32_t TFT_LAYOUT_BUBBLE_W2;
            int32_t TFT_LAYOUT_CENTER;
            int32_t TFT_LAYOUT_MARGIN_IN1;
            int32_t TFT_LAYOUT_MARGIN_IN2;
            int32_t TFT_LAYOUT_SEP_H;
            int32_t TFT_LAYOUT_SEP_W;
            int32_t TFT_UPDATE_INTERVAL;
    }

    LCDSettings::LCDSettings(){
        this->TFT_BG_COLOR = 0x0000;
        this->TFT_COLOR_1  = 0xFF00;
        this->TFT_COLOR_2  = 0x2124;
        this->TFT_LAYOUT_MARGIN_LEFT  = 7;
        this->TFT_LAYOUT_MARGIN_RIGHT = this->TFT_LAYOUT_MARGIN_LEFT;
        this->TFT_LAYOUT_MARGIN_TOP   = 7;
        this->TFT_LAYOUT_BORDER_THICKNESS = 3;
        this->TFT_LAYOUT_BUBBLE_H1 = 24;
        this->TFT_LAYOUT_BUBBLE_H2 = (2*this->TFT_LAYOUT_BUBBLE_H1);
        this->TFT_LAYOUT_BUBBLE_W1 = (82 + this->TFT_LAYOUT_BUBBLE_H1);
        this->TFT_LAYOUT_BUBBLE_W2 = (TFT_WIDTH - this->TFT_LAYOUT_MARGIN_LEFT - this->TFT_LAYOUT_MARGIN_RIGHT);
        this->TFT_LAYOUT_CENTER = 120;
        this->TFT_LAYOUT_MARGIN_IN1 = 7;
        this->TFT_LAYOUT_MARGIN_IN2 = 6;
        this->TFT_LAYOUT_SEP_H = 3;
        this->TFT_LAYOUT_SEP_W = this->TFT_LAYOUT_BUBBLE_W2;

        this->TFT_UPDATE_INTERVAL = 2000;
    }

    #endif //_LCDSettings
#endif //LCD


#ifndef _Settings
#define _Settings

    class Settings{
        public: 
            Settings();
            ~Settings();

            // artnet
            settings_response set_universe(uint8_t universe);
            uint8_t get_universe();

            // DMX
            uint64_t get_dmx_buffer_size();
            
            // network
            settings_response set_network_type(network_type_t nt);
            #ifdef WIFI
            settings_response disable_wifi();
            settings_response enable_wifi();
            #endif //WIFI
            #ifdef ETHERNET
            settings_response disable_ether();
            settings_response enable_ether();
            #endif // ETHERNET        

            // wifi
            settings_response set_ssid(String ssid);
            String get_ssid();
            settings_response set_password(String password);
            String get_passwword();

            // time
            
            //lcd
            LCDSettings lcd;


        protected:
            // artnet
            uint8_t universe = 0; // the universe artnet listens to

            // DMX
            int64_t dmx_max_packet_size = 513; // the buffer size for dmx. The entry at index 0 is not used. So increase the size by +1 to fit all data in. 
            uint32_t dmx_send_interval = 1000; // interval in milli seconds until the MCU prints teh current dmx buffer onto the serial  

            // network
                // nt: seting to configure which network types can be used. It's used to enable/disable only the by hardware available ones
            #if defined(WIFI) && !defined(ETHER)
                network_type_t nt = wifi_only;  
            #endif
            #if !defined(WIFI) && defined(ETHER)
                network_type_t nt = ethernet_only;
            #endif
            #if defined(WIFI) && defined(ETHER)
                network_type_t nt = ether_wifi;
            #endif
            #if !defined(WIFI) && !defined(ETHER)
                network_type_t nt = no_network;
            #endif

            // wifi
            int32_t wifi_reconnect_trials = 50;     // reconnect trials until the MCU restarts
            String ssid;        // wifi ssid
            String password;    // wifi password

            //time
            String  ntpServer = "pool.ntp.org"; // time server
            int64_t gmtOffset_sec = 3600;       // time zone offset
            int32_t daylightOffset_sec = 3600;  // daylight saving offset

        
    };

    Settings::Settings(){
        #ifdef LCD
            this->lcd = LCDSettings();
        #endif //LCD

        // TODO: read from SD
    }
#endif //_Settings