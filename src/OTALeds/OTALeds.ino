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
#include "FastLED.h"

#include "geometry.h"
// open weather map api key
String apiKey = "7067e6fe9d7d2c44fcb05d94ec932a98";

// the city you want the weather for
String location = "stockholm,SE";

int status = WL_IDLE_STATUS;
char server[] = "api.openweathermap.org";

WiFiClient client;

CRGB leds[NUM_LEDS];
Bulb bulbs[NUM_LEDS];

#define CS 15
#define ESP8266_LED 2
const char *ssid = "jazik";
const char *password = "R0j4rr4lf";
const char *host = "OTA-LEDS";
IPAddress ip(192, 168, 0, 201);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

int led_pin = ESP8266_LED;
#define N_DIMMERS 0
int dimmer_pin[] = {14, 5, 15};

void getWeather() {
  client.stop();
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.print("GET /data/2.5/forecast?");
    client.print("q=" + location);
    client.print("&cnt=3");
    client.print("&appid=" + apiKey);
    client.println("&units=metric");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("unable to connect");
  }

  delay(1000);
  String line = "";

  while (client.available()) {
    Serial.println("parsingValues");
    line = client.readStringUntil('\n');

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
    // // get the data from the json tree
    // String nextWeatherTime0 = root["list"][0]["dt_txt"];
    // String nextWeather0 = root["list"][0]["weather"][0]["main"];

    // String nextWeatherTime1 = root["list"][1]["dt_txt"];
    // String nextWeather1 = root["list"][1]["weather"][0]["main"];

    // String nextWeatherTime2 = root["list"][2]["dt_txt"];
    // String nextWeather2 = root["list"][2]["weather"][0]["main"];
    // float temp = root["list"][0]["main"]["temp"];

    // // Print values.
    // Serial.println(nextWeatherTime0);
    // Serial.println(nextWeather0);
    // Serial.println(nextWeatherTime1);
    // Serial.println(nextWeather1);
    // Serial.println(nextWeatherTime2);
    // Serial.println(nextWeather2);
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

int limit = 0;
class Eye {
 public:
  Bulb Eye_target;
  Eye() {
    Eye_target.location.r = 1;
    Eye_target.location.y = 0.8;
  }
  int Update() {
    Move();
    Bulb *target = &Eye_target;
    int closestIndex = -1;
    uint minDist = UINT32_MAX;
    for (int i = 0; i < NUM_LEDS; ++i) {
      float distance = MIN(1, getDistance(target->location, bulbs[i].location));
      leds[i] = CHSV(30 + 100 * distance, 255 - 150 * distance,
                     Max(0, 255 - 120 * distance));
    }
    return minDist;
  }

 private:
  int counter = 0;
  float yLoc = 0.0001;
  void Move() {
    Eye_target.location.phi += 0.01;
    // Eye_target.location.r += 0.001;
    if (Eye_target.location.r > 1.2) {
      Eye_target.location.r = 0;
    }
    if (Eye_target.location.phi > 2 * M_PI) {
      Eye_target.location.phi -= 2 * M_PI;
    }
    uint8 theta = UINT8_MAX * Eye_target.location.phi / (2 * M_PI);
    Eye_target.location.x =
        (cos8(theta) / (float)UINT8_MAX) * Eye_target.location.r;
    Eye_target.location.z =
        (sin8(theta) / (float)UINT8_MAX) * Eye_target.location.r;
  }
};

void VerticalRainbow() {
  for (int i = 0; i < NUM_LEDS; ++i) {
    // uint8 hue = 127 * ((1 + bulbs[i].y) - yLoc);

    // leds[i] = CHSV(hue, 255, 30);
    // leds[i] = CHSV(i*4, 255, 30);
    leds[i] = CHSV((255 * bulbs[i].location.phi) / (2 * M_PI), 255, 30);
  }
}

Eye eye;
DEFINE_GRADIENT_PALETTE(heatmap_gp){0, 255, 255, 0,  // bright yellow
                                    50, 255, 0, 0,   // red
                                    70, 255, 255, 255,

                                    // full white
                                    255, 150, 150, 255};  // full white

class Sun {
 public:
  Point location;
  Sun() {
    location.x = 0.5;
    location.y = 0;
    location.z = 0.5;
  }
  void Update() {
    // Move();
    // int closestIndex = -1;
    // uint minDist = UINT32_MAX;
    for (int i = 0; i < NUM_LEDS; ++i) {
      float distance = MIN(1, getDistance(location, bulbs[i].location));
      // leds[i] = CHSV(30 + 100 * distance, 255 - 150 * distance, Max(0, 255 -
      // 120 * distance));
      CHSV color = CHSV(150, 150, 255);
      if (distance < 0.3) {
        color.hue = 64;
        color.saturation = 255;
      }
      leds[i] = color;
      uint8_t heatindex = distance * 255;
      leds[i] = ColorFromPalette(myPal, heatindex);
    }
    // return minDist;
  }

 private:
  int counter = 0;
  float yLoc = 0.0001;

  CRGBPalette16 myPal = heatmap_gp;
  void Move() {
    location.phi += 0.05;
    // location.r += 0.001;
    if (location.r > 1.2) {
      location.r = 0;
    }
    if (location.phi > 2 * M_PI) {
      location.phi -= 2 * M_PI;
    }
    uint8 theta = UINT8_MAX * location.phi / (2 * M_PI);
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
void loop() {
  ArduinoOTA.handle();

  // eye.Update();
  // sun.Update();
  Rain();
  // static float phi = 0.0f;
  // for (int i = 0; i < NUM_LEDS; ++i)
  // {
  //   leds[i] = CHSV(255 * (bulbs[i].location.phi - phi)/(2*M_PI), 255, 255);
  // }
  // phi += 0.1;
  // while (phi > 2 * M_PI)
  // {
  //   phi -= 2 * M_PI;
  // }
  FastLED.show();
  delay(30);
}
