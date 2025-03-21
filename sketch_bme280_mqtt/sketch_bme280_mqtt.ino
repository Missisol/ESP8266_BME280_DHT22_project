#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

Adafruit_BME280 bme;

// Change the credentials below, so a ESP8266 connects to a router
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Raspberry Pi IP address to connect with MQTT broker
const char* mqtt_server = "YOUR_RPi_IP_Address";

WiFiClient espClient;
PubSubClient client(espClient);

long now = millis();
long lastMeasure = 0;

char data[80];

void setup_wifi() {
  delay(1000);
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

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESPBmeClient")) {
      Serial.println("connected");  
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  bool status;

  status = bme.begin();  
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())

    client.connect("ESPBmeClient");

  now = millis();
  if (now - lastMeasure > 30000) {
    lastMeasure = now;
    float t = bme.readTemperature();
    float h = bme.readHumidity();
    float p = bme.readPressure();
    float pr = bme.readPressure() / 100.0F * 0.75;

    if (isnan(h) || isnan(t) || isnan(p)) {
      Serial.println("Failed to read from BME280 sensor!");
      return;
    }

    static char temperature[7];
    dtostrf(t, 6, 1, temperature);
    static char humidity[7];
    dtostrf(h, 6, 1, humidity);
    static char pressure[5];
    dtostrf(pr, 4, 0, pressure);

    String bmeReadings = "{ \"temperature\": \"" + String(temperature) + "\", \"humidity\" : \"" + String(humidity) + "\" , \"pressure\" : \"" + String(pressure) + "\"}";
    bmeReadings.toCharArray(data, (bmeReadings.length() + 1));

    client.publish("/bme280/bmereadings", data);
    Serial.println(data);
  }
}
