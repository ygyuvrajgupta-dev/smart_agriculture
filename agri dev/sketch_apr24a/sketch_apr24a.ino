#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"
#include "ThingSpeak.h"

// -------- Pins --------
#define DHTPIN 4
#define DHTTYPE DHT22
#define SOIL_PIN 34
#define RELAY_PIN 26

DHT dht(DHTPIN, DHTTYPE);

// -------- WiFi --------
const char* ssid = "JioLEGAL_5G";
const char* password = "20802080";

// -------- ThingSpeak --------
unsigned long channelID = 3252160;
const char* writeAPI = "1TZ7JM1RVZTK5HOY";

WiFiClient client;
WebServer server(80);

// -------- Pump --------
String pumpState = "OFF";

// -------- Timing --------
unsigned long lastTime = 0;
const long interval = 5000;

// ================== ROUTES ==================

void handleON() {

  digitalWrite(RELAY_PIN, LOW);   // Active LOW relay
  pumpState = "ON";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "ON");
}

void handleOFF() {

  digitalWrite(RELAY_PIN, HIGH);
  pumpState = "OFF";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OFF");
}

void handleSTATUS() {

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", pumpState);
}

// ================== SETUP ==================

void setup() {

  Serial.begin(115200);

  dht.begin();

  pinMode(RELAY_PIN, OUTPUT);

  // Pump OFF initially
  digitalWrite(RELAY_PIN, HIGH);

  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected ✅");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);

  // -------- Web Server --------
  server.on("/ON", handleON);
  server.on("/OFF", handleOFF);
  server.on("/STATUS", handleSTATUS);

  server.begin();

  Serial.println("Server Started ✅");
}

// ================== LOOP ==================

void loop() {

  server.handleClient();

  if (millis() - lastTime >= interval) {

    lastTime = millis();

    int soil = analogRead(SOIL_PIN);

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {

      Serial.println("DHT Error ❌");
      return;
    }

    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.println(" %");

    Serial.print("Soil: ");
    Serial.println(soil);

    // Upload to ThingSpeak
    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, hum);
    ThingSpeak.setField(3, soil);

    int response = ThingSpeak.writeFields(channelID, writeAPI);

    if (response == 200) {
      Serial.println("ThingSpeak Updated ✅");
    }
    else {
      Serial.print("ThingSpeak Error: ");
      Serial.println(response);
    }

    Serial.println("-----------------------");
  }
}