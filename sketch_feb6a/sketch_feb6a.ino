#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"
#include "ThingSpeak.h"

#define DHTPIN 4
#define DHTTYPE DHT22
#define SOIL_PIN 34
#define RELAY_PIN 26

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "A";
const char* password = "yuvi12345";

unsigned long channelID = 3252160;
const char* writeAPI = "1TZ7JM1RVZTK5HOY";

WiFiClient client;
WebServer server(80);

String pumpState = "OFF";

unsigned long lastTime = 0;
const long interval = 5000;

// -------- ROUTES --------
void handleRoot(){
  server.send(200, "text/plain", "ESP32 Running");
}

void handleON(){
  digitalWrite(RELAY_PIN, LOW);
  pumpState = "ON";
  Serial.println("Pump ON");
  server.send(200, "text/plain", "ON");
}

void handleOFF(){
  digitalWrite(RELAY_PIN, HIGH);
  pumpState = "OFF";
  Serial.println("Pump OFF");
  server.send(200, "text/plain", "OFF");
}

void handleSTATUS(){
  server.send(200, "text/plain", pumpState);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  // Sync state
  if (digitalRead(RELAY_PIN) == LOW)
    pumpState = "ON";
  else
    pumpState = "OFF";

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected ✅");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);

  server.on("/", handleRoot);
  server.on("/ON", handleON);
  server.on("/OFF", handleOFF);
  server.on("/STATUS", handleSTATUS);

  server.begin();
}

void loop() {

  server.handleClient();

  // reconnect WiFi if needed
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
  }

  if (millis() - lastTime >= interval) {
    lastTime = millis();

    int soil = analogRead(SOIL_PIN);
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) return;

    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, hum);
    ThingSpeak.setField(3, soil);

    ThingSpeak.writeFields(channelID, writeAPI);
  }
}