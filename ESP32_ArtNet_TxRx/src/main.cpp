#include <Arduino.h>
#include <time.h>

void dmx_status_print(void * pvParameters);
void artnet_u1_callback(const uint8_t* data, const uint16_t size);
void print_dmx(byte* dmx_buffer);
void printLocalTime(bool newline=true);

/*
** TX: The one which recieves ArtNet and sends it on DMX. The TX shows the data on its attached LEDs
** RX: The one recieving DMX and lightning up the LEDs
*/
#define TX

/*
** Switch for ethernet-based or WiFi-based network
*/
//#define ETHERNET


// LED settings
byte LED_PINS[] = {22, 19, 5}; // pins at MCU where LEDs are connected
#define DMX_CHANNEL_OFFSET 1    // DMX base address for LEDs
#define N_CHANNELS 3            // number of channels occupied by LEDs


// time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;


// network
#ifdef TX
  #ifdef ETHERNET
    #include <ArtnetEther.h>
  #else  // WiFi
    const char* SSID = "FREDDY-L390 6365";//"Jubel";//"FREDDY-L390 6365";
    const char* PWD = "0936u)1E";//"69792805351678254921";//"0936u)1E";
    
    #include <ArtnetWiFi.h>
    ArtnetWiFiReceiver artnet;
    uint8_t universe0 = 0;
  #endif
#else
#endif


// DMX
#include "esp_dmx.h"
byte dmx_buffer[DMX_MAX_PACKET_SIZE] = {0};
const dmx_port_t dmx_num = DMX_NUM_2;
QueueHandle_t dmx_queue;
bool dmx_available = false;

#ifdef TX
  #define DMX_SEND_INTERVAL 1000
  int last_dmx_send = 0;
#else
  dmx_event_t event;
  byte current_value[N_CHANNELS] = {0};
#endif

// LCD
#define LCD
#ifdef LCD
  #ifdef TX
    #include <TFT_eSPI.h>
    #define TFT_BG_COLOR TFT_BLACK
    #define TFT_COLOR_1 0xFF00
    #define TFT_COLOR_2 0x2124
    #define TFT_LAYOUT_MARGIN_LEFT 7
    #define TFT_LAYOUT_MARGIN_RIGHT TFT_LAYOUT_MARGIN_LEFT
    #define TFT_LAYOUT_MARGIN_TOP  7
    #define TFT_LAYOUT_BORDER_THICKNESS 3
    #define TFT_LAYOUT_BUBBLE_H1 24
    #define TFT_LAYOUT_BUBBLE_H2 (2*TFT_LAYOUT_BUBBLE_H1)
    #define TFT_LAYOUT_BUBBLE_W1 (82 + TFT_LAYOUT_BUBBLE_H1)
    #define TFT_LAYOUT_BUBBLE_W2 (TFT_WIDTH-TFT_LAYOUT_MARGIN_LEFT-TFT_LAYOUT_MARGIN_RIGHT)
    #define TFT_LAYOUT_CENTER 120
    #define TFT_LAYOUT_MARGIN_IN1 7
    #define TFT_LAYOUT_MARGIN_IN2 6
    #define TFT_LAYOUT_SEP_H 3
    #define TFT_LAYOUT_SEP_W TFT_LAYOUT_BUBBLE_W2
    TFT_eSPI tft = TFT_eSPI();
    void tft_bubble_H1_solid(int32_t x, int32_t y, int32_t width, int32_t height, int32_t color, int32_t bg_color);
    void tft_bubble_H1_hollow(int32_t x, int32_t y, int32_t width, int32_t height, int32_t border, int32_t color, int32_t bg_color);
    void tft_draw_wifi_bubble(bool hollow);
    void tft_draw_ip_bubble();
    void tft_draw_ethernet_bubble(bool hollow);
    void tft_draw_url_bubble();
    void tft_horizontal_sep1();
    void tft_horizontal_sep2();
  #endif
#endif

// debug
#ifdef TX
  #define DMX_PRINT_INTERVAL 10000
  int last_dmx_print = 0;
#endif


//////////////////////////////////////////
///               SETUP                ///
//////////////////////////////////////////

void setup() {
  // set up serial port
  Serial.begin(115200);

  // print status string
  Serial.print("ESP32_ArtNet_TxRx and DMX-");
  #ifdef TX
    Serial.print("TX\n");
  #else
    Serial.print("RX\n");
  #endif

  // set up network
  #ifdef TX
    #ifdef ETHERNET
    #else  // WIFI
      WiFi.mode(WIFI_STA);
      WiFi.begin(SSID, PWD);
      Serial.print("connecting wifi ");
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(250);
      }
    Serial.printf("\nWiFi connected. ");
    Serial.print(WiFi.SSID());
    Serial.print(" @ ");
    Serial.println(WiFi.localIP());
    #endif
  #endif

  // set up ArtNet
  #ifdef TX
    artnet.begin();
    artnet.subscribe(universe0, artnet_u1_callback);
    Serial.printf("listening to universe %d\n", universe0);
  #endif

  // set up dmx
  const dmx_config_t config = DMX_DEFAULT_CONFIG;
  dmx_param_config(dmx_num, &config);
  const int tx_io_num = 17, rx_io_num = 16, rts_io_num = 21;
  dmx_set_pin(dmx_num, tx_io_num, rx_io_num, rts_io_num);
  dmx_driver_install(dmx_num, DMX_MAX_PACKET_SIZE, 10, &dmx_queue, ESP_INTR_FLAG_IRAM);
  
  #ifdef TX
    dmx_set_mode(dmx_num, DMX_MODE_WRITE);
  #else
    dmx_set_mode(dmx_num, DMX_MODE_READ);
  #endif

  // set up LEDs
  for (int i=i; i<N_CHANNELS; i++){
    pinMode(LED_PINS[i], OUTPUT);
  }

  // configure time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("set time to: ");
  printLocalTime();

  // set up LCD
  #ifdef LCD
    #ifdef TX
      tft.init();
      tft.setRotation(1);

      // all elements on screen
      tft.fillScreen(TFT_BG_COLOR);
      tft_draw_wifi_bubble(false);
      tft_draw_ip_bubble();
      tft_draw_ethernet_bubble(false);
      tft_horizontal_sep1();
      tft_draw_url_bubble();
      tft_horizontal_sep2();

      //tft.setTextColor(TFT_WHITE);
      /*
      tft.drawPixel(0,   0,   TFT_RED);
      tft.drawPixel(239, 0,   TFT_GREEN);
      tft.drawPixel(239, 239, TFT_BLUE);
      tft.drawPixel(0,   239, TFT_ORANGE);
      */
      // wifi bubble
      
      //tft.fillSmoothCircle(TFT_LAYOUT_X_1, TFT_LAYOUT_Y_1, TFT_LAYOUT_RADIUS, TFT_COLOR_1, TFT_BG_COLOR);
      //tft.fillRect(TFT_LAYOUT_X_1, TFT_LAYOUT_COL_2-TFT_LAYOUT_RADIUS, TFT_LAYOUT_NARROW_BUBBLE, 2*TFT_LAYOUT_RADIUS+1, TFT_GREEN);
      //tft.fillSmoothCircle(TFT_LAYOUT_X_2, TFT_LAYOUT_Y_1, TFT_LAYOUT_RADIUS, TFT_COLOR_1, TFT_BG_COLOR);
      
      
      
      /*
      tft.fillRect(10, 20, 30, 60, TFT_GREEN);
      tft.fillSmoothCircle(30,  30,  TFT_LAYOUT_RADIUS, TFT_RED, TFT_BG_COLOR);
      tft.fillSmoothCircle(209, 30,  TFT_LAYOUT_RADIUS, TFT_GREEN, TFT_BG_COLOR);
      tft.fillSmoothCircle(209, 209, TFT_LAYOUT_RADIUS, TFT_BLUE, TFT_BG_COLOR);
      tft.fillSmoothCircle(30,  209, TFT_LAYOUT_RADIUS, TFT_ORANGE, TFT_BG_COLOR);
      */
    #endif
  #endif

  // services
  xTaskCreatePinnedToCore (
    dmx_status_print,     // Function to implement the task
    "dmx status print",   // Name of the task
    1750,                 // Stack size in bytes
    NULL,                 // Task input parameter
    0,                    // Priority of the task
    NULL,                 // Task handle.
    1                     // Core where the task should run
  );

  // done
  Serial.print("Setup done.\n\n");

}

//////////////////////////////////////////
///               LOOPS                ///
//////////////////////////////////////////

void loop() {
  #ifdef TX
    // recieve ArtNet
    artnet.parse();
    
    // send data on DMX
    if(dmx_available || ((millis()-last_dmx_send) > DMX_SEND_INTERVAL) ){
      dmx_available = false;

      dmx_write_packet(dmx_num, dmx_buffer, DMX_MAX_PACKET_SIZE);
      dmx_send_packet(dmx_num, DMX_MAX_PACKET_SIZE);
      dmx_wait_send_done(dmx_num, DMX_PACKET_TIMEOUT_TICK);
    }

    // display data on LEDs
    for(int i=0; i<N_CHANNELS; i++){
      analogWrite(LED_PINS[i], dmx_buffer[i+DMX_CHANNEL_OFFSET]);
    }

  #else
    if (xQueueReceive(dmx_queue, &event, DMX_PACKET_TIMEOUT_TICK)) {
      // read the packet from the driver buffer into 'data'
      dmx_read_packet(dmx_num, dmx_buffer, DMX_MAX_PACKET_SIZE);
      Serial.printf("RX: [");
      for (int i=0; i<N_CHANNELS; i++){
        if(dmx_buffer[i+DMX_CHANNEL_OFFSET]!=current_value[i]){
          current_value[i] = dmx_buffer[i+DMX_CHANNEL_OFFSET];
          analogWrite(LED_PINS[i], current_value[i]);
          Serial.printf("%d, ", current_value[i]);
        }
      }
      Serial.println("]");
    }
  #endif //ifdef TX

}

//////////////////////////////////////////

void dmx_status_print(void * pvParameters){
  while(true){
    #ifdef TX
      // print dmx data
      if (millis()-last_dmx_print > DMX_PRINT_INTERVAL){
        last_dmx_print = millis();
        print_dmx(dmx_buffer);
      }
    #endif //ifdef TX
  }
}

//////////////////////////////////////////
///             FUNCTIONS              ///
//////////////////////////////////////////

void artnet_u1_callback(const uint8_t* data, const uint16_t size) {
    // copy data to buffer
    for(int i=0; i<size; i++){
      dmx_buffer[i+1] = data[i];
    }
    // set signal
    dmx_available = true;
}

//////////////////////////////////////////

void print_dmx(byte* dmx_buffer){
  int cols = 16;
  int rows = 32;

  // print header
  printLocalTime();

  // print seperator
  Serial.print("+---------++");
  for(int i=0; i<cols; i++){
    Serial.print("-----+");
  }
  Serial.println("");

  // print content
  for (int i=0; i<rows; i++){
    // print first col
    int start = i*cols+1;
    int stop = (i+1)*cols;
    Serial.printf("| %03d-%03d ||", start, stop);
    for (int j=0; j<cols; j++){
      Serial.printf(" %03d |", dmx_buffer[start+j]);
    }
    Serial.print("\n");
  }

  // print seperator
  Serial.print("+---------++");
  for(int i=0; i<cols; i++){
    Serial.print("-----+");
  }
  Serial.print("\n\n");
}

//////////////////////////////////////////

void printLocalTime(bool newline){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.print(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  if(newline){
    Serial.println("");
  }
}

//////////////////////////////////////////

#ifdef TX
#ifdef LCD

void tft_bubble_H1_solid(int32_t x, int32_t y, int32_t width, int32_t height, int32_t color, int32_t bg_color){
  /*
  Draws a bubble onto the screen. 
  The overall width is taken from the param width
  The radius of the sides is half the height.
  x and y is the upper left corner 
  */
  int32_t radius = (int)(height/2);
  int32_t box_width = width - height;
  /*
  if (radius % 2 == 0){
   height++;
  }
  */
  height++;
  tft.fillSmoothCircle(x+radius,           y+radius, radius, color, bg_color);
  tft.fillSmoothCircle(x+radius+box_width, y+radius, radius, color, bg_color);
  tft.fillRect(x+radius, y, box_width, height, color);
}

void tft_bubble_H1_hollow(int32_t x, int32_t y, int32_t width, int32_t height, int32_t border, int32_t color, int32_t bg_color){
  tft_bubble_H1_solid(x,        y,        width,            height,            color,    bg_color);
  tft_bubble_H1_solid(x+border, y+border, width-(2*border), height-(2*border), bg_color, color);
}

//////////////////////////////////////////

void tft_horizontal_sep(int32_t y, int32_t color){
  tft.fillRect(TFT_LAYOUT_MARGIN_LEFT, y, TFT_LAYOUT_SEP_W, TFT_LAYOUT_SEP_H, TFT_COLOR_2);
}

//////////////////////////////////////////

void tft_draw_wifi_bubble(bool hollow){
  int32_t x = TFT_LAYOUT_MARGIN_LEFT;
  int32_t y = TFT_LAYOUT_MARGIN_TOP;
  int32_t width = TFT_LAYOUT_BUBBLE_W1;
  int32_t height = TFT_LAYOUT_BUBBLE_H1;
  int32_t border = TFT_LAYOUT_BORDER_THICKNESS;
  int32_t color = TFT_COLOR_1;
  int32_t bg_color = TFT_BG_COLOR;
  if (hollow){
    tft_bubble_H1_hollow(x, y, width, height, border, color, bg_color);
  } else{
    tft_bubble_H1_solid(x, y, width, height, color, bg_color);
  }
}

//////////////////////////////////////////

void tft_draw_ip_bubble(){
  int32_t x = TFT_LAYOUT_CENTER+TFT_LAYOUT_MARGIN_LEFT;
  int32_t y = TFT_LAYOUT_MARGIN_TOP;
  int32_t width = TFT_LAYOUT_BUBBLE_W1;
  int32_t height = TFT_LAYOUT_BUBBLE_H1;
  int32_t border = TFT_LAYOUT_BORDER_THICKNESS;
  int32_t color = TFT_COLOR_1;
  int32_t bg_color = TFT_BG_COLOR;
  tft_bubble_H1_solid(x, y, width, height, color, bg_color);
}

//////////////////////////////////////////

void tft_draw_ethernet_bubble(bool hollow){
  int32_t x = TFT_LAYOUT_MARGIN_LEFT;
  int32_t y = TFT_LAYOUT_MARGIN_TOP+TFT_LAYOUT_BUBBLE_H1+TFT_LAYOUT_MARGIN_IN1;
  int32_t width = TFT_LAYOUT_BUBBLE_W1;
  int32_t height = TFT_LAYOUT_BUBBLE_H1;
  int32_t border = TFT_LAYOUT_BORDER_THICKNESS;
  int32_t color = TFT_COLOR_1;
  int32_t bg_color = TFT_BG_COLOR;

  if (hollow){
    tft_bubble_H1_hollow(x, y, width, height, border, color, bg_color);
  } else{
    tft_bubble_H1_solid(x, y, width, height, color, bg_color);
  }
}

//////////////////////////////////////////

void tft_draw_url_bubble(){
  int32_t x = TFT_LAYOUT_MARGIN_LEFT;
  int32_t y = TFT_LAYOUT_MARGIN_TOP + 2*TFT_LAYOUT_BUBBLE_H1 + TFT_LAYOUT_MARGIN_IN1 + 2*TFT_LAYOUT_MARGIN_IN2 + TFT_LAYOUT_SEP_H;
  int32_t width = TFT_LAYOUT_BUBBLE_W2;
  int32_t height = TFT_LAYOUT_BUBBLE_H1;
  int32_t color = TFT_COLOR_1;
  int32_t bg_color = TFT_BG_COLOR;
  tft_bubble_H1_solid(x, y, width, height, color, bg_color);
}

//////////////////////////////////////////

void tft_horizontal_sep1(){
  int32_t y = TFT_LAYOUT_MARGIN_TOP + 2*TFT_LAYOUT_BUBBLE_H1 + TFT_LAYOUT_MARGIN_IN1 + TFT_LAYOUT_MARGIN_IN2;
  int32_t color = TFT_COLOR_2;
  tft_horizontal_sep(y, color);
}

//////////////////////////////////////////

void tft_horizontal_sep2(){
  int32_t y = TFT_LAYOUT_MARGIN_TOP + 3*TFT_LAYOUT_BUBBLE_H1 + TFT_LAYOUT_MARGIN_IN1 + 3*TFT_LAYOUT_MARGIN_IN2 + TFT_LAYOUT_SEP_H;
  int32_t color = TFT_COLOR_2;
  tft_horizontal_sep(y, color);
}

#endif //LCD
#endif //TX