#include <Arduino.h>
void artnet_u1_callback(const uint8_t* data, const uint16_t size);
void print_dmx(byte* dmx_buffer);

// LED settings
byte LED_PINS[] = {22, 19, 23}; // pins at MCU where LEDs are connected
#define DMX_CHANNEL_OFFSET 1    // DMX base address for LEDs
#define N_CHANNELS 3            // number of channels occupied by LEDs


/*
** TX: The one which recieves ArtNet and sends it on DMX. The TX shows the data on its attached LEDs
** RX: The one recieving DMX and lightning up the LEDs
*/
#define TX

/*
** Switch for ethernet-based or WiFi-based network
*/
//#define ETHERNET

#include "esp_dmx.h"

#ifdef TX
  #ifdef ETHERNET
    #include <ArtnetEther.h>
  #else  // WiFi
    const char* SSID = "FREDDY-L390 6365";//"Jubel";//"FREDDY-L390 6365";
    const char* PWD = "0936u)1E";//"69792805351678254921";//"0936u)1E";
    
    #include <ArtnetWiFi.h>
    ArtnetWiFiReceiver artnet;
    uint8_t universe1 = 0;
  #endif

  // debug
  int last_dmx_print = 0;
#else
  dmx_event_t event;
  byte current_value[N_CHANNELS] = {0};
#endif

byte dmx_buffer[DMX_MAX_PACKET_SIZE] = {0};
const dmx_port_t dmx_num = DMX_NUM_2;
QueueHandle_t dmx_queue;

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
    Serial.print("WiFi connected, IP = ");
    Serial.println(WiFi.localIP());
    #endif
  #endif

  // set up ArtNet
  #ifdef TX
    artnet.begin();
    artnet.subscribe(universe1, artnet_u1_callback);
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

  // done
  Serial.print("Setup done.\n");

}

void loop() {
  #ifdef TX
    // recieve ArtNet
    artnet.parse();
    
    // send data on DMX
    dmx_write_packet(dmx_num, dmx_buffer, DMX_MAX_PACKET_SIZE);
    dmx_send_packet(dmx_num, DMX_MAX_PACKET_SIZE);
    dmx_wait_send_done(dmx_num, DMX_PACKET_TIMEOUT_TICK);

    // display data on LEDs
    for(int i=0; i<N_CHANNELS; i++){
      analogWrite(LED_PINS[i], dmx_buffer[i+DMX_CHANNEL_OFFSET]);
    }
  
    // print dmx data
    if (millis()-last_dmx_print > 10000){
      last_dmx_print = millis();
      print_dmx(dmx_buffer);
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


void artnet_u1_callback(const uint8_t* data, const uint16_t size) {
    // copy data to buffer
    for(int i=0; i<size; i++){
      dmx_buffer[i+1] = data[i];
    }
}

void print_dmx(byte* dmx_buffer){
  int cols = 16;
  int rows = 32;

  // TODO print header

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
  Serial.println("");
}
