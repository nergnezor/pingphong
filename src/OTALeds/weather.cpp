#include "weather.h"
#include <ArduinoJson.h>

// open weather map api key
String apiKey = "7067e6fe9d7d2c44fcb05d94ec932a98";

// the city you want the weather for
String location = "stockholm,SE";

int status = WL_IDLE_STATUS;
char server[] = "api.openweathermap.org";
WiFiClient wifiClient;
void getWeather() {
  wifiClient.stop();
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (wifiClient.connect(server, 80)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    wifiClient.print("GET /data/2.5/forecast?");
    wifiClient.print("q=" + location);
    wifiClient.print("&cnt=3");
    wifiClient.print("&appid=" + apiKey);
    wifiClient.println("&units=metric");
    wifiClient.println("Host: api.openweathermap.org");
    wifiClient.println("Connection: close");
    wifiClient.println();
  } else {
    Serial.println("unable to connect");
  }

  delay(1000);
  String line = "";

  while (wifiClient.available()) {
    Serial.println("parsingValues");
    line = wifiClient.readStringUntil('\n');

    // Serial.println(line);

    // create a json buffer where to store the json data
    // StaticJsonBuffer<5000> jsonBuffer;
    DynamicJsonBuffer jsonBuffer;

    JsonObject &root = jsonBuffer.parseObject(line);
    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }

    for (int i = 0; i < 3; ++i) {
      float temp = root["list"][i]["main"]["temp"];
      String nextWeatherTime = root["list"][i]["dt_txt"];
      String nextWeather = root["list"][i]["weather"][0]["main"];
      Serial.println(temp);
      Serial.println(nextWeatherTime);
      Serial.println(nextWeather);
    }
  }
}