#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include "IPAddress.h"


#define SSID        "hongphuong"
#define PASSWORD    "abcde123"
#define LIGHT_PIN   16

#define HOST  "192.168.137.1"
#define PORT        5683

IPAddress local_IP(192,168,137,100);
IPAddress gateway(192,168,137, 1);
IPAddress subnet(255,255,255,1);
IPAddress primaryDNS(8,8,8,8);
IPAddress secondaryDNS(4,4,4,4);

WiFiUDP udp;
Coap coap(udp);

bool Light_flag = false;
static int light_dim = 0;

void coap_response_cb(CoapPacket &packet, IPAddress ip, int port){
    Serial.println("[Coap Response got]");
  
    char p[packet.payloadlen + 1];
    memcpy(p, packet.payload, packet.payloadlen);
    p[packet.payloadlen] = NULL;
    
    Serial.println(p);
}

void callback_ligth_server(CoapPacket &packet, IPAddress ip, int port){
    Serial.println("Light Control!");

    char p[packet.payloadlen+1];
    memcpy(p, packet.payload, packet.payloadlen);
    p[packet.payloadlen] = '\0';

    String message(p);

    light_dim = message.toInt();
    Light_flag = true;
    coap.sendResponse(ip, port, packet.messageid, "OK");
}

void light_control( void *args){
    while(1){
        if(Light_flag){
            Serial.print("Controlling light to dim (%): ");
            Serial.println(light_dim);
            analogWrite(LIGHT_PIN, (int)light_dim *256/100);
            Light_flag = false;
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}


void setup(){
    Serial.begin(115200);

    WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
    WiFi.begin(SSID, PASSWORD);
    while(WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    pinMode(LIGHT_PIN, OUTPUT);
    analogWrite(LIGHT_PIN, 0);
    // Light_state = false;
    coap.server(callback_ligth_server, "light");
    coap.response(coap_response_cb);
    coap.start(PORT);
       
    xTaskCreatePinnedToCore(light_control, "light control task", 2048, NULL, 2, NULL, 1);
}

void loop(){
    vTaskDelay(100/portTICK_PERIOD_MS);
    coap.loop();
}