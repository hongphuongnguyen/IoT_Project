#include <DHT11.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define mqtt_server "192.168.137.1"
#define ssid        "hongphuong" 
#define pass        "abcde123"

#define iot_topic    "sensor/DHT11"

#define DHTPIN 15
DHT11 dht(DHTPIN);

WiFiClient espClient;
PubSubClient client(espClient);

float humi = 0;
float temp = 0;

void setup() {
  Serial.begin(115200);
  setup_wifi();

  client.setServer(mqtt_server, 1884);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  temp = dht.readTemperature();
  humi = dht.readHumidity();
  Serial.print("Temperature = ");
  Serial.print(temp);
  Serial.println(" do C");
  Serial.print("Humidity = ");
  Serial.print(humi);
  Serial.println(" %");
  String payload = "{temperature: " + String(temp) + ", humidity: " + String(humi) + "}";
  client.publish(iot_topic, String(payload).c_str(), false);
  Serial.print("Published temperature: ");
  Serial.print(temp);
  Serial.print(" oC and humidity: ");
  Serial.print(humi);
  Serial.println(" % to topic sensor/DHT11.");
  delay(1000);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Sensor")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      delay(3000);
    }
  }
}
