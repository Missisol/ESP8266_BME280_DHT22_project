#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTTYPE DHT22

// Change the credentials below, so a ESP8266 connects to a router
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Raspberry Pi IP address to connect with MQTT broker
const char* mqtt_server = "YOUR_RPi_IP_Address";

// Initializes the espClient
WiFiClient espClient;
PubSubClient client(espClient);

// DHT Sensors
const int DHT1Pin = 5;
const int DHT2Pin = 4;

// Initialize DHT sensor.
DHT dht1(DHT1Pin, DHTTYPE);
DHT dht2(DHT2Pin, DHTTYPE);

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

char data[160];

// This functions connects a ESP8266 to a router
void setup_wifi() {
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
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// This functions is executed when some device publishes a message to a topic that a ESP8266 is subscribed to
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

// This functions reconnects a ESP8266 to a MQTT broker
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect

    if (client.connect("ESPDhtClient")) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// The setup function sets a DHT sensor, starts the serial communication at a baud rate of 115200
void setup() {
  dht1.begin();
  dht2.begin();
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESPDhtClient");
    
  now = millis();
  // Publishes new temperature and humidity every 60 seconds
  if (now - lastMeasure > 60000) {
    lastMeasure = now;
    float h1 = dht1.readHumidity();
    float t1 = dht1.readTemperature();
    float h2 = dht2.readHumidity();
    float t2 = dht2.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h1) || isnan(t1)) {
      Serial.println("Failed to read from DHT-1 sensor!");
      return;
    }

    if (isnan(h2) || isnan(t2)) {
      Serial.println("Failed to read from DHT-2 sensor!");
      return;
    }

    static char temperatureTemp1[7];
    dtostrf(t1, 6, 2, temperatureTemp1);
    static char humidityTemp1[7];
    dtostrf(h1, 6, 2, humidityTemp1);

    static char temperatureTemp2[7];
    dtostrf(t2, 6, 2, temperatureTemp2);
    static char humidityTemp2[7];
    dtostrf(h2, 6, 2, humidityTemp2);
  
    String dhtReadings = "{ \"temperature-1\": \"" + String(temperatureTemp1) + "\", \"humidity-1\" : \"" + String(humidityTemp1) + "\", \"temperature-2\": \"" + String(temperatureTemp2) + "\", \"humidity-2\" : \"" + String(humidityTemp2) + "\"}";
    dhtReadings.toCharArray(data, (dhtReadings.length() + 1));
    
    // Publishes temperature and humidity values
    client.publish("/dht22/dhtreadings", data);
    Serial.println(data);
  }
}
