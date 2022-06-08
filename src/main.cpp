#include <Arduino.h>
#include <IRsend.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <AH-A5SAY.h>

#define XENV(x) #x
#define ENV(x) XENV(x)

#define IRLED_PIN 14
#define LED_BUILTIN 2

const char *topic = "attic/ac";

IRsend irsend(IRLED_PIN);

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setupWiFi() {
  Serial.begin(9600);
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ENV(WIFI_SSID), ENV(WIFI_PASS));

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }

  randomSeed(micros());
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}

void setupMessageBroker() {
  espClient.setInsecure();
  client.setClient(espClient);
  client.setServer(ENV(BROKER_HOST), 8883);
  client.setCallback([](char *topic, byte *payload, unsigned int length) {
    if ((char)payload[0] == '1') {
      Serial.println("Turning on...");
      irsend.sendRaw(rawData_26c, 211, 38);
    } else {
      Serial.println("Turning off...");
      irsend.sendRaw(rawData_off, 211, 38);
    }
  });
}

void reconnectMessageBroker() {
  digitalWrite(LED_BUILTIN, LOW);

  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(ENV(CLIENT_ID), ENV(BROKER_USERNAME), ENV(BROKER_PASSWORD))) {
      Serial.print("connected ");
      Serial.println(ENV(CLIENT_ID));

      client.subscribe(topic);
    } else {
      Serial.println("connection failed, try again in 5 seconds");
      delay(5000);
    }
  }
  
  digitalWrite(LED_BUILTIN, HIGH);
}

void setup() {
  pinMode(IRLED_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  setupWiFi();

  setupMessageBroker();

  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  if (!client.connected()) {
    reconnectMessageBroker();
  }
  client.loop();
}