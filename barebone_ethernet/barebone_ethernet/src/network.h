#include <Arduino.h>
#include <vector>

class WifiPw{
    public:
        WifiPw();
        WifiPw(String SSID, String password);
        ~WifiPw();
        String password;
        String SSID;
        
        bool operator==(WifiPw);
};

WifiPw::WifiPw(){
    this->SSID = "";
    this->password = "";
}

WifiPw::WifiPw(String SSID, String password){
    this->SSID = SSID;
    this->password = password;
}


WifiPw::~WifiPw(){};


bool WifiPw::operator==(WifiPw other){
    return (this->SSID==other.SSID) && (this->password==other.password);
}

//////////////////////////////////

class WifiNetworks{
    public:
        WifiNetworks();
        WifiNetworks(WifiPw* networks, int16_t n);
        ~WifiNetworks();
        
        int16_t n;

        WifiPw operator[](int idx);
        
        bool is_known(String SSID);
        void print_known_ssid();
        String SSID(int idx);
        WifiPw get_network(String SSID);
        void set(WifiPw* networks, int16_t n);
        //void WifiNetworks::set(vector<WifiPw> networks);

    protected:
        WifiPw* networks;  
};

WifiNetworks::WifiNetworks(){
}

WifiNetworks::WifiNetworks(WifiPw* networks, int16_t n){
    this->set(networks, n);
}


WifiNetworks::~WifiNetworks(){
    free(this->networks);
};


bool WifiNetworks::is_known(String SSID){
    for(int i=0; i<this->n; i++){
        if (this->networks[i].SSID == SSID){
            return true;
        }
    }
    return false;
}


WifiPw WifiNetworks::operator[](int idx){
    /*
    if (idx >= this->n){
        return WifiPw("", "");
    } else{
        return this->networks[(int) idx];
    }
    */
   idx %= this->n;
   return this->networks[idx];
}


void WifiNetworks::print_known_ssid(){
    for (int i=0; i<this->n; i++){
        Serial.print(this->operator[](i).SSID);
        if (i < this->n-1){
            Serial.print(", ");
        }
    }
    Serial.println("");
}


String WifiNetworks::SSID(int idx){
    return this->networks[idx].SSID;
}


WifiPw WifiNetworks::get_network(String SSID){
    for(int i=0; i<this->n; i++){
        if (this->operator[](i).SSID==SSID){
            return this->operator[](i);
        }
    }
    return WifiPw("", "");
}


void WifiNetworks::set(WifiPw* networks, int16_t n){
  this->networks = new WifiPw[n];
  this->n = n; //(int16_t) (sizeof(this->networks) / sizeof(WifiPw));
  for (int i = 0; i < n; i++) {
      this->networks[i] = networks[i];
  }
}

