#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <SPI.h>  //SPI
#include <WiFiUdp.h>
#include <math.h>

#define FASTLED_ESP8266_RAW_PIN_ORDER
// #define FASTLED_ESP8266_NODEMCU_PIN_ORDER
// #define FASTLED_ESP8266_D1_PIN_ORDER
#include <PubSubClient.h>
#include "FastLED.h"
#include "geometry.h"

#define mqtt_server "hassio"
#define mqtt_user "DVES_USER"
#define mqtt_password "DVES_PASS"

// open weather map api key
String apiKey = "7067e6fe9d7d2c44fcb05d94ec932a98";

// the city you want the weather for
String location = "stockholm,SE";

int status = WL_IDLE_STATUS;
char server[] = "api.openweathermap.org";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
String switch1;
String strTopic;
String strPayload;
CRGB leds[NUM_LEDS];
Bulb bulbs[NUM_LEDS];

#define CS 15
#define ESP8266_LED 2
const char *ssid = "💀";
const char *password = "";
const char *host = "OTA-LEDS";
IPAddress ip(192, 168, 0, 201);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

int led_pin = ESP8266_LED;
#define N_DIMMERS 0
int dimmer_pin[] = {14, 5, 15};

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

static byte light = 255;
static bool active = true;
void callback(char *topic, byte *payload, unsigned int length) {
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
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 50; ++i) {
    SPI.transfer(1);
    SPI.transfer(0);
    SPI.transfer(0);
  }
  /* switch on led */
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.hostname(host);
  // WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.println("Retrying connection...");
  }
  /* switch off led */
  digitalWrite(led_pin, HIGH);

  /* configure dimmers, and OTA server events */
  analogWriteRange(1000);
  analogWrite(led_pin, 990);

  for (int i = 0; i < N_DIMMERS; i++) {
    pinMode(dimmer_pin[i], OUTPUT);
    analogWrite(dimmer_pin[i], 50);
  }

  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() {  // switch off all the PWMs during upgrade
    for (int i = 0; i < N_DIMMERS; i++) analogWrite(dimmer_pin[i], 0);
    analogWrite(led_pin, 0);
  });

  ArduinoOTA.onEnd([]() {  // do a fancy thing with our board led at end
    for (int i = 0; i < 30; i++) {
      analogWrite(led_pin, (i * 100) % 1001);
      delay(50);
    }
  });

  ArduinoOTA.onError([](ota_error_t error) { ESP.restart(); });

  /* setup the OTA server */
  ArduinoOTA.begin();
  Serial.println("Ready");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip.toString());
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);

  SPI.begin();
  uint8_t buf[] = {255, 20, 0};

  for (int i = 0; i < 50; ++i) {
    SPI.transfer(buf[0]);
    SPI.transfer(buf[1]);
    SPI.transfer(buf[2]);
  }
  getBulbs(bulbs);
  RainInit();
  Serial.println("Starting fastLED");
  FastLED.addLeds<WS2801, 13, 14>(leds, NUM_LEDS);
  getWeather();
  // delay(1000);
  // FastLED.show();
}

DEFINE_GRADIENT_PALETTE(heatmap_gp){
    0,   255, 200, 100,  //
    40,  0,   0,   100,  //
    120, 200, 50,  0,    //
    255, 100, 0,   0,    //
                         // 255, 0,   0,   0,    //
};
DEFINE_GRADIENT_PALETTE(rio_gp){
    0,   0,   255, 0,    // green
    60,  0,   0,   255,  // blue
    130, 128, 0,   255,  // purple
    255, 255, 0,   200,  // pink
};

class Sun {
 public:
  Point location;
  Sun() {
    location.x = 0.5;
    location.z = 0.5;
    location.r = 1;
  }
  void Update() {
    Move();
    for (int i = 0; i < NUM_LEDS; ++i) {
      float distance = getDistance(location, bulbs[i].location) / 1.3;
      uint8_t heatindex = distance * 255;
      leds[i] = ColorFromPalette(myPal, heatindex);
    }
  }

 private:
  CRGBPalette16 myPal = heatmap_gp;
  void Move() {
    location.y += -0.005;
    location.phi += -0.05;
    if (location.phi > 2 * PI) {
      location.phi -= 2 * PI;
    }
    uint8 theta = UINT8_MAX * location.phi / (2 * PI);
    location.x = (cos8(theta) / (float)UINT8_MAX) * location.r;
    location.z = (sin8(theta) / (float)UINT8_MAX) * location.r;
  }
};

#define N_RAINDROPS_MAX 10
#define N_DROPBULBS_MAX 10
struct WaterBulb;
struct BulbBelow {
  WaterBulb *bulb;
  int index;
  float distance;
  int amount;
};
struct WaterBulb {
  BulbBelow below[N_DROPBULBS_MAX];
  int belowCount;
  int amount;
  bool wasFull;
};
// struct Rain
// {
//   Raindrop drops[N_RAINDROPS_MAX];
//   int count;
// };
// struct Rain rain;

static WaterBulb waterBulbs[NUM_LEDS] = {0};
void Rain() {
  for (int i = 0; i < NUM_LEDS; ++i) {
    if (bulbs[i].location.y < 0.01) {
      // if (waterBulbs[i].amount == 0)
      waterBulbs[i].amount = 100;
      // waterBulbs[i].amount = Min(255, waterBulbs[i].amount + 1);
      waterBulbs[i].wasFull = true;
    }
    if (waterBulbs[i].amount > 0) {
      if (waterBulbs[i].amount >= 255) {
        waterBulbs[i].wasFull = true;
      }
      if (waterBulbs[i].wasFull) {
        for (int k = 0; k < waterBulbs[i].belowCount; ++k) {
          BulbBelow *below = &waterBulbs[i].below[k];
          if (below == 0) break;
          float dy = bulbs[below->index].location.y - bulbs[i].location.y;
          // int leakage = dy * 20;
          int leakage = dy * 20 + below->distance / 1;
          leakage *= 4;
          // Serial.println(leakage);
          waterBulbs[below->index].amount =
              Min(255, waterBulbs[below->index].amount + leakage);
          waterBulbs[i].amount = Max(0, waterBulbs[i].amount - leakage);
        }
        if (waterBulbs[i].amount <= 0) {
          waterBulbs[i].wasFull = false;
          waterBulbs[i].amount = 0;
        }
      }
    }
  }
  for (int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = CHSV(150 - waterBulbs[i].amount / 20, 200, waterBulbs[i].amount);
  }
}

void RainInit() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (bulbs[i].location.y < 0.01) {
      waterBulbs[i].amount = 255;
      waterBulbs[i].wasFull = true;
    }
    for (int j = 0; j < NUM_LEDS; j++) {
      if (i == j || bulbs[j].location.y < bulbs[i].location.y) continue;
      float distance =
          Min(1, getDistance(bulbs[i].location, bulbs[j].location));
      // Serial.println(distance);
      if (distance > 0.5) continue;
      for (int k = 0; k < N_DROPBULBS_MAX; ++k) {
        BulbBelow *below = &waterBulbs[i].below[k];
        if (k >= waterBulbs[i].belowCount || distance < below->distance) {
          if (distance < below->distance) {
            for (int l = N_DROPBULBS_MAX - 1; l > k; --l) {
              waterBulbs[i].below[l] = waterBulbs[i].below[l - 1];
            }
          }
          below->bulb = &waterBulbs[j];
          below->index = j;
          below->distance = distance;
          ++waterBulbs[i].belowCount;
          break;
        }
      }
      // Serial.println(waterBulbs[i].belowCount);
    }
  }
  Rain();
}

Sun sun;
void reconnect() {
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

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
  // eye.Update();
  sun.Update();
  // Rain();
  // static float phi = 0.0f;
  for (int i = 0; i < NUM_LEDS; ++i) {
    auto hsv = rgb2hsv_approximate(leds[i]);
    hsv.v = min(hsv.v, light);
    if (!active) hsv.v = 0;
    leds[i] = CRGB(hsv);
  }

  // for (CRGB rgb : leds) {
  //   auto hsv = rgb2hsv_approximate(rgb);
  //   hsv.v = min(hsv.v, light);

  //   rgb = CRGB(hsv);
  // }
  // phi += 0.1;
  // while (phi > 2 * PI)
  // {
  //   phi -= 2 * PI;
  // }
  FastLED.show();
  delay(10);
  ArduinoOTA.handle();
}
