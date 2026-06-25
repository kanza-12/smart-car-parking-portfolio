#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASS";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Servo gate;
int servoPin = 13;
int trig[3] = {5, 18, 19};
int echo[3] = {17, 16, 4};
int totalSlots = 3;
int availableSlots = 0;

float getDistance(int t, int e) {
  digitalWrite(t, LOW);
  delayMicroseconds(2);
  digitalWrite(t, HIGH);
  delayMicroseconds(10);
  digitalWrite(t, LOW);
  long duration = pulseIn(e, HIGH, 30000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void reconnect() {
  while (!client.connected()) {
    client.connect("ESP32Parking");
  }
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 3; i++) {
    pinMode(trig[i], OUTPUT);
    pinMode(echo[i], INPUT);
  }
  gate.attach(servoPin);
  gate.write(0);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  setupWiFi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();
  availableSlots = 0;

  for (int i = 0; i < 3; i++) {
    float d = getDistance(trig[i], echo[i]);
    if (d > 10) {
      availableSlots++;
    }
  }

  if (availableSlots > 0) {
    gate.write(90);
  } else {
    gate.write(0);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("SMART PARKING");
  display.setCursor(0, 15);
  display.print("Total: ");
  display.println(totalSlots);
  display.setCursor(0, 30);
  display.print("Free: ");
  display.println(availableSlots);
  display.setCursor(0, 45);
  display.println(availableSlots == 0 ? "Status: FULL" : "Status: AVAILABLE");
  display.display();

  String data = "{";
  data += "\"total\":" + String(totalSlots) + ",";
  data += "\"free\":" + String(availableSlots) + ",";
  data += "\"status\":\"" + String(availableSlots == 0 ? "FULL" : "AVAILABLE") + "\"";
  data += "}";
  client.publish("parking/data", data.c_str());
  delay(2000);
}
