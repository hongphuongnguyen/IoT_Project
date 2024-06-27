#include <PubSubClient.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <string.h>
#include <stdlib.h>

#define HVAC_TOPIC      "/actuator/HVAC"
#define TOPIC_RESPONSE  "room1/response"

#define TEMP_CONTROL    (1)
#define HUMI_CONTROL    (2)
#define AIR_CONTROL     (3)
#define WARNING_EVENT   (4)

const char* ssid = "hongphuong";
const char* password = "abcde123";

const char *mqtt_server = "192.168.137.1";

typedef struct {
    float value;
    float set_point;
}__attribute__((packed)) HVAC_element_t;

typedef struct{
    int room_id;
    HVAC_element_t temp;
    HVAC_element_t humi;
    HVAC_element_t air;
}__attribute__((packed)) HVAC_actuator_t;

typedef struct{
    int event_control;
    float set_point;
}__attribute__((packed)) HVAC_event_t;

WiFiClient espClient;
PubSubClient client(espClient);

QueueHandle_t msg_queue;

HVAC_actuator_t HVA_controller = {
    .room_id = 1,
};

HVAC_event_t msg_control = {
    .event_control = 0
};

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;
int check = 1;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

void mqtt_connection_task(void *arg){
    char topic[100]; 
    while(1){
        if(!client.connected()){
            Serial.println("Attemping  MQTT connection!");
            if(client.connect("HVAC",  TOPIC_RESPONSE, (uint8_t)1, 1, "HVAC disconnected")){
                Serial.println("MQTT connected!");
                sprintf(topic, "room%d%s\0", HVA_controller.room_id, HVAC_TOPIC);
                // client.setKeepAlive(0);
                client.subscribe(topic, 1);
                Serial.println("Subscribed to " + String(topic));
                client.publish(TOPIC_RESPONSE, "HVAC Connected", (boolean)true);
            }
        }
        else
            client.loop();
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}

void wifi_setup(){
    delay(10);
  // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char *topic, byte* message, unsigned int length){
    Serial.print("Message arrived on topic:");
    Serial.println(topic);
    char *messageTemp = (char *)malloc(sizeof(char) * 50);
    char *pchar;
    // memset(messageTemp, 0, (size_t)length);
    memcpy(messageTemp, (void*)message, length);
    messageTemp[length] = '\0';

    Serial.print("Message:");
    Serial.println((char*)messageTemp);
    pchar = strtok(messageTemp, ":\n");
    while(pchar != NULL){
        if(!strcmp(pchar, "\"temp\"")){
            msg_control.event_control = TEMP_CONTROL;
            // Serial.println(msg_control.event_control);
            pchar = strtok(NULL, ":\n");
            msg_control.set_point = atoi(pchar);
            // Serial.println(msg_control.set_point);
            xQueueSend(msg_queue,(void *) &msg_control, 0);
            continue;
        }
        else if(!strcmp(pchar, "\"humidity\"")){
            msg_control.event_control = HUMI_CONTROL;
            // Serial.println(msg_control.event_control);
            pchar = strtok(NULL, ":\n");
            msg_control.set_point = atoi(pchar);
            // Serial.println(msg_control.set_point);
            xQueueSend(msg_queue,(void *) &msg_control, 0);
            continue;
        }
        else if(!strcmp(pchar, "\"air\"")){
            msg_control.event_control = AIR_CONTROL;
            // Serial.println(msg_control.event_control);
            pchar = strtok(NULL, ":\n");
            msg_control.set_point = atoi(pchar);
            // Serial.println(msg_control.set_point);
            xQueueSend(msg_queue,(void *) &msg_control, 0);
            continue;
        }
        pchar = strtok(NULL, ":\n");
    }
    
    free(messageTemp);
}

void LCD_display_task(void *arg){
    char hvac0[20];
    char hvac1[20];
    while(1){
        if(check){
            lcd.clear();
            sprintf(hvac1, "A: %.0f\0", HVA_controller.air.value);
            sprintf(hvac0, "T: %.1f H: %.0f\0", HVA_controller.temp.value, HVA_controller.humi.value);
            lcd.setCursor(0,0);
            lcd.print(hvac0);
            lcd.setCursor(0,1);
            lcd.print(hvac1);
            check = 0;
        }
        vTaskDelay(200/portTICK_PERIOD_MS);
    }
    
}

void HVAC_control_task(void *arg){
    while(1){
        if(xQueueReceive(msg_queue, (void *)&msg_control, 0)){
            check = 1;
            switch (msg_control.event_control)
            {
            case TEMP_CONTROL:
                HVA_controller.temp.value = msg_control.set_point;
                break;
            case HUMI_CONTROL:
                HVA_controller.humi.value = msg_control.set_point;
                break;
            case AIR_CONTROL:
                HVA_controller.air.value = msg_control.set_point;
                break;
            default:
                break;
            }
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void setup(){
    Serial.begin(115200);
    // initialize LCD
    lcd.init();
    // turn on LCD backlight                      
    lcd.backlight();
    msg_queue = xQueueCreate(6, sizeof(HVAC_event_t));

    wifi_setup();
    client.setServer(mqtt_server, 1884);
    client.setCallback(callback);

    xTaskCreatePinnedToCore(mqtt_connection_task, "mqtt task", 2048, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(LCD_display_task, "lcd task", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(HVAC_control_task, "control task", 1024, NULL, 2, NULL, 1);
}


void loop(){

}