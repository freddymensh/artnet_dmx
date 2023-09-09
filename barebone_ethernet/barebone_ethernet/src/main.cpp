#include <Arduino.h>
#include "esp_dmx.h"
//#include <ArtnetETH.h>
#include <ArtnetWiFi.h>

// functions
void service_dmx_status_print(void * pvParameters);
void print_dmx(byte* dmx_buffer);
void printLocalTime(bool newline);
void printLocalTime(bool newline=true);
void artnet_u1_callback(const uint8_t* data, const uint16_t size);
void print_wifi_status();

// network
#include <WiFi.h>
#define WIFI_RECONNECT_TRIALS 50
const char* SSID = "FREDDY-L390 6365";//"Jubel";//"FREDDY-L390 6365";
const char* PWD = "0936u)1E";//"69792805351678254921";//"0936u)1E";    

// time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// dmx sender
byte dmx_buffer[DMX_MAX_PACKET_SIZE] = {0};
const dmx_port_t dmx_num = DMX_NUM_2;
QueueHandle_t dmx_queue;
bool dmx_available = false;
#define DMX_SEND_INTERVAL 1000
int last_dmx_send = 0;

// dmx debug
#define DMX_PRINT_INTERVAL 10000
int last_dmx_print = 0;

// artnet
uint8_t universe0 = 0;
//ArtnetReceiver artnet;
ArtnetWiFi artnet;

//////////////////////////////////////////
///               SETUP                ///
//////////////////////////////////////////

void setup() {
  Serial.begin(250000); //115200

  // set up network
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PWD);
  int wifi_connect_counter = 0;
  while ((WiFi.status() != WL_CONNECTED) and (wifi_connect_counter < WIFI_RECONNECT_TRIALS)) {
    Serial.print(".");
    wifi_connect_counter++;
  delay(250);
  }
  if (WiFi.status() == WL_CONNECTED){
    print_wifi_status();
  } else {
    Serial.println("Wifi connection failed. Rebooting...");
    ESP.restart();
  }
  
  // configure time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("set time to: ");
  printLocalTime();

  // set up dmx
  const dmx_config_t config = DMX_DEFAULT_CONFIG;
  dmx_param_config(dmx_num, &config);
  const int tx_io_num = 32, rx_io_num = 33, rts_io_num = 16;
  dmx_set_pin(dmx_num, tx_io_num, rx_io_num, rts_io_num);
  dmx_driver_install(dmx_num, DMX_MAX_PACKET_SIZE, 10, &dmx_queue, ESP_INTR_FLAG_IRAM);
  dmx_set_mode(dmx_num, DMX_MODE_WRITE);

  // set up ArtNet
  WiFi.begin();
  artnet.begin();
  artnet.subscribe(universe0, artnet_u1_callback);
  Serial.printf("listening to universe %d\n", universe0);
  

  // services
  /*
  xTaskCreatePinnedToCore (
    service_dmx_status_print,     // Function to implement the task
    "dmx status print",   // Name of the task
    1750,                 // Stack size in bytes
    NULL,                 // Task input parameter
    0,                    // Priority of the task
    NULL,                 // Task handle.
    1                     // Core where the task should run
  );
  */

}

void loop() {
  // send data on DMX
  
  if(dmx_available || ((millis()-last_dmx_send) > DMX_SEND_INTERVAL) ){
    dmx_available = false;

    dmx_write_packet(dmx_num, dmx_buffer, DMX_MAX_PACKET_SIZE);
    dmx_send_packet(dmx_num, DMX_MAX_PACKET_SIZE);
    dmx_wait_send_done(dmx_num, DMX_PACKET_TIMEOUT_TICK);
  }
  

  // recieve ArtNet
  artnet.parse();

  // print DMX data
  if (millis()-last_dmx_print > DMX_PRINT_INTERVAL){
      last_dmx_print = millis();
      print_dmx(dmx_buffer);
    }

}


//////////////////////////////////////////

void service_dmx_status_print(void * pvParameters){
  while(true){
    // print dmx data
    if (millis()-last_dmx_print > DMX_PRINT_INTERVAL){
      last_dmx_print = millis();
      print_dmx(dmx_buffer);
    }
  }
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

void artnet_u1_callback(const uint8_t* data, const uint16_t size) {
    // copy data to buffer
    for(int i=0; i<size; i++){
      dmx_buffer[i+1] = data[i];
    }
    // set signal
    dmx_available = true;
}

//////////////////////////////////////////

void print_wifi_status(){
  if (WiFi.status() == WL_CONNECTED){
    Serial.printf("\nWiFi connected. ");
    Serial.print(WiFi.SSID());
    Serial.print(" @ ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi not connected.");
  }
}

