#include "const.h"

#ifdef LCD
  #include <TFT_eSPI.h>



  //////////////////////////////////////////
  //               header                 //
  //////////////////////////////////////////


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

  #define TFT_UPDATE_INTERVAL 2000


  //////////////////////////////////////////
  #ifndef _Screen
  #define _Screen

  class Screen: public TFT_eSPI{
      public:
          using TFT_eSPI::TFT_eSPI;
          #ifdef TRANSMIT
            void bubble_H1_solid(int32_t x, int32_t y, int32_t width, int32_t height, int32_t color, int32_t bg_color);
            void bubble_H1_hollow(int32_t x, int32_t y, int32_t width, int32_t height, int32_t border, int32_t color, int32_t bg_color);
            void draw_wifi_bubble(bool hollow, String message);
            void draw_ip_bubble(String message);
            void draw_ip_bubble();
            void draw_ethernet_bubble(bool hollow);
            void draw_url_bubble();
            void horizontal_sep(int32_t y, int32_t color);
            void horizontal_sep1();
            void horizontal_sep2();
            void init_screen();
            void update_screen();
          #endif // ifdef TRANSMIT
          


  };
  #endif // _Screen

#endif  //idef LCD



//////////////////////////////////////////
//            implementaion             //
//////////////////////////////////////////
#ifdef LCD
  
  #ifdef TRANSMIT

  void Screen::bubble_H1_solid(int32_t x, int32_t y, int32_t width, int32_t height, int32_t color, int32_t bg_color){
    /*
    Draws a solid bubble onto the screen. 
    The overall width is taken from the param width
    The radius of the sides is half the height. 
    x and y is the upper left corner
    */

    int32_t r = (int)(height/2);      // radius
    this->fillSmoothRoundRect(x, y, width, height, r, color, bg_color);
  }

  //////////////////////////////////////////

  void Screen::bubble_H1_hollow(int32_t x, int32_t y, int32_t width, int32_t height, int32_t border, int32_t color, int32_t bg_color){
    /*
    Draws a hollow bubble onto the screen. 
    The overall width is taken from the param width
    The outer radius of the sides is half the height.
    The inner radius is callulated from the param border. The param describes the width of the border.
    x and y is the upper left corner
    */
    int32_t r = (int)(height/2);                 // outer radius
    int32_t ir = (int)(height/2) - border + 1;   // inner radius
    this->drawSmoothRoundRect(x, y, r, ir, width, height, color, bg_color);
    
  }

  //////////////////////////////////////////

  void Screen::horizontal_sep(int32_t y, int32_t color){
    this->fillRect(TFT_LAYOUT_MARGIN_LEFT, y, TFT_LAYOUT_SEP_W, TFT_LAYOUT_SEP_H, TFT_COLOR_2);
  }

  //////////////////////////////////////////

  void Screen::draw_wifi_bubble(bool hollow, String message){
    int32_t x = TFT_LAYOUT_MARGIN_LEFT;
    int32_t y = TFT_LAYOUT_MARGIN_TOP;
    int32_t width = TFT_LAYOUT_BUBBLE_W1;
    int32_t height = TFT_LAYOUT_BUBBLE_H1;
    int32_t border = TFT_LAYOUT_BORDER_THICKNESS;
    int32_t color = TFT_COLOR_1;
    int32_t bg_color = TFT_BG_COLOR;

    // delete bubble
    this->fillRect(x, y, width+1, height+1, bg_color);
    
    // draw new bubble
    if (hollow){
      this->bubble_H1_hollow(x, y, width, height, border, color, bg_color);
      this->setTextColor(color, bg_color);
    } else{
      this->bubble_H1_solid(x, y, width, height, color, bg_color);
      this->setTextColor(bg_color, color);
    }

    //message
    int32_t max_text_width = width - height;
    // TODO: shorten SSID if too long 
    this->setTextDatum(MC_DATUM);
    this->drawString(message, x+(int)(width/2), y+(int)(height/2));

  }

  //////////////////////////////////////////

  void Screen::draw_ip_bubble(){
    String message = "---.---.---.---";
    this->draw_ip_bubble(message);
  }

  void Screen::draw_ip_bubble(String message){
    int32_t x = TFT_LAYOUT_CENTER+TFT_LAYOUT_MARGIN_LEFT;
    int32_t y = TFT_LAYOUT_MARGIN_TOP;
    int32_t width = TFT_LAYOUT_BUBBLE_W1;
    int32_t height = TFT_LAYOUT_BUBBLE_H1;
    int32_t border = TFT_LAYOUT_BORDER_THICKNESS;
    int32_t color = TFT_COLOR_1;
    int32_t bg_color = TFT_BG_COLOR;
    int32_t r = (int)(height/2);
    
    // delete old bubble
    this->fillRect(x, y, width+1, height+1, bg_color);

    // draw new bubble
    this->bubble_H1_solid(x, y, width, height, color, bg_color);

    // print message
    this->setTextDatum(MC_DATUM);
    this->setTextColor(bg_color, color);
    this->drawString(message, x+(int)(width/2), y+(int)(height/2));

  }

  //////////////////////////////////////////

  void Screen::draw_ethernet_bubble(bool hollow){
    int32_t x = TFT_LAYOUT_MARGIN_LEFT;
    int32_t y = TFT_LAYOUT_MARGIN_TOP+TFT_LAYOUT_BUBBLE_H1+TFT_LAYOUT_MARGIN_IN1;
    int32_t width = TFT_LAYOUT_BUBBLE_W1;
    int32_t height = TFT_LAYOUT_BUBBLE_H1;
    int32_t border = TFT_LAYOUT_BORDER_THICKNESS;
    int32_t color = TFT_COLOR_1;
    int32_t bg_color = TFT_BG_COLOR;
    int32_t r = (int)(height/2);

    if (hollow){
      this->bubble_H1_hollow(x, y, width, height, border, color, bg_color);
    } else{
      this->bubble_H1_solid(x, y, width, height, color, bg_color);
    }
  }

  //////////////////////////////////////////

  void Screen::draw_url_bubble(){
    int32_t x = TFT_LAYOUT_MARGIN_LEFT;
    int32_t y = TFT_LAYOUT_MARGIN_TOP + 2*TFT_LAYOUT_BUBBLE_H1 + TFT_LAYOUT_MARGIN_IN1 + 2*TFT_LAYOUT_MARGIN_IN2 + TFT_LAYOUT_SEP_H;
    int32_t width = TFT_LAYOUT_BUBBLE_W2;
    int32_t height = TFT_LAYOUT_BUBBLE_H1;
    int32_t color = TFT_COLOR_1;
    int32_t bg_color = TFT_BG_COLOR;
    int32_t r = (int)(height/2);

    this->bubble_H1_solid(x, y, width, height, color, bg_color);
  }

  //////////////////////////////////////////

  void Screen::horizontal_sep1(){
    int32_t y = TFT_LAYOUT_MARGIN_TOP + 2*TFT_LAYOUT_BUBBLE_H1 + TFT_LAYOUT_MARGIN_IN1 + TFT_LAYOUT_MARGIN_IN2;
    int32_t color = TFT_COLOR_2;
    this->horizontal_sep(y, color);
  }

  //////////////////////////////////////////

  void Screen::horizontal_sep2(){
    int32_t y = TFT_LAYOUT_MARGIN_TOP + 3*TFT_LAYOUT_BUBBLE_H1 + TFT_LAYOUT_MARGIN_IN1 + 3*TFT_LAYOUT_MARGIN_IN2 + TFT_LAYOUT_SEP_H;
    int32_t color = TFT_COLOR_2;
    this->horizontal_sep(y, color);
  }

  //////////////////////////////////////////

  void Screen::init_screen(){
    // init
    this->fillScreen(TFT_BG_COLOR);
    //this->setTextFont(12);

    this->draw_wifi_bubble(true, "");  
    this->draw_ip_bubble();
    this->draw_ethernet_bubble(true);
    this->horizontal_sep1();
    this->draw_url_bubble();
    this->horizontal_sep2();

  };



  #endif //ifdef TRANSMIT
#endif  //idef LCD
