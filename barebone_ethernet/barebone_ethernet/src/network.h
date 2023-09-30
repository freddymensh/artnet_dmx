#include <Arduino.h>

class WifiPw{
    public:
        WifiPw(String SSID, String password);
        ~WifiPw();
        String password;
        String SSID;
        
        bool operator==(WifiPw);
};

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
        WifiNetworks(WifiPw* networks, int16_t n);
        ~WifiNetworks();
        
        int16_t n;

        WifiPw operator[](int idx);
        
        bool is_known(String SSID);
        void print_known_ssid();
        String SSID(int idx);
        WifiPw get_network(String SSID);

    protected:
        WifiPw* networks;  
};


WifiNetworks::WifiNetworks(WifiPw* networks, int16_t n){
    this->networks = networks;
    this->n = n; //(int16_t) (sizeof(this->networks) / sizeof(WifiPw));
}


WifiNetworks::~WifiNetworks(){};


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
