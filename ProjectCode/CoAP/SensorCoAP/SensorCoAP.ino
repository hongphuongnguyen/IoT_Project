#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>

const char* ssid = "hongphuong";
const char* password = "abcde123";

IPAddress gateway_ip(192, 168, 137, 30); // Thay bằng địa chỉ IP của ESP32 gateway
const int gateway_port = 5685; // Cổng CoAP

const int ldrPin = 34;
const int mq02Pin = 35;

WiFiUDP udp;
Coap coap(udp);

// Hàm callback để xử lý phản hồi từ gateway
void callback_response(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("Received response:");
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  Serial.println(p);

}

void setup() {
  Serial.begin(9600);

  pinMode(ldrPin, INPUT);
  pinMode(mq02Pin, INPUT);
  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  coap.start();
  
  // Đăng ký hàm callback để nhận phản hồi
  coap.response(callback_response);
}

void loop() {
  int ldrValue = analogRead(ldrPin);
  int ldr = map(ldrValue, 0, 3500, 100, 0);
  if(ldr < 0) ldr = 0;
  //.println("LDR Value Analog: " + String(ldrValue));
  Serial.println("Do sang: " + String(ldr) + "%");

  int mq02Value = analogRead(mq02Pin);
  float voltage = mq02Value * (5 / 4095.0);
  float ratio = voltage/(5 - voltage);
  float mq02 = 10 * pow(ratio, -2.5);
  //Serial.println("MQ02 Value Analog: " + String(mq02Value));
  Serial.println("Nong do khoi: " + String(mq02) + "ppm");

  String payload = "{ldr: " + String(ldr) + ", airquality: " + String(mq02) + "}";
  //String payload = "{airquality: " + String(mq02) + "}";
  
  // Gửi yêu cầu PUT đến ESP32 gateway
  coap.put(gateway_ip, gateway_port, "coap", payload.c_str());
  // Xử lý các gói CoAP
  coap.loop();
  
  delay(1000);
}
