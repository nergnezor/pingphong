// #include <bitswap.h>
// #include <chipsets.h>
// #include <color.h>
// #include <colorpalettes.h>
// #include <colorutils.h>
// #include <controller.h>
// #include <cpp_compat.h>
// #include <dmx.h>
// #include <FastLED.h>
// #include <fastled_config.h>
// #include <fastled_delay.h>
// #include <fastled_progmem.h>
// #include <fastpin.h>
// #include <fastspi.h>
// #include <fastspi_bitbang.h>
// #include <fastspi_dma.h>
// #include <fastspi_nop.h>
// #include <fastspi_ref.h>
// #include <fastspi_types.h>
// #include <hsv2rgb.h>
// #include <led_sysdefs.h>
// #include <lib8tion.h>
// #include <noise.h>
// #include <pixelset.h>
// #include <pixeltypes.h>
// #include <platforms.h>
// #include <power_mgt.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <math.h>
#include <SPI.h> //SPI
#define FASTLED_ESP8266_RAW_PIN_ORDER
// #define FASTLED_ESP8266_NODEMCU_PIN_ORDER
// #define FASTLED_ESP8266_D1_PIN_ORDER
#include "FastLED.h"
#define NUM_LEDS 50
typedef struct Bulb
{
  float y;
  float r;
  float phi;
  uint8 index;
} Bulb;
CRGB leds[NUM_LEDS];
Bulb bulbs[NUM_LEDS];

#define CS 15
#define ESP8266_LED 2
const char *ssid = "jazik";
const char *password = "R0j4rr4lf";
const char *host = "OTA-LEDS";

int led_pin = ESP8266_LED;
#define N_DIMMERS 0
int dimmer_pin[] = {14, 5, 15};
uint8 ledIndexes[] = {34, 4, 38, 5, 20,
                      28, 11, 33, 45, 14, //10
                      44, 15, 1, 39, 21,
                      32, 27, 10, 49, 6, //20
                      43, 35, 3, 37, 22,
                      19, 29, 16, 48, 40, //30
                      0, 30, 12, 9, 46,
                      13, 31, 7, 2, 36, //40
                      23, 42, 26, 18, 47,
                      17, 24, 41, 8, 25};
void getBulbs()
{
  float offset = 2. / NUM_LEDS;
  float increment = M_PI * (3. - sqrt(5.));
  for (int i = 0; i < NUM_LEDS; ++i)
  {
    float y = ((i * offset) - 1) + (offset / 2);
    float r = sqrt(1 - pow(y, 2));
    float phi = (i % NUM_LEDS) * increment;
    Serial.println("bulb:");
    Serial.println(y);
    Serial.println(r);
    Serial.println(phi);
    while (phi > 2 * M_PI){
      phi -= 2 * M_PI;
    }
    bulbs[ledIndexes[i]] = {y, r, phi, ledIndexes[i]};
    // var bulb = new Bulb(y, r, phi);
    // 		console.log('phi: ' + (phi/(2* Math.PI)) + ', real phi: ' + phi + ', r: ' + r+ ', y: ' + y);
    // bulbs[ledIndexes[i]] = bulb
    //  scene.add(bulb.model);
  }
}

void setup()
{
  Serial.begin(115200);

  for (int i = 0; i < 50; ++i)
  {
    SPI.transfer(10);
    SPI.transfer(0);
    SPI.transfer(0);
  }
  /* switch on led */
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  Serial.println("Booting");
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    WiFi.begin(ssid, password);
    Serial.println("Retrying connection...");
  }
  /* switch off led */
  digitalWrite(led_pin, HIGH);

  /* configure dimmers, and OTA server events */
  analogWriteRange(1000);
  analogWrite(led_pin, 990);

  for (int i = 0; i < N_DIMMERS; i++)
  {
    pinMode(dimmer_pin[i], OUTPUT);
    analogWrite(dimmer_pin[i], 50);
  }

  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() { // switch off all the PWMs during upgrade
    for (int i = 0; i < N_DIMMERS; i++)
      analogWrite(dimmer_pin[i], 0);
    analogWrite(led_pin, 0);
  });

  ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
    for (int i = 0; i < 30; i++)
    {
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

  for (int i = 0; i < 50; ++i)
  {
    SPI.transfer(buf[0]);
    SPI.transfer(buf[1]);
    SPI.transfer(buf[2]);
  }
  getBulbs();
  Serial.println("Starting fastLED");
  FastLED.addLeds<WS2801, 13, 14>(leds, NUM_LEDS);
  // for (int i = 0; i < NUM_LEDS; ++i)
  // {
  //   uint8 hue = 127 * (1 + bulbs[i].y);
  //   leds[i] = CHSV(hue, 255, 255);
  // }
}

uint8 counter = 0;
float yLoc = 0.01;
void loop()
{
  ArduinoOTA.handle();
  // Serial.println("fastLED: White");
  // leds[0] = CRGB::White;
  for (int i = 0; i < NUM_LEDS; ++i)
  {
    // uint8 hue = 127 * ((1 + bulbs[i].y) - yLoc);

    // leds[i] = CHSV(hue, 255, 30);
    // leds[i] = CHSV(i*4, 255, 30);
    leds[i] = CHSV((255 * bulbs[i].phi) / (2*M_PI) , 255, 30);
  }
  FastLED.show();
  delay(20);
  ++counter;
  yLoc += 0.005;
}
