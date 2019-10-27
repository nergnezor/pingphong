#include "mqtt.h"
#include "weather.h"
#include <ArduinoJson.h>

#define mqtt_server "hassio"
#define mqtt_user "DVES_USER"
#define mqtt_password "DVES_PASS"

// extern WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
String switch1;
String strTopic;
String strPayload;
    byte light = 255;

extern bool active;
void callback(char *topic, uint8_t *payload, unsigned int length) {
  payload[length] = '\0';
  Serial.println(String((char *)topic));
  Serial.println(String((char *)payload));
  strTopic = String((char *)topic);
  if (strTopic == "pingphong/brightness/set") {
    light = String((char *)payload).toInt();
    Serial.println(light);
  } else if (strTopic == "pingphong/light/switch") {
    active = String((char *)payload) == "ON";
    mqttClient.publish("pingphong/light/status", active ? "ON" : "OFF");
  }
}
Mqtt::Mqtt(){
    // mqttClient;
    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(callback);
}

void Mqtt::Update(){
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
}




void Mqtt::reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (mqttClient.connect("ESP8266Client")) {
    if (mqttClient.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Publish a message to the status topic
      mqttClient.publish("pingphong/light/status", "ON");

      // Listen for messages on the control topic
      mqttClient.subscribe("pingphong/light/switch");
      mqttClient.subscribe("pingphong/brightness/set");
      mqttClient.subscribe("pingphong/effect/set");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}