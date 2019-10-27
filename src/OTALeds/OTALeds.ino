// #include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <SPI.h>  //SPI
#include <WiFiUdp.h>
#include <math.h>

#define FASTLED_ESP8266_RAW_PIN_ORDER

#include "FastLED.h"
#include "geometry.h"
#include "sun.h"
#include "rain.h"
#include "weather.h"
#include "mqtt.h"

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
  // RainInit();
  Serial.println("Starting fastLED");
  FastLED.addLeds<WS2801, 13, 14>(leds, NUM_LEDS);
  getWeather();
  // delay(1000);
  // FastLED.show();
}


Sun sun;
Mqtt mqtt;
bool active;

void loop() {
  mqtt.Update();
  // eye.Update();
  sun.Update();
  // Rain();
  // static float phi = 0.0f;
  for (int i = 0; i < NUM_LEDS; ++i) {
    // auto hsv = rgb2hsv_approximate(leds[i]);
    // hsv.v = min(hsv.v, light);
    if (!active) {
      // hsv.v = 0;
      leds[i] = CRGB(0, 0, 0);
    }
  }

  FastLED.show();
  delay(20);
  ArduinoOTA.handle();
}
