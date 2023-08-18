#include "main.h"



//////////////////////////////////////////
///               SETUP                ///
//////////////////////////////////////////

void setup() {
  // set up serial port
  Serial.begin(115200);

  // print status string
  Serial.print("ESP32_ArtNet_TxRx and DMX-");
  #ifdef TRANSMIT
    Serial.print("transmit DMX\n");
  #else
    Serial.print("recieve DMX\n");
  #endif

  // set up LCD
  #ifdef LCD
    #ifdef TRANSMIT
      tft.init();
      tft.setRotation(1);

      // initialize all elements on screen
      tft.init_screen();
      
    #endif
  #endif

  // set up network
  #ifdef ETHERNET
  #else  // WIFI
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
  #endif

  // set up ArtNet
  #ifdef TRANSMIT
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
  
  #ifdef TRANSMIT
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


  // services
  xTaskCreatePinnedToCore (
    service_dmx_status_print,     // Function to implement the task
    "dmx status print",   // Name of the task
    1750,                 // Stack size in bytes
    NULL,                 // Task input parameter
    0,                    // Priority of the task
    NULL,                 // Task handle.
    1                     // Core where the task should run
  );

  #ifdef TRANSMIT
  #ifdef LCD
    xTaskCreatePinnedToCore (
      service_tft_update_screen,     // Function to implement the task
      "tft screen update",   // Name of the task
      1750,                 // Stack size in bytes
      NULL,                 // Task input parameter
      0,                    // Priority of the task
      NULL,                 // Task handle.
      1                     // Core where the task should run
    );
  #endif //LCD
  #endif //TRANSMIT

  // done
  Serial.print("Setup done.\n\n");

}

//////////////////////////////////////////
///               LOOPS                ///
//////////////////////////////////////////

void loop() {
  #ifdef TRANSMIT
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

    // reconnect wifi
    #ifndef ETHERNET
      if (WiFi.status() != WL_CONNECTED){
        Serial.println("try reconnecting wifi");
        //WiFi.begin(SSID, PWD);
        WiFi.reconnect();
        int reconnect_counter = 0;
        while ((WiFi.status() != WL_CONNECTED) and (reconnect_counter < WIFI_RECONNECT_TRIALS)) {
          Serial.print(".");
          reconnect_counter++;
        delay(250);
        }
        if (WiFi.status() == WL_CONNECTED){
          print_wifi_status();
        } else {
          Serial.println("Reconnect failed. Rebooting...");
          ESP.restart();
        }
      }
    #endif

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
  #endif //ifdef TRANSMIT

}

//////////////////////////////////////////

void service_dmx_status_print(void * pvParameters){
  while(true){
    #ifdef TRANSMIT
      // print dmx data
      if (millis()-last_dmx_print > DMX_PRINT_INTERVAL){
        last_dmx_print = millis();
        print_dmx(dmx_buffer);
      }
    #endif //ifdef TRANSMIT
  }
}


#ifdef LCD
void service_tft_update_screen(void * pvParameters){
  #ifdef TRANSMIT
  while(true){
      // print dmx data
      if (millis()-last_tft_update > TFT_UPDATE_INTERVAL){
        last_tft_update = millis();
        tft_update_screen();
      }
  }
  #else // ifdef TRANSMIT
  #endif // ifdef TRANSMIT
}
#endif // ifdef LCD

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

#ifndef ETHERNET

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

#endif // ETHERNET

//////////////////////////////////////////

#ifdef TRANSMIT
#ifdef LCD

void tft_update_screen(){
  #ifndef ETHERNET // wifi only
    if ( (WiFi.status() == WL_CONNECTED) && (tft_status_wifi == disconnected)){ // wifi connected but disconnected shown on screen
      tft_status_wifi = connected;
      tft.draw_wifi_bubble(false, WiFi.SSID());
      tft.draw_ip_bubble(WiFi.localIP().toString());
      Serial.println("wifi bubble updated to CONNECTED status");
    } else if((WiFi.status() != WL_CONNECTED) && (tft_status_wifi == connected)){
      tft_status_wifi = disconnected;
      tft.draw_wifi_bubble(true, "not connected");
      tft.draw_ip_bubble();
      Serial.println("wifi bubble updated to DISCONNECTED status");
    } else {
      // do nothing
    }
  #else
  #endif //ETHERNET
}


#endif //ifdef LCD
#endif //ifdef TRANSMIT

