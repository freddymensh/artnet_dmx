#include <Arduino.h>

byte linear_interpolation_byte(float x, float x0, float x1, float y0, float y1);

byte LED_PINS[] = {22, 19, 23};
//#define BUFFER_LEN 512
#define DMX_CHANNEL_OFFSET 1
#define N_CHANNELS 3

//#define TX
#define smooth_TX

#include "esp_dmx.h"

#ifdef TX
  #define INTERVAL_REGENERATE_COLOR 2000 // milli seconds
  long time_regenerate_color = 0;
  byte base_color[N_CHANNELS];
  byte target_color[N_CHANNELS];
  byte current_color[N_CHANNELS];
#else
  dmx_event_t event;
  byte current_value[N_CHANNELS] = {0};
#endif

byte dmx_buffer[DMX_MAX_PACKET_SIZE] = {0};
const dmx_port_t dmx_num = DMX_NUM_2;
QueueHandle_t dmx_queue;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("Arduino_ESP32_TxRx and DMX-");
  #ifdef TX
    Serial.print("TX\n");
  #else
    Serial.print("RX\n");
  #endif

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

  for (int i=i; i<N_CHANNELS; i++){
    pinMode(LED_PINS[i], OUTPUT);
  }
  Serial.print("Setup done.\n");

}

void loop() {
  // put your main code here, to run repeatedly:
  #ifdef TX
    if ((millis()-time_regenerate_color) > INTERVAL_REGENERATE_COLOR){
      time_regenerate_color = millis();
      // copy old target color to base color
      Serial.print("\nbase_color   = [");
      for (int i=0; i<N_CHANNELS; i++){
        base_color[i] = target_color[i];
        Serial.printf("%d, ", base_color[i]);
      }
      Serial.print("]\n");

      // sample new target color
      Serial.print("target_color = [");
      for (int i=0; i<N_CHANNELS; i++){
        target_color[i] = random(0, 255);
        Serial.printf("%d, ", target_color[i]);
      }
      Serial.print("]\n");
    }

    // calulate current color
    //Serial.print("current_color = [");
    for (int i=0; i<N_CHANNELS; i++){
      //Serial.printf("time_regenerate_color = %d\n", time_regenerate_color);
      float relative_time = (float)(millis()-time_regenerate_color) / INTERVAL_REGENERATE_COLOR;
      //Serial.printf("relative_time = %f\n", relative_time);
      #ifdef smooth_TX
        current_color[i] = linear_interpolation_byte(relative_time, 0, 1, base_color[i], target_color[i]);
      #else
        current_color[i] = target_color[i];
      #endif
      //Serial.printf("%d, ", current_color[i]);
    }
    //Serial.print("]\n");
    //delay(1000);

    // copy color to dmx buffer
    for(int i=0; i<N_CHANNELS; i++){
      dmx_buffer[i+DMX_CHANNEL_OFFSET] = current_color[i];
    }

    dmx_write_packet(dmx_num, dmx_buffer, DMX_MAX_PACKET_SIZE);
    dmx_send_packet(dmx_num, DMX_MAX_PACKET_SIZE);
    dmx_wait_send_done(dmx_num, DMX_PACKET_TIMEOUT_TICK);
    for(int i=0; i<N_CHANNELS; i++){
      analogWrite(LED_PINS[i], dmx_buffer[i+DMX_CHANNEL_OFFSET]);
    }
    //delay(1000);
    //Serial.printf("TX:%d\n", dmx_buffer[0+DMX_CHANNEL_OFFSET]);


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


byte linear_interpolation_byte(float x, float x0, float x1, float y0, float y1){
  float y = (y1-y0)/(x1-x0) * (x-x0) + y0;
  byte ret;
  if ( y > 255 ){
    //Serial.println("upper limit catch");
    ret = 255;
  } else if ( y < 0 ){
    //Serial.println("lower limit catch");
    ret = 0;
  } else {
    ret = (byte) y;
  }
  return(ret);
}

