#include "PubSubClient.h"
#include <ESP8266WiFi.h>

class Mqtt{


    // void callback(char *topic, uint8_t *payload, unsigned int length);
    void reconnect();    
    
    public:
        Mqtt();
        void Update();
};
