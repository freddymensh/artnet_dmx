#include <Arduino.h>
#include <vector>
#include "esp_dmx.h"
//#include <ArtnetETH.h>
#include <ArtnetWiFi.h>
#include "FS.h"
#include "SD_MMC.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "network.h"

//defines
#define DEBUG

// functions
void service_dmx_status_print(void * pvParameters);
void print_dmx(byte* dmx_buffer);
void printLocalTime(bool newline);
void printLocalTime(bool newline=true);
void artnet_u1_callback(const uint8_t* data, const uint16_t size);
void print_wifi_status();
void printMacAddress();
void listNetworks(int numSsid);
void initSD();
void printConfig(fs::FS &fs, const char* filename);
void loadConfig(fs::FS &fs, const char* filename, WifiNetworks &networks);

// network
#define WIFI_RECONNECT_TRIALS 50
/* set ssids and passwords in the json on the sd. eg:
 {
  "wifi": [	
	  {"ssid": "my-first-ssid", "pw": "mY-fIrST-aWEsOme-pASswOrd"},
    {"ssid": "my-first-ssid", "pw": "thE-SecONd-paSswORd"}
  ]
}
*/
WifiNetworks wifinetw;

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

// setup-file & SD
const char* config_filename = "/config.json";

//////////////////////////////////////////
///               SETUP                ///
//////////////////////////////////////////

void setup() {
  Serial.begin(115200); //115200   250000
  // SD
  initSD();
  #ifdef DEBUG
  printConfig(SD_MMC, config_filename);
  #endif  //DEBUG
  loadConfig(SD_MMC, config_filename, wifinetw);

  // set up network
  WiFi.mode(WIFI_STA);
  int numSsid = WiFi.scanNetworks();
  #ifdef DEBUG
    Serial.printf("%d networks are known: ", wifinetw.n);
    wifinetw.print_known_ssid();
    listNetworks(numSsid);
  #endif // DEBUG

  for (int i=0; i<numSsid; i++){
    // break loop if wifi is already connected
    if (WiFi.status() == WL_CONNECTED){
      break;
    }

    // loop over available networks and try to connect to them
    if (wifinetw.is_known(WiFi.SSID(i)) ){
      WifiPw nw = wifinetw.get_network(WiFi.SSID(i));
      
      Serial.printf("Try connecting to %s", nw.SSID);
      WiFi.begin(nw.SSID, nw.password);
      int wifi_connect_counter = 0;
      while ((WiFi.status() != WL_CONNECTED) and (wifi_connect_counter < WIFI_RECONNECT_TRIALS)) {
        Serial.print(".");
        wifi_connect_counter++;
        delay(250);
      }
      if (WiFi.status() == WL_CONNECTED){
        print_wifi_status();
      } else {
        Serial.println("failed. Try next SSID.");
      }
    } else {
      continue;
    }
  }
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Wifi connection failed. Rebooting...");
    ESP.restart();
  }

  //
  
  // configure time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("set time to: ");
  printLocalTime();

  // set up dmx
  const dmx_config_t config = DMX_DEFAULT_CONFIG;
  dmx_param_config(dmx_num, &config);
  const int tx_io_num = 16, rx_io_num = 33, rts_io_num = 32;
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




/////////////////////////
void printMacAddress() {
  // the MAC address of your Wifi shield
  byte mac[6];

  // print your MAC address:
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
}

void listNetworks(int numSsid) {
  if (numSsid < 0) {
    return;
  }

  // print the list of networks seen:
  Serial.printf("Discovered %d wifi-networks.\n", numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.printf("\t known: %s", wifinetw.is_known(WiFi.SSID(thisNet)) ? "true" : "false");
    //Serial.print("\tEncryption: ");
    //printEncryptionType(WiFi.encryptionType(thisNet));
    Serial.println("");
  }
  Serial.println("");
}

/*
void printEncryptionType(int thisType) {
  // read the encryption type and print out the name:
  switch (thisType) {
    case ENC_TYPE_WEP:
      Serial.println("WEP");
      break;
    case ENC_TYPE_TKIP:
      Serial.println("WPA");
      break;
    case ENC_TYPE_CCMP:
      Serial.println("WPA2");
      break;
    case ENC_TYPE_NONE:
      Serial.println("None");
      break;
    case ENC_TYPE_AUTO:
      Serial.println("Auto");
      break;
  }
}
*/


void initSD(){
  if(!SD_MMC.begin("/sdcard", true))
  {
      Serial.println("Card Mount Failed");
      Serial.println("Note: is it formated as FAT32? (exFAT and other formats at NOT supported)");
  }
}

void printConfig(fs::FS &fs, const char* filename){
  File file = fs.open(filename);
  if (!file) {
    Serial.println("Failed to open file. Rebooting...");
    ESP.restart();   
  }
  Serial.print("\nconfig from SD\n");
  while (file.available()) {
    Serial.write(file.read()); // This prints each character as it is read
  }
  Serial.print("\n");
  file.close();
}


void loadConfig(fs::FS &fs, const char* filename, WifiNetworks &networks){
  File file = fs.open(filename);
  if (!file) {
    Serial.println("Failed to open file. Rebooting...");
    ESP.restart();   
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error){
    Serial.println(F("Failed to read file."));
    Serial.println(error.c_str());
  }

  size_t n_known_nw = doc["wifi"].size();
  Serial.println("after size before malloc");
  std::vector<WifiPw> passwords;

  for(int i=0; i<n_known_nw; i++){
    passwords.push_back(WifiPw(doc["wifi"][i]["ssid"], doc["wifi"][i]["pw"]));
  }
  networks.set(passwords.data(), n_known_nw);

  file.close();
}